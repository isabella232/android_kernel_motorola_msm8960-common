/*
 * Copyright (C) 2007 - 2011 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Brief overview about the switches used
 * ======================================
 *
 * wsdev (Whisper switch)
 *	DOCKED      : PPD(UART) dock is detected.
 *	UNDOCKED    : SMART dock is detected.
 *                    No dock is connected.
 *
 * dsdev (Standard Dock switch)
 *      NO_DOCK	    : No dock is connected.
 *	DESK_DOCK   : Not used.
 *	CAR_DOCK    : CAR (PPD/SPD) dock is detected.
 *	LE_DOCK     : BASIC(PPD/SPD) dock is detected.
 *	HD_DOCK     : LAPDOCK(SMART) dock with LID open detected.
 *		    : HD-MM(SMART) dock is detected.
 *
 * edsdev (Motorola Dock switch)
 *      NO_DOCK	    : No dock is connected.
 *	DESK_DOCK   : DESK (PPD) dock is detected.
 *	CAR_DOCK    : CAR (PPD) dock is detected.
 *	MOBILE_DOCK : LAP/MOBILE(SMART) dock is detected.
 *	HD_DOCK     : HD(SMART) dock is detected.
 *	LE_DOCK     : Low end dock is detected in HC.
 *	HE_DOCK     : High end dock is detected in HC.
 *
 * emusdev (Audio switch)
 *	NO_DEVICE   : Audio cable not present.
 *	EMU_OUT     : Audio cable detected on a PPD dock.
 *	SPDIF_AUDIO_OUT : Audio cable detected on a SMART dock.
 *
 * asdev (Audio switch in HC, need clean up when merge to ice cream)
 *	HC_NO_DEVICE   : Audio cable not present.
 *	HC_EMU_OUT     : Audio cable detected on a PPD dock.
 *	HC_SPDIF_AUDIO_OUT : Audio cable detected on a SMART dock.
 *
 * sdsdev (Smart Dock switch) - Used only by Dock Status App
 *	DOCKED      : SMART dock is detected.
 *	UNDOCKED    : No dock is connected.
 *
 * noauthdev (No authentication switch) - Used to indicate if
 *                                        authentication is needed
 *      AUTH_REQUIRED     : Need to authenticate
 *      AUTH_NOT_REQUIRED : Do not need to authenticate, already done
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/pm_runtime.h>
#include <linux/emu-accy.h>
#include <linux/usb.h>
#include <linux/usb/otg.h>
#include <linux/usb/ulpi.h>
#include <linux/usb/gadget.h>
#include <linux/usb/hcd.h>
#include <linux/usb/msm_hsusb.h>
#include <linux/usb/msm_hsusb_hw.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#include <linux/mfd/pm8xxx/gpio.h>
#include <linux/mfd/pm8xxx/mpp.h>

#include <mach/irqs.h>
#include <mach/mmi_emu_det.h>
#include <mach/gpiomux.h>
#include <mach/msm_iomap.h>

#define PM8921_IRQ_BASE		(NR_MSM_IRQS + NR_GPIO_IRQS)

#define MSM_USB_BASE		(emud->regs)
#define PHY_SESS_VLD		(1 << 29)
#define PHY_SESS_VLD_CHG	(1 << 30)

#define PRINT_STATUS	(1U << 2)
#define PRINT_TRANSITION (1U << 1)
#define PRINT_DEBUG	(1U << 3)
#define PRINT_ERROR	(1U << 0)
#define PRINT_PARANOIC	(1U << 4)

#undef DEF
#define LIMITS { \
	DEF(FACTORY,	2500, 2680), \
	DEF(FLOAT_UP,	2200, 2480), \
	DEF(FLOAT_P,	10, 60), \
	DEF(GROUND,	60, 90), \
	DEF(STD_100K,	940, 1100), \
	DEF(STD_200K,	1400, 1590), \
	DEF(ALT_100K,	1550, 1700), \
	DEF(ALT_200K,	1850, 2100), \
	DEF(VBUS_PRESENT, 700, 5200), \
	DEF(MAX_NUM,	-1, -1) }

#define DEF(_idx, _min, _max)		REF_##_idx
enum voltage_limits LIMITS;
#undef DEF

struct v_limits {
	int min, max;
};

#define DEF(_idx, _min, _max) \
	[REF_##_idx] = { \
		.min = _min, \
		.max = _max, \
	}
static struct v_limits voltage_refs[REF_MAX_NUM+1] = LIMITS;
#undef DEF

static bool in_range(int voltage, int idx)
{
	bool result = false;
	if (idx >= 0 && idx < REF_MAX_NUM) {
		if ((voltage >= voltage_refs[idx].min) &&
		    (voltage <= voltage_refs[idx].max))
			result = true;
	}
	return result;
}

#define ULPI_REG_READ(a)	a
#define ULPI_REG_SET(a)		(a+1)
#define ULPI_REG_CLEAR(a)	(a+2)

#define ALT_INT_ENABLE	0x93
#define CHG_DET_CTRL	0x84

#define ALT_INT_LATCH	0x91
#define CHG_DET_OUT	0x87

#define OTGSC_BSEIE	(1<<28)	/* B session end interrupt enable */
#define OTGSC_BSEIS	(1<<20)	/* B session end interrupt state */
#define OTGSC_BSE	(1<<12)	/* B session end */

#define OTGSC_ASVIE	(1<<26) /* A session valid interrupt enable */
#define OTGSC_ASVIS	(1<<18) /* A session valid interrupt state */
#define OTGSC_ASV	(1<<10) /* A session valid */

#define OTGSC_AVVIE	(1<<25) /* A VBUS valid interrupt enable */
#define OTGSC_AVVIS	(1<<17) /* A VBUS valid interrupt state */
#define OTGSC_AVV	(1<<9)  /* A VBUS valid */

#define PORTSC_LS_DM	(1<<10)
#define PORTSC_LS_DP	(1<<11)
#define PORTSC_LS_SE1	(PORTSC_LS_DM | PORTSC_LS_DP)

#define MUXMODE_USB	0
#define MUXMODE_UART	1
#define MUXMODE_AUDIO	2

static int debug_mask = PRINT_ERROR | PRINT_DEBUG |
	PRINT_STATUS | PRINT_TRANSITION /*| PRINT_PARANOIC*/;

#define pr_emu_det(debug_level_mask, fmt, args...) \
	do { \
		if (debug_mask & \
		    PRINT_##debug_level_mask) { \
			pr_info("EMU-det(%s): " fmt, __func__, ##args); \
		} \
	} while (0)


static BLOCKING_NOTIFIER_HEAD(emu_det_notifier_list);

void emu_det_register_notify(struct notifier_block *nb)
{
	blocking_notifier_chain_register(&emu_det_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(emu_det_register_notify);

#undef DEF
#define STATES	{ \
	DEF(CONFIG), \
	DEF(SAMPLE_1), \
	DEF(SAMPLE_2), \
	DEF(WAIT_4_DCD), \
	DEF(DCD_DONE), \
	DEF(CHG_DET), \
	DEF(CHG_DET_WALL), \
	DEF(IDENTIFY), \
	DEF(IDENTIFY_WHISPER_SEMU), \
	DEF(IDENTIFY_WHISPER_SMART), \
	DEF(USB), \
	DEF(FACTORY), \
	DEF(CHARGER), \
	DEF(WHISPER_PPD), \
	DEF(WHISPER_SMART), \
	DEF(CHARGER_INDUCTIVE), \
	DEF(USB_ADAPTER), \
	DEF(WHISPER_PPD_UNKNOWN), \
	DEF(WHISPER_SMART_LD2_OPEN), \
	DEF(WHISPER_SMART_LD2_CLOSE), \
	DEF(STATE_MAX) }


#define DEF(a)	a
enum emu_det_state STATES;
#undef DEF

#define DEF(a)	#a
static char *state_names[] = STATES;
#undef DEF

enum {
	NO_DOCK,
	DESK_DOCK,
	CAR_DOCK,
	MOBILE_DOCK,
	HD_DOCK,
	LE_DOCK,
	HE_DOCK,
};

enum {
	NO_DEVICE,
	HEADSET_WITH_MIC,
	HEADSET_WITHOUT_MIC,
	SPDIF_AUDIO_OUT = 0x4,
};

enum {
	HC_NO_DEVICE,
	HC_EMU_OUT,
	HC_SPDIF_AUDIO_OUT,
};

enum {
	AUTH_NOT_STARTED,
	AUTH_IN_PROGRESS,
	AUTH_FAILED,
	AUTH_PASSED,
};

enum {
	UNDOCKED,
	DOCKED,
};

enum {
	AUTH_REQUIRED,
	AUTH_NOT_REQUIRED
};

enum {
	NORMAL,
	HIGH
};

#undef DEF
#define ACCYS { \
	DEF(USB), \
	DEF(FACTORY), \
	DEF(CHARGER), \
	DEF(USB_DEVICE), \
	DEF(WHISPER_PPD), \
	DEF(WHISPER_SPD), \
	DEF(WHISPER_SMART), \
	DEF(NONE), \
	DEF(UNKNOWN), \
	DEF(MAX) }

#define DEF(a)	ACCY_##a
enum emu_det_accy ACCYS;
#undef DEF

#define DEF(a)	#a
static char *accy_names[] = ACCYS;
#undef DEF

enum emu_det_interrupts {
	SEMU_PPD_DET_IRQ = 0,
	EMU_SCI_OUT_IRQ,
	PHY_USB_IRQ,
	EMU_DET_MAX_INTS = 8,
	/* just a definition */
	PMIC_USBIN_IRQ,
};

enum emu_det_gpios {
	MSMGPIO_BASE = 0,
	EMU_SCI_OUT_GPIO = MSMGPIO_BASE,
	EMU_ID_EN_GPIO,
	EMU_MUX_CTRL0_GPIO,
	EMU_MUX_CTRL1_GPIO,
	PMGPIO_BASE,
	SEMU_PPD_DET_GPIO = PMGPIO_BASE,
	SEMU_ALT_MODE_EN_GPIO,
	PMMPP_BASE,
	EMU_ID_GPIO = PMMPP_BASE,
	EMU_DET_MAX_GPIOS,
};

#undef DEF
#define BITS { \
	DEF(B_SESSVLD), \
	DEF(B_SESSEND), \
	DEF(VBUS_VLD), \
	DEF(SESS_VLD), \
	DEF(DCD), \
	DEF(CHG_DET), \
	DEF(DP), \
	DEF(DM), \
	DEF(SE1), \
	DEF(FACTORY), \
	DEF(FLOAT), \
	DEF(GRND), \
	DEF(PD_100K), \
	DEF(PD_200K), \
	DEF(PD_440K), \
	DEF(SENSE_MAX) }

#define DEF(a)	a##_BIT
enum emu_det_sense_bits BITS;
#undef DEF

#define DEF(a)	#a
static char *bit_names[] = BITS;
#undef DEF

/***********************************************************************
#define SENSE_USB           (CPCAP_BIT_ID_FLOAT_S  | \
			     CPCAP_BIT_CHRGCURR1_S | \
			     CPCAP_BIT_SESSVLD_S)

#define SENSE_FACTORY       (CPCAP_BIT_ID_FLOAT_S  | \
			     CPCAP_BIT_ID_GROUND_S | \
			     CPCAP_BIT_SESSVLD_S)

#define SENSE_FACTORY_MASK   (~(CPCAP_BIT_DP_S_LS | \
			      CPCAP_BIT_CHRGCURR1_S))

#define SENSE_CHARGER_FLOAT (CPCAP_BIT_ID_FLOAT_S  | \
			     CPCAP_BIT_CHRGCURR1_S | \
			     CPCAP_BIT_SESSVLD_S   | \
			     CPCAP_BIT_SE1_S       | \
			     CPCAP_BIT_DM_S_LS     | \
			     CPCAP_BIT_DP_S_LS)

#define SENSE_CHARGER       (CPCAP_BIT_CHRGCURR1_S | \
			     CPCAP_BIT_SESSVLD_S   | \
			     CPCAP_BIT_SE1_S       | \
			     CPCAP_BIT_DM_S_LS     | \
			     CPCAP_BIT_DP_S_LS)

#define SENSE_WHISPER_SPD    (CPCAP_BIT_CHRGCURR1_S | \
			      CPCAP_BIT_SESSVLD_S)

#define SENSE_CHARGER_IND   (CPCAP_BIT_ID_FLOAT_S  | \
			     CPCAP_BIT_CHRGCURR1_S | \
			     CPCAP_BIT_SESSVLD_S   | \
			     CPCAP_BIT_DP_S_LS     | \
			     GPIO_BIT_SWST1)

#define SENSE_CHARGER_MASK  (CPCAP_BIT_ID_GROUND_S | \
			     CPCAP_BIT_SESSVLD_S)

#define SENSE_WHISPER_PPD   (CPCAP_BIT_CHRGCURR1_S | \
			     CPCAP_BIT_DP_S_LS)

#define SENSE_WHISPER_PPD_MASK   (CPCAP_BIT_ID_FLOAT_S | \
				  CPCAP_BIT_SESSVLD_S)

#define SENSE_WHISPER_CABLE (CPCAP_BIT_CHRGCURR1_S)

#define SENSE_WHISPER_SMART (CPCAP_BIT_ID_GROUND_S | \
			     CPCAP_BIT_SESSVLD_S   | \
			     CPCAP_BIT_CHRGCURR1_S | \
			     CPCAP_BIT_DP_S_LS)

#define SENSE_WHISPER_LD2   (CPCAP_BIT_ID_GROUND_S | \
			     CPCAP_BIT_SESSVLD_S   | \
			     CPCAP_BIT_CHRGCURR1_S | \
			     CPCAP_BIT_SE1_S       | \
			     CPCAP_BIT_DM_S_LS     | \
			     CPCAP_BIT_DP_S_LS)

#define SENSE_USB_ADAPTER      (CPCAP_BIT_ID_GROUND_S)
#define SENSE_USB_ADAPTER_MASK      (~(CPCAP_BIT_CHRGCURR1_S | \
				CPCAP_BIT_SE1_S | \
				CPCAP_BIT_DP_S_LS))
********************************************************************/

#define SENSE_USB           (test_bit(FLOAT_BIT, &data->sense) && \
			     test_bit(B_SESSVLD_BIT, &data->sense) && \
			     test_bit(SESS_VLD_BIT, &data->sense))

#define SENSE_FACTORY       (test_bit(FACTORY_BIT, &data->sense) && \
			     test_bit(FLOAT_BIT, &data->sense) && \
			     test_bit(B_SESSVLD_BIT, &data->sense) && \
			     test_bit(SESS_VLD_BIT, &data->sense))

#define SENSE_CHARGER_FLOAT (test_bit(FLOAT_BIT, &data->sense) && \
			     test_bit(B_SESSVLD_BIT, &data->sense) && \
			     test_bit(SESS_VLD_BIT, &data->sense) && \
			     test_bit(CHG_DET_BIT, &data->sense))

#define SENSE_CHARGER       (test_bit(B_SESSVLD_BIT, &data->sense) && \
			     test_bit(SESS_VLD_BIT, &data->sense) && \
			     test_bit(CHG_DET_BIT, &data->sense))

#define SENSE_WHISPER_SPD   (test_bit(SESS_VLD_BIT, &data->sense))

#define SENSE_CHARGER_IND   (test_bit(FLOAT_BIT, &data->sense) && \
			     test_bit(DP_BIT, &data->sense) && \
			     test_bit(SESS_VLD_BIT, &data->sense))

#define SENSE_CHARGER_MASK  (test_bit(GRND_BIT, &data->sense) && \
			     test_bit(SESS_VLD_BIT, &data->sense))

#define SENSE_WHISPER_PPD   (test_bit(B_SESSEND_BIT, &data->sense) && \
			     test_bit(PD_100K_BIT, &data->sense))

#define SENSE_WHISPER_PPD_MASK   (!test_bit(SESS_VLD_BIT, &data->sense) && \
				  !test_bit(FLOAT_BIT, &data->sense))

#define SENSE_WHISPER_CABLE (0) /* FIXME: */

#define SENSE_WHISPER_SMART (test_bit(GRND_BIT, &data->sense) && \
			     test_bit(SESS_VLD_BIT, &data->sense) && \
			     test_bit(DP_BIT, &data->sense))

#define SENSE_WHISPER_LD2   (test_bit(GRND_BIT, &data->sense) && \
			     test_bit(SE1_BIT, &data->sense) && \
			     test_bit(SESS_VLD_BIT, &data->sense) && \
			     test_bit(DM_BIT, &data->sense) && \
			     test_bit(DP_BIT, &data->sense))

#define SENSE_USB_ADAPTER   (test_bit(GRND_BIT, &data->sense))

struct emu_det_data {
	struct device *dev;
	struct delayed_work timer_work;
	struct delayed_work irq_work;
	struct workqueue_struct *wq;
	unsigned long sense;
	unsigned long prev_sense;
	u32 otgsc_mask;
	enum emu_det_state state;
	enum emu_det_accy accy;
	struct platform_device *usb_dev;
	struct platform_device *usb_connected_dev;
	struct platform_device *charger_connected_dev;
	struct regulator *regulator;
	struct wake_lock wake_lock;
	unsigned char is_vusb_enabled;
	unsigned char undetect_cnt;
	char bpass_mod;
	struct switch_dev wsdev; /* Whisper switch */
	struct switch_dev dsdev; /* Standard Dock switch */
	struct switch_dev emusdev; /* Audio switch */
	struct switch_dev asdev; /* Audio switch for HC */
	struct switch_dev csdev;  /*charge capablity switch */
	struct switch_dev edsdev; /* Motorola Dock switch */
	struct switch_dev sdsdev; /* Smart Dock Switch */
	struct switch_dev noauthdev; /* If authentication is needed */
	DECLARE_BITMAP(enabled_irqs, EMU_DET_MAX_INTS);
	char dock_id[128];
	char dock_prop[128];
	unsigned char audio;
	short whisper_auth;
	u8 retries;
	unsigned char power_up;
	unsigned emu_gpio[EMU_DET_MAX_GPIOS],
		emu_irq[EMU_DET_MAX_INTS];
	atomic_t in_lpm, last_irq;
	int async_int;
	int usb_present;
	bool protection_forced_off;
	bool semu_alt_mode;
	bool ext_5v_switch_enabled;
	void __iomem *regs;
	struct otg_transceiver *trans;
};

static struct mmi_emu_det_platform_data *emu_pdata;
static struct emu_det_data *the_emud;
static DEFINE_MUTEX(switch_access);
static char buffer[512];

/*
static char *irq_names[23] = {"NA", "NA", "NA", "NA", "NA",
	"NA", "NA", "NA", "NA", "NA", "NA",
	"CPCAP_IRQ_VBUSOV",
	"CPCAP_IRQ_RVRS_CHRG",
	"CPCAP_IRQ_CHRG_DET",
	"CPCAP_IRQ_IDFLOAT",
	"CPCAP_IRQ_IDGND",
	"CPCAP_IRQ_SE1",
	"CPCAP_IRQ_SESSEND",
	"CPCAP_IRQ_SESSVLD",
	"CPCAP_IRQ_VBUSVLD",
	"CPCAP_IRQ_CHRG_CURR1",
	"CPCAP_IRQ_CHRG_CURR2",
	"CPCAP_IRQ_RVRS_MODE"};

static char *accy_devices[] = {
	"cpcap_usb_charger",
	"cpcap_factory",
	"cpcap_charger",
	"cpcap_whisper_ppd",
	"cpcap_whisper_smart",
};
*/

static ssize_t dock_print_name(struct switch_dev *switch_dev, char *buf)
{
	switch (switch_get_state(switch_dev)) {
	case NO_DOCK:
		return sprintf(buf, "None\n");
	case DESK_DOCK:
		return sprintf(buf, "DESK\n");
	case CAR_DOCK:
		return sprintf(buf, "CAR\n");
	case LE_DOCK:
		return sprintf(buf, "BASIC\n");
	case HE_DOCK:
		return sprintf(buf, "HD/MOBILE\n");
	}

	return -EINVAL;
}
/*
static ssize_t ext_dock_print_name(struct switch_dev *switch_dev, char *buf)
{
	switch (switch_get_state(switch_dev)) {
	case EXT_NO_DOCK:
		return sprintf(buf, "None\n");
	case EXT_DESK_DOCK:
		return sprintf(buf, "DESK\n");
	case EXT_CAR_DOCK:
		return sprintf(buf, "CAR\n");
	case EXT_MOBILE_DOCK:
		return sprintf(buf, "MOBILE\n");
	case EXT_HD_DOCK:
		return sprintf(buf, "HD\n");
	}

	return -EINVAL;
}
*/
static ssize_t emu_audio_print_name(struct switch_dev *sdev, char *buf)
{
	switch (switch_get_state(sdev)) {
	case NO_DEVICE:
		return sprintf(buf, "No Device\n");
	case HEADSET_WITH_MIC:
		return sprintf(buf, "Mono out\n");
	case HEADSET_WITHOUT_MIC:
		return sprintf(buf, "Stereo out\n");
	case SPDIF_AUDIO_OUT:
		return sprintf(buf, "SPDIF audio out\n");
	}
	return -EINVAL;
}

static char *print_sense_bits(unsigned long *sense)
{
	char *start = &buffer[0];
	int i, total = sizeof(buffer);
	buffer[0] = 0;
	for (i = 0; i < SENSE_MAX_BIT; i++)
		if (test_bit(i, sense)) {
			total -= snprintf(start, total, "%s, ", bit_names[i]);
			start = buffer + strlen(buffer);
		}
	return buffer;
}

static const char *print_accy_name(enum emu_det_accy accy)
{
	return (accy < ACCY_MAX ? accy_names[accy] : "NA");
}

static const char *print_state_name(enum emu_det_state st)
{
	return (st < STATE_MAX ? state_names[st] : "NA");
}

static void mux_ctrl_mode(int mode)
{
	struct emu_det_data *emud = the_emud;
	int sel0=0, sel1=0;
	switch(mode) {
	 case MUXMODE_USB:
			sel0 = 1; break;
	 case MUXMODE_UART:
			sel1 = 1; break;
	 case MUXMODE_AUDIO:
			sel0 = sel1 = 1; break;
	 default: return;
	}
	gpio_set_value(emud->emu_gpio[EMU_MUX_CTRL0_GPIO], sel0);
	gpio_set_value(emud->emu_gpio[EMU_MUX_CTRL1_GPIO], sel1);
}

static int adc_emu_id_get(void)
{
	struct pm8xxx_adc_chan_result res;
	memset(&res, 0, sizeof(res));
	if (pm8xxx_adc_read(CHANNEL_MPP_2, &res))
		pr_err("CHANNEL_MPP_2 ADC read error\n");
	else
		pr_emu_det(DEBUG, "emu_id=%lld mV\n", res.physical);
	return (int)res.physical;
}

static int adc_vbus_get(void)
{
	struct pm8xxx_adc_chan_result res;
	memset(&res, 0, sizeof(res));
	if (pm8xxx_adc_read(CHANNEL_USBIN, &res))
		pr_err("VBUS ADC read error\n");
	else
		pr_emu_det(DEBUG, "vbus=%lld mV\n", res.physical);
	return (int)res.physical;
}


static void emu_det_enable_irq(int interrupt)
{
	struct emu_det_data *emud = the_emud;
	if (!test_and_set_bit(interrupt, emud->enabled_irqs)) {
		pr_emu_det(PARANOIC, "enabling irq %d\n",
					emud->emu_irq[interrupt]);
		enable_irq(emud->emu_irq[interrupt]);
	}
}

static void emu_det_disable_irq(int interrupt)
{
	struct emu_det_data *emud = the_emud;
	if (test_and_clear_bit(interrupt, emud->enabled_irqs)) {
		pr_emu_det(PARANOIC, "disabling irq %d\n",
					emud->emu_irq[interrupt]);
		disable_irq_nosync(emud->emu_irq[interrupt]);
	}
}

static void phy_block_on(void)
{
	struct emu_det_data *emud = the_emud;
	struct otg_transceiver *otg = emud->trans;
	u32 func_ctrl;

	/* put the controller in non-driving mode */
	func_ctrl = otg->io_ops->read(otg, ULPI_FUNC_CTRL);
	func_ctrl &= ~ULPI_FUNC_CTRL_OPMODE_MASK;
	func_ctrl |= ULPI_FUNC_CTRL_OPMODE_NONDRIVING;
	otg->io_ops->write(otg, func_ctrl, ULPI_FUNC_CTRL);
	/* Clear charger detecting control bits */
	otg->io_ops->write(otg, 0x3F, ULPI_REG_CLEAR(CHG_DET_CTRL));
	/* Clear alt interrupt latch and enable bits */
	otg->io_ops->write(otg, 0x1F, ULPI_REG_SET(ALT_INT_LATCH));
	otg->io_ops->write(otg, 0x1F, ULPI_REG_CLEAR(ALT_INT_ENABLE));
	udelay(100);
	pr_emu_det(DEBUG, "PHY on\n");
}
/*
static void phy_block_off(void)
{
	struct emu_det_data *emud = the_emud;
	struct otg_transceiver *otg = emud->trans;
	u32 func_ctrl;

	otg->io_ops->write(otg, 0x3F, 0x86);
	otg->io_ops->write(otg, 0x1F, 0x92);
	otg->io_ops->write(otg, 0x1F, 0x95);

	func_ctrl = otg->io_ops->read(otg, ULPI_FUNC_CTRL);
	func_ctrl &= ~ULPI_FUNC_CTRL_OPMODE_MASK;
	func_ctrl |= ULPI_FUNC_CTRL_OPMODE_NORMAL;
	otg->io_ops->write(otg, func_ctrl, ULPI_FUNC_CTRL);
	pr_emu_det(DEBUG, "PHY off\n");
}

static bool dcd_check(void)
{
	struct emu_det_data *emud = the_emud;
	struct otg_transceiver *otg = emud->trans;
	u32 line_state = otg->io_ops->read(otg, 0x87);
	pr_emu_det(DEBUG, "DCD check: 0x%08x\n", line_state);
	return (line_state & 2);
}
*/

static struct pm8xxx_mpp_config_data pm_emu_id_en_config[] = {
	{       /* PMIC MPP 9 - EMU_ID_EN - protection Off */
		.type           = PM8XXX_MPP_TYPE_D_OUTPUT,
		.level          = PM8921_MPP_DIG_LEVEL_L17,
		.control        = PM8XXX_MPP_DOUT_CTRL_LOW,
	},
	{       /* PMIC MPP 9 - EMU_ID_EN - protection On */
		.type           = PM8XXX_MPP_TYPE_D_INPUT,
		.level          = PM8921_MPP_DIG_LEVEL_L17,
		.control        = PM8XXX_MPP_DIN_TO_INT,
	},
};

static void emu_id_protection_setup(bool on)
{
	struct emu_det_data *emud = the_emud;
	if (on == emud->protection_forced_off) {
		if (emud->emu_gpio[EMU_ID_EN_GPIO] > NR_MSM_GPIOS) {
			pr_emu_det(DEBUG, "re-config PMIC MPP\n");
			pm8xxx_mpp_config(emud->emu_gpio[EMU_ID_EN_GPIO],
				&pm_emu_id_en_config[on]);
		}
		if (on) {
			gpio_direction_input(
				emud->emu_gpio[EMU_ID_EN_GPIO]);
			emud->protection_forced_off = false;
		} else {
			gpio_direction_output(
				emud->emu_gpio[EMU_ID_EN_GPIO], 0);
			emud->protection_forced_off = true;
		}
		pr_emu_det(DEBUG, "EMU_ID protection is %s\n",
			emud->protection_forced_off ? "Off" : "On");
	} else
		pr_emu_det(DEBUG, "state is the same as requested\n");
}

static struct pm_gpio pm_semu_alt_mode_config[] = {
	{	/* PM GPIO 17 - SEMU_ALT_MODE_EN - standard mode */
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
		.output_value	= 0,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_L17,
		.out_strength	= PM_GPIO_STRENGTH_LOW,
		.function	= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	= 0,
		.disable_pin	= 1, /* high impedance */
	},
	{	/* PM GPIO 17 - SEMU_ALT_MODE_EN - alternate mode */
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
		.output_value	= 1,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_L17,
		.out_strength	= PM_GPIO_STRENGTH_LOW,
		.function	= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	= 0,
		.disable_pin	= 0,
	},
};

static void alt_mode_setup(bool on)
{
	struct emu_det_data *emud = the_emud;
	pm8xxx_gpio_config(emud->emu_gpio[SEMU_ALT_MODE_EN_GPIO],
			&pm_semu_alt_mode_config[on]);
	emud->semu_alt_mode = on;
	pr_emu_det(DEBUG, "SEMU_ALT_MODE_EN is %s\n",
		on ? "alternate" : "standard");
}

static void dcd_setup(int disable)
{
	struct emu_det_data *emud = the_emud;
	struct otg_transceiver *otg = emud->trans;
	otg->io_ops->write(otg, 0x10, disable ?
			ULPI_REG_CLEAR(CHG_DET_CTRL) :
			ULPI_REG_SET(CHG_DET_CTRL));
	pr_emu_det(DEBUG, "DCD %s: CHG_DET_CTRL 0x%02x\n",
		disable ? "disable" : "enable",
		otg->io_ops->read(otg, ULPI_REG_READ(CHG_DET_CTRL)));

	otg->io_ops->write(otg, 0x4, disable ?
			ULPI_REG_CLEAR(ALT_INT_ENABLE) :
			ULPI_REG_SET(ALT_INT_ENABLE));
	pr_emu_det(DEBUG, "ALT_DCD %s: ALT_INT_EN 0x%02x\n",
		disable ? "disable" : "enable",
		otg->io_ops->read(otg, ULPI_REG_READ(ALT_INT_ENABLE)));
}
/*
static void dpdm_setup(int disable)
{
	struct emu_det_data *emud = the_emud;
	struct otg_transceiver *otg = emud->trans;

	otg->io_ops->write(otg, 0x18, disable ?
			ULPI_REG_CLEAR(ALT_INT_ENABLE) :
			ULPI_REG_SET(ALT_INT_ENABLE));
	pr_emu_det(DEBUG, "ALT_DP_DM %s: ALT_INT_EN 0x%02x\n",
		disable ? "disable" : "enable",
		otg->io_ops->read(otg, ULPI_REG_READ(ALT_INT_ENABLE)));
}
*/
static void chgdet_setup(int disable, u8 mask)
{
	struct emu_det_data *emud = the_emud;
	struct otg_transceiver *otg = emud->trans;
	otg->io_ops->write(otg, (mask|0x2|0x1), disable ?
			ULPI_REG_CLEAR(CHG_DET_CTRL) :
			ULPI_REG_SET(CHG_DET_CTRL));
	pr_emu_det(DEBUG, "DP/DM source/sink %s: CHG_DET_CTRL 0x%02x\n",
		disable ? "disable" : "enable",
		otg->io_ops->read(otg, ULPI_REG_READ(CHG_DET_CTRL)));

	otg->io_ops->write(otg, 0x2, disable ?
			ULPI_REG_CLEAR(ALT_INT_ENABLE) :
			ULPI_REG_SET(ALT_INT_ENABLE));
	pr_emu_det(DEBUG, "ALT_CGHDET %s: ALT_INT_EN 0x%02x\n",
		disable ? "disable" : "enable",
		otg->io_ops->read(otg, ULPI_REG_READ(ALT_INT_ENABLE)));
}

static void otgsc_setup(int disable)
{
	struct emu_det_data *emud = the_emud;
	struct otg_transceiver *otg = emud->trans;
	u32 mask = /*OTGSC_IDIE |*/ OTGSC_BSVIE | OTGSC_BSEIE |
				OTGSC_ASVIE | OTGSC_AVVIE;
	u32 val = 0;
	u32 ulpi_val = 0;

	val = readl_relaxed(USB_OTGSC);
	if (disable) {
		val &= ~mask;
		emud->otgsc_mask &= ~(mask >> 8);
	} else {
		val |= mask;
		emud->otgsc_mask |= (mask >> 8);
	}
	writel_relaxed(val, USB_OTGSC);

	ulpi_val = ULPI_INT_IDGRD | ULPI_INT_SESS_VALID | ULPI_INT_SESS_END;

	if (disable) {
		otg->io_ops->write(otg, ulpi_val, 0x0F);
		otg->io_ops->write(otg, ulpi_val, 0x12);
	} else {
		otg->io_ops->write(otg, ulpi_val, ULPI_USB_INT_EN_RISE);
		otg->io_ops->write(otg, ulpi_val, ULPI_USB_INT_EN_FALL);
	}
	pr_emu_det(DEBUG, "OTGSC %s, clear mask 0x%08x\n",
			disable ? "disable" : "enable", emud->otgsc_mask);
}

#define alternate_mode_enable()	alt_mode_setup(1)
#define standard_mode_enable()	alt_mode_setup(0)
#define chgdet_enable(a)	chgdet_setup(0, a)
#define chgdet_disable(a)	chgdet_setup(1, a)
#define otgsc_enable()		otgsc_setup(0)
#define otgsc_disable()		otgsc_setup(1)
#define dpdm_enable()		dpdm_setup(0)
#define dpdm_disable()		dpdm_setup(1)
#define dcd_enable()		dcd_setup(0)
#define dcd_disable()		dcd_setup(1)
#define emu_id_protection_on()	emu_id_protection_setup(true)
#define emu_id_protection_off()	emu_id_protection_setup(false)

#define pmic_gpio_config_store(_idx)	pmic_gpio_config(1, _idx)
#define pmic_gpio_config_restore(_idx)	pmic_gpio_config(0, _idx)

static void sense_emu_id(unsigned long *lsense)
{
	struct emu_det_data *emud = the_emud;
	bool auto_protected = false;
	int id;

	clear_bit(PD_100K_BIT, lsense);
	clear_bit(PD_200K_BIT, lsense);

	/* determine whether id protection is on */
	if (in_range(adc_vbus_get(), REF_VBUS_PRESENT))
		auto_protected = true;
	else
		clear_bit(FACTORY_BIT, lsense);

	id = adc_emu_id_get();

	if (auto_protected && !emud->protection_forced_off) {
		if (in_range(id, REF_FACTORY)) {
			set_bit(FACTORY_BIT, lsense);
			set_bit(FLOAT_BIT, lsense);
			return;
		} else {
			clear_bit(FACTORY_BIT, lsense);
			emu_id_protection_off();
			msleep(20);
			id = adc_emu_id_get();
		}
	}
#if 0
	/* this belongs to EMU_SCI_OUT, but not happening */
	if (in_range(id, REF_GROUND))
		set_bit(GRND_BIT, lsense);
	else
		clear_bit(GRND_BIT, lsense);

	/* this belongs to SEMU_PPD_DET, but not happening reliably */
	if (in_range(id, REF_FLOAT_UP))
		set_bit(FLOAT_BIT, lsense);
	/*else
		clear_bit(FLOAT_BIT, &emud->sense);*/
#endif
	if (emud->semu_alt_mode) {
		if (in_range(id, REF_ALT_100K))
			set_bit(PD_100K_BIT, lsense);
		else if (in_range(id, REF_ALT_200K))
			set_bit(PD_200K_BIT, lsense);
	} else {
		if (in_range(id, REF_STD_100K))
			set_bit(PD_100K_BIT, lsense);
		else if (in_range(id, REF_STD_200K))
			set_bit(PD_200K_BIT, lsense);
	}
}

static int get_sense(bool do_adc)
{
	struct emu_det_data *emud = the_emud;
	struct otg_transceiver *otg = emud->trans;
	unsigned long lsense = emud->sense;
	int irq, value;
	bool is_dcd_en, is_chgdet_en;
	u32 otgsc, portsc;
	u8 ulpi_reg;

	irq = atomic_read(&emud->last_irq);
/*
	clear_bit(DCD_BIT, &lsense);
	clear_bit(CHG_DET_BIT, &lsense);
*/
/*	if (irq == emud->emu_irq[SEMU_PPD_DET_IRQ]) {*/

		value = gpio_get_value_cansleep(
					emud->emu_gpio[SEMU_PPD_DET_GPIO]);
		pr_emu_det(DEBUG, "SEMU_PPD_DET indicates ID %s floating\n",
					value ? "not" : "");
		if (value) {
			clear_bit(FLOAT_BIT, &lsense);
		} else {
			set_bit(FLOAT_BIT, &lsense);
			clear_bit(GRND_BIT, &lsense);
		}
		value = gpio_get_value_cansleep(
					emud->emu_gpio[EMU_SCI_OUT_GPIO]);
		pr_emu_det(PARANOIC, "while EMU_SCI_OUT is %s\n",
					value ? "high" : "low");

/*	} else
	if (irq == emud->emu_irq[EMU_SCI_OUT_IRQ]) {

*/		value = gpio_get_value_cansleep(
					emud->emu_gpio[EMU_SCI_OUT_GPIO]);
		pr_emu_det(DEBUG, "EMU_SCI_OUT indicates ID %s grounded\n",
					value ? "" : "not");
		if (value) {
			set_bit(GRND_BIT, &lsense);
			clear_bit(FLOAT_BIT, &lsense);
		} else
			clear_bit(GRND_BIT, &lsense);

		value = gpio_get_value_cansleep(
					emud->emu_gpio[SEMU_PPD_DET_GPIO]);
		pr_emu_det(PARANOIC, "while SEMU_PPD_DET is %s\n",
					value ? "high" : "low");
/*
	} else if (irq == emud->emu_irq[PHY_USB_IRQ]) {
*/
		ulpi_reg = otg->io_ops->read(otg, ULPI_REG_READ(CHG_DET_CTRL));
		is_dcd_en = (ulpi_reg & 0x10);
		is_chgdet_en = (ulpi_reg & 0x3);

		/* read CHARGING_DETECTION_OUTPUT register */
		ulpi_reg = otg->io_ops->read(otg, ULPI_REG_READ(CHG_DET_OUT));
		if (ulpi_reg)
			pr_emu_det(DEBUG, "CHARGING_DETECTION_OUTPUT: 0x%x\n",
						ulpi_reg);
		if (is_dcd_en && (ulpi_reg & 2))
			set_bit(DCD_BIT, &lsense);

		if (is_chgdet_en && (ulpi_reg & 1))
			set_bit(CHG_DET_BIT, &lsense);
/*	}*/

	otgsc = readl(USB_OTGSC);
	portsc = readl(USB_PORTSC);

	pr_emu_det(DEBUG, "irq=%d, otgsc: 0x%08x, portsc: 0x%08x\n",
					irq, otgsc, portsc);

	if ((otgsc & (OTGSC_BSEIS | OTGSC_BSEIE)) &&
	    (otgsc & OTGSC_BSE))
		set_bit(B_SESSEND_BIT, &lsense);
	else
		clear_bit(B_SESSEND_BIT, &lsense);

	if ((otgsc & (OTGSC_BSVIS | OTGSC_BSVIE)) &&
	    (otgsc & OTGSC_BSV))
		set_bit(B_SESSVLD_BIT, &lsense);
	else
		clear_bit(B_SESSVLD_BIT, &lsense);

	if ((otgsc & (OTGSC_ASVIS | OTGSC_ASVIE)) &&
	    (otgsc & OTGSC_ASV))
		set_bit(SESS_VLD_BIT, &lsense);
	else
		clear_bit(SESS_VLD_BIT, &lsense);

	if ((otgsc & (OTGSC_AVVIS | OTGSC_AVVIE)) &&
	    (otgsc & OTGSC_AVV))
		set_bit(VBUS_VLD_BIT, &lsense);
	else
		clear_bit(VBUS_VLD_BIT, &lsense);

	if ((portsc & PORTSC_LS_SE1) == PORTSC_LS_SE1)
		set_bit(SE1_BIT, &lsense);
	else {
		clear_bit(SE1_BIT, &lsense);
		if (portsc & PORTSC_LS_DP)
			set_bit(DP_BIT, &lsense);
		else
			clear_bit(DP_BIT, &lsense);

		if (portsc & PORTSC_LS_DM)
			set_bit(DM_BIT, &lsense);
		else
			clear_bit(DM_BIT, &lsense);
	}

	if (do_adc)
		sense_emu_id(&lsense);

	atomic_set(&emud->last_irq, 0);
	emud->sense = lsense;
	pr_emu_det(STATUS, "%s\n", print_sense_bits(&lsense));

	return irq;
}

static int configure_hardware(enum emu_det_accy accy)
{
	struct emu_det_data *emud = the_emud;
	int retval = 0;

	switch (accy) {
	case ACCY_USB:
	case ACCY_FACTORY:
		mux_ctrl_mode(MUXMODE_USB);

		break;

	case ACCY_CHARGER:
		mux_ctrl_mode(MUXMODE_UART);

		break;

	case ACCY_WHISPER_PPD:
		if (emu_pdata && emu_pdata->enable_5v)	{
			emu_pdata->enable_5v(1);
			emud->ext_5v_switch_enabled = true;
			pr_emu_det(DEBUG, "5VS_OTG enabled\n");
		}
		mux_ctrl_mode(MUXMODE_UART);

		break;

	case ACCY_UNKNOWN:
		mux_ctrl_mode(MUXMODE_UART);

		emud->whisper_auth = AUTH_NOT_STARTED;

		break;

	case ACCY_WHISPER_SMART:
		mux_ctrl_mode(MUXMODE_USB);

		break;

	case ACCY_NONE:

		emud->whisper_auth = AUTH_NOT_STARTED;
		emu_id_protection_on();

		break;
	default:break;
	}

	if (retval != 0)
		retval = -EFAULT;

	return retval;
}

static void notify_otg(enum emu_det_accy accy)
{
	struct emu_det_data *data = the_emud;

	switch (accy) {
	case ACCY_NONE:
		atomic_notifier_call_chain(&data->trans->notifier,
					USB_EVENT_NONE, NULL);
			break;

	case ACCY_USB:
	case ACCY_FACTORY:
		atomic_notifier_call_chain(&data->trans->notifier,
					USB_EVENT_VBUS, NULL);
			break;

	case ACCY_CHARGER:
		atomic_notifier_call_chain(&data->trans->notifier,
					USB_EVENT_CHARGER, NULL);
			break;
	default:
			break;
	}
}

static void notify_accy(enum emu_det_accy accy)
{
	struct emu_det_data *data = the_emud;

	data->accy = accy;
	pr_emu_det(STATUS, "EMU detection Notify: %s\n", print_accy_name(accy));

	notify_otg(accy);

	/* Always go through None State */
	if (data->accy != ACCY_NONE) {
		blocking_notifier_call_chain(&emu_det_notifier_list,
					     (unsigned long)ACCY_NONE,
					     (void *)NULL);
	}

	configure_hardware(accy);

	blocking_notifier_call_chain(&emu_det_notifier_list,
				     (unsigned long)accy,
				     (void *)NULL);
}

static void notify_whisper_switch(enum emu_det_accy accy)
{
	struct emu_det_data *data = the_emud;

	if ((accy == ACCY_CHARGER) ||
		(accy == ACCY_WHISPER_PPD)) {
		switch_set_state(&data->wsdev, DOCKED);
	/* LD2 open->close */
	} else if ((accy == ACCY_WHISPER_SMART) &&
		   (data->state == WHISPER_SMART_LD2_CLOSE)) {

		if (data->whisper_auth == AUTH_PASSED)
			switch_set_state(&data->noauthdev, AUTH_NOT_REQUIRED);
		else
			switch_set_state(&data->noauthdev, AUTH_REQUIRED);

		switch_set_state(&data->dsdev, NO_DOCK);
		switch_set_state(&data->edsdev, NO_DOCK);
		switch_set_state(&data->wsdev, DOCKED);
		switch_set_state(&data->emusdev, NO_DEVICE);
		switch_set_state(&data->sdsdev, UNDOCKED);
		switch_set_state(&data->csdev, NORMAL);

	} else {
		switch_set_state(&data->wsdev, UNDOCKED);
		switch_set_state(&data->dsdev, NO_DOCK);
		switch_set_state(&data->edsdev, NO_DOCK);
		switch_set_state(&data->emusdev, NO_DEVICE);
		switch_set_state(&data->asdev, HC_NO_DEVICE);
		switch_set_state(&data->csdev, NORMAL);
		if (accy == ACCY_WHISPER_SMART)
			switch_set_state(&data->sdsdev, DOCKED);
		else {
			switch_set_state(&data->noauthdev, AUTH_REQUIRED);
			switch_set_state(&data->sdsdev, UNDOCKED);
			memset(data->dock_id, 0, CPCAP_WHISPER_ID_SIZE);
			memset(data->dock_prop, 0, CPCAP_WHISPER_PROP_SIZE);
		}
	}
}

#define POLL_TIME		(100 * HZ/1000) /* 100 msec */
#define MAX_RETRIES		6
#define CHG_PRI_DET_TIME	(40 * HZ/1000) /* TVDPSRC_ON */
#define CHG_SEC_DET_TIME	(40 * HZ/1000) /* TVDMSRC_ON */
#define WASTE_CYCLE_TIME	(50 * HZ/1000)

static void detection_work(bool caused_by_irq)
{
	struct emu_det_data *data = the_emud;
	int last_irq;
	bool   tmout;

	static unsigned long last_run;

	pr_emu_det(STATUS, "state %s, time since last run %d ms\n",
				print_state_name(data->state),
				jiffies_to_msecs(jiffies-last_run));
	/* store last run info */
	last_run = jiffies;

	switch (data->state) {
	case CONFIG:
		pm_runtime_get(data->trans->dev);
		data->trans->init(data->trans); /* reset PHY */
		phy_block_on();
		otgsc_enable();
		/*dpdm_disable();*/
		/*dcd_enable();*/
		chgdet_enable(0);

		configure_hardware(ACCY_UNKNOWN);
		emu_det_enable_irq(PHY_USB_IRQ);

		switch_set_state(&data->noauthdev, AUTH_REQUIRED);
		data->whisper_auth = AUTH_NOT_STARTED;
		data->retries = 0;
		/*data->state = SAMPLE_1;*/
		data->state = SAMPLE_2;

		queue_delayed_work(data->wq, &data->timer_work,
					msecs_to_jiffies(WASTE_CYCLE_TIME));
		break;

	case SAMPLE_1:
		last_irq = get_sense(true);

		tmout = ++data->retries == MAX_RETRIES;
		if (tmout)
			pr_emu_det(DEBUG, "DCD timed out\n");

		if (tmout || test_bit(DCD_BIT, &data->sense)) {
			dcd_disable();
			clear_bit(DCD_BIT, &data->sense);
			chgdet_enable(0);
			data->retries = 0;
			data->state = SAMPLE_2;
		}

		queue_delayed_work(data->wq, &data->timer_work,
					msecs_to_jiffies(POLL_TIME));
		break;

	case SAMPLE_2:
		data->prev_sense = data->sense;
		last_irq = get_sense(true);

		if (test_bit(CHG_DET_BIT, &data->sense)) {
			clear_bit(CHG_DET_BIT, &data->sense);
			chgdet_enable(0x8);
			pr_emu_det(DEBUG, "Primary CHG_DET-ed\n");
			data->retries = 0;
			data->state = CHG_DET;

			break;
		}

		chgdet_disable(0);

		if (data->sense != data->prev_sense) {
			pr_emu_det(DEBUG, "SAMPLE_2 keep waiting ...\n");
			data->state = SAMPLE_2;
		} else
			data->state = IDENTIFY;

		queue_delayed_work(data->wq, &data->timer_work, 0);
		break;

	case CHG_DET:
		data->prev_sense = data->sense;
		last_irq = get_sense(true);

		tmout = ++data->retries == MAX_RETRIES;
		if (tmout || test_bit(CHG_DET_BIT, &data->sense)) {
			chgdet_disable(0x8);
			data->state = IDENTIFY;
			pr_emu_det(DEBUG, "fall through to IDENTIFY ...\n");
		} else
			break;

	case IDENTIFY:
		last_irq = get_sense(true);
		data->state = CONFIG; /* re-arm in case of failed detection */

		if (SENSE_FACTORY) {
			data->state = FACTORY;
			notify_accy(ACCY_FACTORY);

		} else if (SENSE_USB) {
			data->state = USB;
			notify_accy(ACCY_USB);

		} else if (SENSE_CHARGER ||
			   SENSE_CHARGER_FLOAT ||
			   SENSE_WHISPER_SPD ||
			   SENSE_WHISPER_PPD ||
			   SENSE_WHISPER_CABLE) {
			pr_emu_det(STATUS,
					"detection_work: IW Identified\n");
			data->state = IDENTIFY_WHISPER_SEMU;
			queue_delayed_work(data->wq,
					&data->timer_work, 0);

		} else if (SENSE_WHISPER_SMART ||
			   SENSE_WHISPER_LD2) {
			pr_emu_det(STATUS,
					"detection_work: IW Identified\n");
			data->state = IDENTIFY_WHISPER_SMART;
			queue_delayed_work(data->wq,
					&data->timer_work, 0);
		} else if (test_bit(SESS_VLD_BIT, &data->sense) &&
			  (data->accy == ACCY_NONE)) {
			pr_emu_det(STATUS,
				"detection_work: PARTIAL INSERTION\n");
			data->state = CONFIG;

		} else {
			pr_emu_det(DEBUG, "no accessory\n");
			notify_accy(ACCY_NONE);
			notify_whisper_switch(ACCY_NONE);
		}

		if (test_bit(CHG_DET_BIT, &data->sense))
			clear_bit(CHG_DET_BIT, &data->sense);

		break;

	case IDENTIFY_WHISPER_SEMU:
		data->state = CONFIG;

		if (SENSE_CHARGER_FLOAT ||
		    SENSE_CHARGER ||
		    SENSE_WHISPER_SPD) {
			pr_emu_det(STATUS,
				"detection_work: CHARGER Identified\n");
			notify_accy(ACCY_CHARGER);
			data->state = CHARGER;
			notify_whisper_switch(ACCY_CHARGER);

		} else if (SENSE_WHISPER_PPD ||
			   SENSE_WHISPER_PPD_MASK ||
			   SENSE_WHISPER_CABLE) {
			pr_emu_det(STATUS,
				"detection_work: PPD Identified\n");
			notify_accy(ACCY_WHISPER_PPD);
			data->state = WHISPER_PPD;
			notify_whisper_switch(ACCY_WHISPER_PPD);

		} else {
			queue_delayed_work(data->wq, &data->timer_work, 0);
		}
		break;

	case IDENTIFY_WHISPER_SMART:
		data->state = CONFIG;
		if (SENSE_WHISPER_SMART || SENSE_WHISPER_LD2) {
			pr_emu_det(STATUS,
				"detection_work: SMART Identified\n");
			notify_accy(ACCY_USB_DEVICE);
			if (SENSE_WHISPER_SMART)
				data->state = WHISPER_SMART;
			else {
				switch_set_state(&data->noauthdev,
						AUTH_REQUIRED);
				data->state = WHISPER_SMART_LD2_OPEN;
			}
			notify_whisper_switch(ACCY_WHISPER_SMART);
		} else
			queue_delayed_work(data->wq, &data->timer_work, 0);
		break;

	case USB:
		last_irq = get_sense(true);

		if (!test_bit(SESS_VLD_BIT, &data->sense)) {
			data->state = CONFIG;
			queue_delayed_work(data->wq, &data->timer_work, 0);
		}
		break;

	case FACTORY:
		last_irq = get_sense(true);

		if (!test_bit(SESS_VLD_BIT, &data->sense)) {
			data->state = CONFIG;
			queue_delayed_work(data->wq, &data->timer_work, 0);
		}
		break;

	case CHARGER:
		last_irq = get_sense(true);

		if (!test_bit(SESS_VLD_BIT, &data->sense)) {
			data->state = CONFIG;
			queue_delayed_work(data->wq, &data->timer_work, 0);
		}
		break;

	case WHISPER_PPD:
		pr_emu_det(STATUS, "detection_work: PPD\n");
		last_irq = get_sense(true);

		if (1) {
			if ((data->whisper_auth == AUTH_NOT_STARTED) ||
				(data->whisper_auth == AUTH_FAILED)) {
				pr_emu_det(STATUS,
					"detection_work: Set None\n");
				notify_accy(ACCY_NONE);
				notify_whisper_switch(ACCY_NONE);
			} else if (data->whisper_auth == AUTH_IN_PROGRESS) {
				pr_emu_det(STATUS,
					"detection_work: Delay\n");
				msleep(5000);
				if (data->whisper_auth != AUTH_PASSED) {
					notify_accy(ACCY_NONE);
					notify_whisper_switch(ACCY_NONE);
				}
			}
			if (data->whisper_auth == AUTH_PASSED) {
				data->state = IDENTIFY_WHISPER_SEMU;
				queue_delayed_work(data->wq,
						&data->timer_work, 0);
			} else {
				data->state = WHISPER_PPD_UNKNOWN;
			}
		} else if (test_bit(FLOAT_BIT, &data->sense)) {
			data->state = CONFIG;
			queue_delayed_work(data->wq, &data->timer_work, 0);
		} else if (test_bit(SESS_VLD_BIT, &data->sense)) {
			data->state = IDENTIFY_WHISPER_SEMU;
			queue_delayed_work(data->wq, &data->timer_work, 0);
		} else {
			if (data->whisper_auth == AUTH_FAILED &&
			    data->accy == ACCY_WHISPER_PPD) {
				notify_accy(ACCY_NONE);
			}

			if ((last_irq == EMU_SCI_OUT_IRQ) &&
			    (data->whisper_auth == AUTH_PASSED)) {
				/*whisper_audio_check(data); temporary out */
			}
		}
		break;

	case WHISPER_SMART:
		pr_emu_det(STATUS, "detection_work: SMART\n");
		last_irq = get_sense(true);

		if (data->whisper_auth == AUTH_FAILED &&
		    data->accy == ACCY_USB_DEVICE &&
		    test_bit(SESS_VLD_BIT, &data->sense)) {
			notify_accy(ACCY_CHARGER);
		}

		if (!test_bit(SESS_VLD_BIT, &data->sense)) {
			data->state = CONFIG;
			queue_delayed_work(data->wq, &data->timer_work, 0);
		} else
			data->state = WHISPER_SMART;
		break;

	case USB_ADAPTER:
		last_irq = get_sense(true);

		if (0)
			notify_accy(ACCY_USB_DEVICE);
		else {
			data->state = CONFIG;
			queue_delayed_work(data->wq, &data->timer_work, 0);
		}
		break;

	case WHISPER_SMART_LD2_OPEN:
		pr_emu_det(STATUS, "detection_work: SMART_LD2_OPEN\n");
		last_irq = get_sense(true);

		if (data->whisper_auth == AUTH_FAILED &&
			data->accy == ACCY_USB_DEVICE &&
			!test_bit(SESS_VLD_BIT, &data->sense)) {
			notify_accy(ACCY_CHARGER);
		}

		if (!test_bit(SESS_VLD_BIT, &data->sense)) {
			data->state = CONFIG;
			queue_delayed_work(data->wq, &data->timer_work, 0);
		} else if ((test_bit(SESS_VLD_BIT, &data->sense) &&
				(last_irq == EMU_SCI_OUT_IRQ) &&
				test_bit(GRND_BIT, &data->sense)) ||
			(test_bit(SESS_VLD_BIT, &data->sense) &&
				(last_irq == EMU_SCI_OUT_IRQ) &&
				!test_bit(GRND_BIT, &data->sense) &&
				test_bit(SE1_BIT, &data->sense))) {
			pr_emu_det(STATUS,
				"detection_work: LD2 lid closed\n");
			notify_accy(ACCY_CHARGER);
			data->state = WHISPER_SMART_LD2_CLOSE;
			notify_whisper_switch(ACCY_WHISPER_SMART);
		} else {
			data->state = WHISPER_SMART_LD2_OPEN;
		}
		break;

	case WHISPER_SMART_LD2_CLOSE:
		pr_emu_det(STATUS, "detection_work: SMART_LD2_CLOSE\n");
		last_irq = get_sense(true);

		if (!test_bit(SESS_VLD_BIT, &data->sense)) {
			data->state = CONFIG;
			queue_delayed_work(data->wq, &data->timer_work, 0);
		} else if (test_bit(SESS_VLD_BIT, &data->sense) &&
			(last_irq == EMU_SCI_OUT_IRQ) &&
			test_bit(GRND_BIT, &data->sense)) {
			pr_emu_det(STATUS,
					"detection_work: LD2 lid opened\n");
			data->state = WHISPER_SMART_LD2_OPEN;
			notify_whisper_switch(ACCY_WHISPER_SMART);
			if (data->whisper_auth == AUTH_PASSED)
				switch_set_state(&data->edsdev, MOBILE_DOCK);
			notify_accy(ACCY_USB_DEVICE);
		} else
			data->state = WHISPER_SMART_LD2_CLOSE;
		break;

	default:
		return;
	}
}

static irqreturn_t emu_det_irq_handler(int irq, void *data)
{
	struct emu_det_data *emud = data;
	u32 usbsts, otgsc;
	static u32 count;

/* in case we ever care about LPM
	if (atomic_read(&data->in_lpm)) {
		struct otg_transceiver *otg = &emud->trans->otg;

		disable_irq_nosync(irq);
		data->async_int = 1;
		pm_runtime_get(otg->dev);
		pr_emu_det(DEBUG, "IRQ_HANDLED (LPM)\n");
		return IRQ_HANDLED;
	}
*/
	usbsts = readl(USB_USBSTS);
	otgsc = readl(USB_OTGSC);

	if (irq == emud->emu_irq[PHY_USB_IRQ]) {

		if (!(usbsts & PHY_ALT_INT)) {
			count++;
			return IRQ_NONE;
		} else {
			struct otg_transceiver *otg = emud->trans;
			u8 ulpi_reg;

			writel(PHY_ALT_INT, USB_USBSTS);

			ulpi_reg = otg->io_ops->read(otg,
					ULPI_REG_READ(ALT_INT_LATCH));
			if (ulpi_reg) { /* clear ALT_INT_LATCH */
				otg->io_ops->write(otg, ulpi_reg,
					ULPI_REG_SET(ALT_INT_LATCH));
				pr_emu_det(DEBUG, "ALT_INTERRUPT_LATCH:"
						  " 0x%02x\n", ulpi_reg);
			}

			goto put_on_schedule;
		}

		if (!(otgsc & emud->otgsc_mask)) {
			count++;
			return IRQ_NONE;
		}

		writel(otgsc, USB_OTGSC);
	}

	if ((irq == emud->emu_irq[SEMU_PPD_DET_IRQ]) &&
	    (debug_mask & PRINT_PARANOIC)) {
		int value;
		value = gpio_get_value_cansleep(
				emud->emu_gpio[SEMU_PPD_DET_GPIO]);
		pr_emu_det(PARANOIC, "SEMU_PPD_DET is %s\n",
					value ? "high" : "low");
	}

put_on_schedule:

	atomic_set(&emud->last_irq, irq);

	pr_emu_det(DEBUG, "irq=%d(%d), usbsts: 0x%08x, otgsc: 0x%08x\n",
				irq, count, usbsts, otgsc);
	count = 0;
	queue_delayed_work(emud->wq, &emud->irq_work, 0);

	return IRQ_HANDLED;
}

static struct pm_gpio pm_gpio_config_data[] = {
	{	/* PM GPIO 35 - SEMU_PPD_DET */
		.direction	= PM_GPIO_DIR_IN,
		.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
		.output_value	= 0,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_L17,
		.out_strength	= PM_GPIO_STRENGTH_LOW,
		.function	= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	= 0,
		.disable_pin	= 0,
	},
	{	/* PM GPIO 17 - SEMU_ALT_MODE_EN */
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
		.output_value	= 0,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_L17,
		.out_strength	= PM_GPIO_STRENGTH_LOW,
		.function	= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	= 0,
		.disable_pin	= 1, /* high impedance */
	},
};

static int pmic_gpio_initf(int idx, int base)
{
	struct emu_det_data *emud = the_emud;
	return pm8xxx_gpio_config(emud->emu_gpio[idx],
				&pm_gpio_config_data[idx-base]);
}

static struct pm8xxx_mpp_config_data pm_mpp_config_data[] = {
	{	/* PMIC MPP 10 - EMU_ID */
		.type		= PM8XXX_MPP_TYPE_A_INPUT,
		.level		= PM8XXX_MPP_AIN_AMUX_CH7,
		.control	= PM8XXX_MPP_DOUT_CTRL_LOW,
	},
	{	/* PMIC MPP 9 - EMU_ID_EN */
		.type		= PM8XXX_MPP_TYPE_D_INPUT,
		.level		= PM8921_MPP_DIG_LEVEL_L17,
		.control	= PM8XXX_MPP_DIN_TO_INT,
	},
};

static int pmic_mpp_initf(int idx, int base)
{
	struct emu_det_data *emud = the_emud;
	return pm8xxx_mpp_config(emud->emu_gpio[idx],
				&pm_mpp_config_data[idx-base]);
}

struct emu_det_irq_init_data {
	unsigned int	idx;
	char		*name;
	unsigned long	flags;
	irqreturn_t	(*handler)(int, void *);
};

#define EMU_DET_IRQ(_id, _flags, _handler) \
{ \
	.idx		= _id, \
	.name		= #_id, \
	.flags		= _flags, \
	.handler	= _handler, \
}
static struct emu_det_irq_init_data emu_det_irq_data[] = {
	EMU_DET_IRQ(SEMU_PPD_DET_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
			| IRQF_DISABLED, emu_det_irq_handler),
	EMU_DET_IRQ(EMU_SCI_OUT_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
			| IRQF_DISABLED, emu_det_irq_handler),
	EMU_DET_IRQ(PHY_USB_IRQ, IRQF_SHARED, emu_det_irq_handler),
};

static void free_irqs(void)
{
	struct emu_det_data *emud = the_emud;
	int i;

	for (i = 0; i < EMU_DET_MAX_INTS; i++)
		if (emud->emu_irq[i]) {
			free_irq(emud->emu_irq[i], emud);
			pr_emu_det(DEBUG, "free-ed irq %d\n", emud->emu_irq[i]);
			emud->emu_irq[i] = 0;
		}
}

static int __devinit request_irqs(void)
{
	struct emu_det_data *emud = the_emud;
	int ret, i;

	ret = 0;
	bitmap_fill(emud->enabled_irqs, EMU_DET_MAX_INTS);

	for (i = 0; i < ARRAY_SIZE(emu_det_irq_data); i++) {
		pr_emu_det(DEBUG, "requesting irq[%d] %s-%d\n",
				emu_det_irq_data[i].idx,
				emu_det_irq_data[i].name,
				emud->emu_irq[emu_det_irq_data[i].idx]);
		ret = request_irq(emud->emu_irq[emu_det_irq_data[i].idx],
			emu_det_irq_data[i].handler,
			emu_det_irq_data[i].flags,
			emu_det_irq_data[i].name, emud);
		if (ret < 0) {
			pr_err("couldn't request %d (%s) %d\n",
				emud->emu_irq[emu_det_irq_data[i].idx],
				emu_det_irq_data[i].name, ret);
			goto err_out;
		}
		if (!(emu_det_irq_data[i].flags & IRQF_SHARED))
			emu_det_disable_irq(
				emud->emu_irq[emu_det_irq_data[i].idx]);
	}
	return 0;

err_out:
	free_irqs();
	return -EINVAL;
}

static struct gpiomux_setting emu_id_en_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting emu_sci_out_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mux_ctrl0_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting mux_ctrl1_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm_gpio_configs[] = {
	{
		.gpio = 0,
		.settings = {
			[GPIOMUX_SUSPENDED] = &emu_sci_out_config,
			[GPIOMUX_ACTIVE] = &emu_sci_out_config,
		}
	},
	{
		.gpio = 0,
		.settings = {
			[GPIOMUX_SUSPENDED] = &emu_id_en_config,
			[GPIOMUX_ACTIVE] = &emu_id_en_config,
		}
	},
	{
		.gpio = 0,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mux_ctrl0_config,
			[GPIOMUX_ACTIVE] = &mux_ctrl0_config,
		}
	},
	{
		.gpio = 0,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mux_ctrl1_config,
			[GPIOMUX_ACTIVE] = &mux_ctrl1_config,
		}
	},
};

static int msm_gpio_initf(int idx, int base)
{
	struct emu_det_data *emud = the_emud;
	msm_gpio_configs[idx].gpio = emud->emu_gpio[idx];
	msm_gpiomux_install(&msm_gpio_configs[idx], 1);
	return 0;
}

static void common_gpio_initf(unsigned gpio, int idx, int base)
{
	int rc;
	if (gpio < NR_MSM_GPIOS)
		rc = msm_gpio_initf(idx, base);
	else {
		gpio -= NR_MSM_GPIOS;
		if (gpio < PM8921_NR_GPIOS)
			rc = pmic_gpio_initf(idx, base);
		else
			rc = pmic_mpp_initf(idx, base);
	}
	if (rc)
		pr_emu_det(ERROR, "gpio config function failed rc=%d\n", rc);
}

struct emu_det_gpio_init_data {
	int	idx;
	int	base;
	char	*name;
	int	direction;
	int	value;
	void	(*init)(unsigned, int, int);
};

#define SKIP		0
#define DIR_INPUT	1
#define DIR_OUTPUT	2

#define DECLARE_GPIO(_id, _base, _dir, _val, _initf) \
{ \
	.idx		= _id, \
	.base		= _base, \
	.name		= #_id, \
	.direction	= _dir, \
	.value		= _val, \
	.init		= _initf, \
}
/*
 * GPIOs that belong to PMIC need to be consistent with config setup in PMIC
 *	see pm_gpio_config_data[] and pm_mpp_config_data[] above
 */
struct emu_det_gpio_init_data emu_det_gpio_data[] = {
	DECLARE_GPIO(EMU_SCI_OUT_GPIO,   MSMGPIO_BASE, DIR_INPUT, \
				-1, common_gpio_initf),
	DECLARE_GPIO(EMU_ID_EN_GPIO,     MSMGPIO_BASE, DIR_INPUT, \
				-1, common_gpio_initf), /* machine dependent */
	DECLARE_GPIO(EMU_MUX_CTRL0_GPIO, MSMGPIO_BASE, DIR_OUTPUT, \
				1, common_gpio_initf),
	DECLARE_GPIO(EMU_MUX_CTRL1_GPIO, MSMGPIO_BASE, DIR_OUTPUT, \
				0, common_gpio_initf),
	DECLARE_GPIO(SEMU_PPD_DET_GPIO,  PMGPIO_BASE,  DIR_INPUT, \
				-1, common_gpio_initf),
	DECLARE_GPIO(SEMU_ALT_MODE_EN_GPIO, PMGPIO_BASE, SKIP, \
				-1, common_gpio_initf),
	DECLARE_GPIO(EMU_ID_GPIO,        PMMPP_BASE,   SKIP, \
				-1, common_gpio_initf),
};

static void free_gpios(void)
{
	struct emu_det_data *emud = the_emud;
	int i;

	for (i = 0; i < EMU_DET_MAX_GPIOS; i++)
		if (emud->emu_gpio[i]) {
			gpio_free(emud->emu_gpio[i]);
			pr_emu_det(DEBUG, "free-ed gpio %d\n",
						emud->emu_gpio[i]);
			emud->emu_gpio[i] = 0;
		}
}

#define GPIO_IN_OUT(gpio)	(MSM_TLMM_BASE + 0x1004 + (0x10 * (gpio)))

static int __devinit request_gpios(struct platform_device *pdev)
{
	struct emu_det_data *emud = the_emud;
	struct resource *res;
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(emu_det_gpio_data); i++) {
		res = platform_get_resource_byname(pdev, IORESOURCE_IO,
				emu_det_gpio_data[i].name);
		if (res == NULL) {
			pr_err("couldn't find %s\n",
				emu_det_gpio_data[i].name);
			goto err_out;
		}

		emud->emu_gpio[emu_det_gpio_data[i].idx] = res->start;

		pr_emu_det(DEBUG, "requesting gpio[%d] %s-%d\n",
				emu_det_gpio_data[i].idx,
				emu_det_gpio_data[i].name,
				emud->emu_gpio[emu_det_gpio_data[i].idx]);

		ret = gpio_request(emud->emu_gpio[emu_det_gpio_data[i].idx],
				emu_det_gpio_data[i].name);
		if (ret) {
			pr_err("failed to request gpio %s\n",
				emu_det_gpio_data[i].name);
			goto err_out;
		}

		if (emu_det_gpio_data[i].init) {

			emu_det_gpio_data[i].init(
				emud->emu_gpio[emu_det_gpio_data[i].idx],
				emu_det_gpio_data[i].idx,
				emu_det_gpio_data[i].base);
			pr_emu_det(DEBUG, "gpio %d has init function\n",
				emud->emu_gpio[emu_det_gpio_data[i].idx]);
		}

		if (emu_det_gpio_data[i].direction == DIR_OUTPUT) {

			ret = gpio_direction_output(
				emud->emu_gpio[emu_det_gpio_data[i].idx], 0);
			pr_emu_det(DEBUG, "gpio %d is OUT\n",
				emud->emu_gpio[emu_det_gpio_data[i].idx]);

			if (emu_det_gpio_data[i].value != -1) {

				gpio_set_value(
				emud->emu_gpio[emu_det_gpio_data[i].idx],
					emu_det_gpio_data[i].value);
				pr_emu_det(DEBUG, "gpio %d value set to %d\n",
				emud->emu_gpio[emu_det_gpio_data[i].idx],
					emu_det_gpio_data[i].value);
			}

		} else if (emu_det_gpio_data[i].direction == DIR_INPUT) {

			ret = gpio_direction_input(
				emud->emu_gpio[emu_det_gpio_data[i].idx]);
			pr_emu_det(DEBUG, "gpio %d is IN\n",
				emud->emu_gpio[emu_det_gpio_data[i].idx]);
		}

	}
	return 0;

err_out:
	free_gpios();
	return -EINVAL;
}

#ifdef CONFIG_DEBUG_FS
static int accy_read(void *data, u64 *val)
{
	struct emu_det_data *emud = the_emud;
	*val = emud->accy;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(accy_debugfs_fops, accy_read, NULL, "%lld\n");

static int emu_id_adc_read(void *data, u64 *val)
{
	*val = adc_emu_id_get();
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(emu_id_debugfs_fops, emu_id_adc_read, NULL, "%lld\n");

static int vbus_adc_read(void *data, u64 *val)
{
	*val = adc_vbus_get();
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(vbus_debugfs_fops, vbus_adc_read, NULL, "%lld\n");

static int detection_set(void *data, u64 val)
{
	struct emu_det_data *emud = the_emud;
	queue_work(emud->wq, &(emud->irq_work.work));
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(detection_debugfs_fops, NULL, detection_set, "%lld\n");

static int semu_ppd_det_get(void *data, u64 *val)
{
	struct emu_det_data *emud = the_emud;
	*val = gpio_get_value_cansleep(emud->emu_gpio[SEMU_PPD_DET_GPIO]);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(semu_ppd_det_debugfs_fops, semu_ppd_det_get, \
						NULL, "%lld\n");

static int emu_id_en_get(void *data, u64 *val)
{
	struct emu_det_data *emud = the_emud;
	*val = gpio_get_value_cansleep(emud->emu_gpio[EMU_ID_EN_GPIO]);
	return 0;
}

static int emu_id_en_set(void *data, u64 val)
{
	val = !!val;
	if (val)
		emu_id_protection_on();
	else
		emu_id_protection_off();
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(emu_id_en_debugfs_fops, emu_id_en_get, \
						emu_id_en_set, "%lld\n");

static int debug_mask_set(void *data, u64 val)
{
	debug_mask = val;
	return 0;
}

static int debug_mask_get(void *data, u64 *val)
{
	*val = debug_mask;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(debug_mask_debugfs_fops, debug_mask_get, \
						debug_mask_set, "%llx\n");

static void create_debugfs_entries(void)
{
	struct dentry *dent = debugfs_create_dir("emu_det", NULL);

	if (IS_ERR(dent)) {
		pr_err("EMU det debugfs dir not created\n");
		return;
	}

	debugfs_create_file("emu_id", 0444, dent,
			    (void *)NULL, &emu_id_debugfs_fops);
	debugfs_create_file("vbus", 0444, dent,
			    (void *)NULL, &vbus_debugfs_fops);
	debugfs_create_file("detection", 0222, dent,
			    (void *)NULL, &detection_debugfs_fops);
	debugfs_create_file("accy", 0444, dent,
			    (void *)NULL, &accy_debugfs_fops);
	debugfs_create_file("semu_ppd_det", 0444, dent,
			    (void *)NULL, &semu_ppd_det_debugfs_fops);
	debugfs_create_file("emu_id_en", 0644, dent,
			    (void *)NULL, &emu_id_en_debugfs_fops);
	debugfs_create_file("debug", 0644, dent,
			    (void *)NULL, &debug_mask_debugfs_fops);
}
#else
static inline void create_debugfs_entries(void)
{
}
#endif

static void emu_det_vbus_state(int online)
{
	struct emu_det_data *emud = the_emud;
/*
	if (!atomic_read(&emud->in_lpm)) {
		pr_emu_det(DEBUG, "LPM\n");
		return;
	}
*/
	pr_emu_det(DEBUG, "PM8921 USBIN callback: %s\n",
					online ? "in" : "out");
	emud->usb_present = online;
/* causes sleeping function call from interrupt handling context
	due to pm8xxx_mpp_config
	if (!online)
		emu_id_protection_on();
*/
	queue_delayed_work(emud->wq, &emud->irq_work,
				msecs_to_jiffies(50));
}

static void timer_work(struct work_struct *work)
{
	detection_work(false);
}

static void irq_work(struct work_struct *work)
{
	detection_work(true);
}

static long emu_det_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	struct emu_det_data *data = the_emud;
	int retval = -EINVAL;
	int dock = NO_DOCK;
	struct cpcap_whisper_request request;

	switch (cmd) {
	case CPCAP_IOCTL_ACCY_WHISPER:
		if (copy_from_user((void *) &request, (void *) arg,
				   sizeof(request)))
			return -EFAULT;
		request.dock_id[CPCAP_WHISPER_ID_SIZE - 1] = '\0';
		request.dock_prop[CPCAP_WHISPER_PROP_SIZE - 1] = '\0';

		pr_emu_det(STATUS, "ioctl cmd = 0x%04x\n", request.cmd);

		if ((data->state == CHARGER) || (data->state == WHISPER_PPD)) {
			if (request.cmd & CPCAP_WHISPER_ENABLE_UART) {
				data->whisper_auth = AUTH_IN_PROGRESS;
				mux_ctrl_mode(MUXMODE_UART);
			}

			if (request.cmd & CPCAP_WHISPER_MODE_PU) {
				alternate_mode_enable();
				data->whisper_auth = AUTH_PASSED;
			} else if (!(request.cmd & CPCAP_WHISPER_ENABLE_UART))
				data->whisper_auth = AUTH_FAILED;

			/* Report dock type to system */
			dock = (request.cmd & CPCAP_WHISPER_ACCY_MASK) >>
				CPCAP_WHISPER_ACCY_SHFT;
			if (dock == LE_DOCK)
				switch_set_state(&data->dsdev, LE_DOCK);
			else
				switch_set_state(&data->dsdev, dock);
			switch_set_state(&data->edsdev, dock);

			pr_emu_det(STATUS, "Whisper_auth =%d\n",
							data->whisper_auth);

			if (!(request.cmd & CPCAP_WHISPER_ENABLE_UART)) {
				mux_ctrl_mode(MUXMODE_USB);
				standard_mode_enable();
				if (dock && (strlen(request.dock_id) <
					CPCAP_WHISPER_ID_SIZE))
					strncpy(data->dock_id,
						request.dock_id,
						CPCAP_WHISPER_ID_SIZE);
				if (dock && (strlen(request.dock_prop) <
					CPCAP_WHISPER_PROP_SIZE))
					strncpy(data->dock_prop,
						request.dock_prop,
						CPCAP_WHISPER_PROP_SIZE);
				/*whisper_audio_check(data); temporary out */
			}

		} else if ((data->state == WHISPER_SMART) ||
			(data->state == WHISPER_SMART_LD2_OPEN)) {
			/* Report dock type to system */
			dock = (request.cmd & CPCAP_WHISPER_ACCY_MASK) >>
						CPCAP_WHISPER_ACCY_SHFT;
			if (dock &&
			   (strlen(request.dock_id) < CPCAP_WHISPER_ID_SIZE))
				strncpy(data->dock_id, request.dock_id,
						CPCAP_WHISPER_ID_SIZE);
			if (dock &&
			   (strlen(request.dock_prop) <
				CPCAP_WHISPER_PROP_SIZE))
				strncpy(data->dock_prop, request.dock_prop,
						CPCAP_WHISPER_PROP_SIZE);
			switch (dock) {
			case HD_DOCK:
				switch_set_state(&data->dsdev, DESK_DOCK);
				break;
			case HE_DOCK:
				switch_set_state(&data->dsdev, HE_DOCK);
				switch_set_state(&data->csdev, HIGH);
				break;
			case MOBILE_DOCK:
			default:
				switch_set_state(&data->dsdev, NO_DOCK);
				break;
			}
			switch_set_state(&data->edsdev, dock);

			if (request.cmd & 0x01)
				data->whisper_auth = AUTH_PASSED;
			retval = 0;
		}
		break;

	default:
		retval = -ENOTTY;
		break;
	}
	return retval;
}

static const struct file_operations emu_det_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = emu_det_ioctl,
};

static struct miscdevice emu_det_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= CPCAP_DEV_NAME,
	.fops	= &emu_det_fops,
};

static int emu_det_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	struct emu_det_data *data;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);
	the_emud = data;
	emu_pdata = pdev->dev.platform_data;
	data->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("failed to get platform resource mem\n");
		ret = -ENODEV;
		goto free_data;
	}

	data->regs = ioremap(res->start, resource_size(res));
	if (!data->regs) {
		pr_err("ioremap failed\n");
		ret = -ENOMEM;
		goto free_data;
	}

	data->trans = otg_get_transceiver();
	if (!data->trans || !data->trans->io_ops ||
		!data->trans->init) {
		pr_err("failed to obtain OTG transiver\n");
		ret = -ENODEV;
		goto free_data;
	}

	data->emu_irq[PHY_USB_IRQ] = platform_get_irq(pdev, 0);
	if (!data->emu_irq[PHY_USB_IRQ]) {
		pr_err("failed to get PHY_USB_IRQ\n");
		ret = -ENODEV;
		goto free_data;
	}

	if (emu_pdata && emu_pdata->core_power)
		emu_pdata->core_power(1);

	ret = request_gpios(pdev);
	if (ret) {
		pr_err("couldn't register gpios rc=%d\n", ret);
		goto free_data;
	}

	pr_emu_det(DEBUG, "done with gpios\n");

	ret = misc_register(&emu_det_dev);
	if (ret) {
		pr_err("couldn't register device %s\n",
			emu_det_dev.name);
		goto free_misc_dev;
	}

	INIT_DELAYED_WORK(&data->timer_work, timer_work);
	INIT_DELAYED_WORK(&data->irq_work, irq_work);

	wake_lock_init(&data->wake_lock, WAKE_LOCK_SUSPEND, "emu_det");

	data->wq = create_singlethread_workqueue("emu_det");

	/* have to hard code PM GPIO # for SEMU_PPD_DET, since devices-8960.c
		already have GPIO aligned with PM8921_GPIO_PM_TO_SYS */
	data->emu_irq[SEMU_PPD_DET_IRQ] = PM8921_GPIO_IRQ(PM8921_IRQ_BASE, 35);
	data->emu_irq[EMU_SCI_OUT_IRQ] = MSM_GPIO_TO_INT(
				data->emu_gpio[EMU_SCI_OUT_GPIO]);
	ret = request_irqs();
	if (ret) {
		pr_err("couldn't register interrupts rc=%d\n", ret);
		goto free_gpios;
	}
	pr_emu_det(DEBUG, "done with irqs\n");

	pm8921_charger_register_vbus_sn(&emu_det_vbus_state);
	pr_emu_det(DEBUG, "registered callback with PM8921\n");

	data->wsdev.name = "whisper";
	ret = switch_dev_register(&data->wsdev);
	if (ret)
		pr_err("couldn't register switch (%s) rc=%d\n",
					data->wsdev.name, ret);
	data->dsdev.name = "dock";
	data->dsdev.print_name = dock_print_name;
	ret = switch_dev_register(&data->dsdev);
	if (ret)
		pr_err("couldn't register switch (%s) rc=%d\n",
					data->dsdev.name, ret);
	data->emusdev.name = "emuconn";
	data->emusdev.print_name = emu_audio_print_name;
	ret = switch_dev_register(&data->emusdev);
	if (ret)
		pr_err("couldn't register switch (%s) rc=%d\n",
					data->emusdev.name, ret);
	data->asdev.name = "usb_audio";
	ret = switch_dev_register(&data->asdev);
	if (ret)
		pr_err("couldn't register switch (%s) rc=%d\n",
					data->asdev.name, ret);
	data->edsdev.name = "extdock";
	data->edsdev.print_name = dock_print_name;
	ret = switch_dev_register(&data->edsdev);
	if (ret)
		pr_err("couldn't register switch (%s) rc=%d\n",
					data->edsdev.name, ret);
	data->sdsdev.name = "smartdock";
	ret = switch_dev_register(&data->sdsdev);
	if (ret)
		pr_err("couldn't register switch (%s) rc=%d\n",
					data->sdsdev.name, ret);
	data->csdev.name = "charge_capability";
	ret = switch_dev_register(&data->csdev);
	if (ret)
		pr_err("couldn't register switch (%s) rc=%d\n",
					data->csdev.name, ret);
	data->noauthdev.name = "noauth";
	ret = switch_dev_register(&data->noauthdev);
	if (ret)
		pr_err("couldn't register switch (%s) rc=%d\n",
					data->noauthdev.name, ret);
	data->state = CONFIG;
	data->power_up = 1;
	data->semu_alt_mode = false;
	data->accy = ACCY_NONE;
	data->undetect_cnt = 0;
	data->bpass_mod = 'a';
	data->whisper_auth = AUTH_NOT_STARTED;
	dev_set_drvdata(&pdev->dev, data);

	create_debugfs_entries();
	pr_emu_det(DEBUG, "EMU detection driver started\n");

	/*
	   PMIC always kicks off detection work regardless whether
	   anything is connected to USB or not, thus no need to schedule
	*/

	return 0;

free_misc_dev:
	misc_deregister(&emu_det_dev);
free_gpios:
	free_gpios();
free_data:
	kfree(data);
	return ret;
}

static int __exit emu_det_remove(struct platform_device *pdev)
{
	struct emu_det_data *data = the_emud;

	wake_lock_destroy(&data->wake_lock);
	configure_hardware(ACCY_NONE);
	cancel_delayed_work_sync(&data->timer_work);
	cancel_delayed_work_sync(&data->irq_work);
	destroy_workqueue(data->wq);
/*
	if ((data->accy != ACCY_NONE) && (data->usb_dev != NULL))
		platform_device_del(data->usb_dev);
*/
	switch_dev_unregister(&data->wsdev);
	switch_dev_unregister(&data->dsdev);
	switch_dev_unregister(&data->emusdev);
	switch_dev_unregister(&data->asdev);
	switch_dev_unregister(&data->edsdev);
	switch_dev_unregister(&data->sdsdev);
	switch_dev_unregister(&data->csdev);

	pm8921_charger_unregister_vbus_sn(0);
	free_irqs();
	free_gpios();

	misc_deregister(&emu_det_dev);
	if (emu_pdata && emu_pdata->core_power)
		emu_pdata->core_power(0);
	kfree(data);

	return 0;
}

static struct platform_driver emu_det_driver = {
	.probe		= emu_det_probe,
	.remove		= __exit_p(emu_det_remove),
	.suspend	= NULL,
	.driver		= {
		.name	= "emu_det",
		.owner	= THIS_MODULE,
	},
};

static int __init emu_det_init(void)
{
	return platform_driver_register(&emu_det_driver);
}

static void __exit emu_det_exit(void)
{
	platform_driver_unregister(&emu_det_driver);
}

late_initcall(emu_det_init);
module_exit(emu_det_exit);

MODULE_ALIAS("platform:emu_det");
MODULE_DESCRIPTION("Motorola MSM8960 EMU detection driver");
MODULE_LICENSE("GPL");

