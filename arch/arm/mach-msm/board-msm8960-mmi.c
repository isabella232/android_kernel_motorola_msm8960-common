/* Copyright (c) 2011, Motorola Mobility. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/i2c/isl9519.h>
#include <linux/gpio.h>
#include <linux/leds-pwm-gpio.h>
#include <linux/msm_ssbi.h>
#include <linux/regulator/gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/slimbus/slimbus.h>
#include <linux/bootmem.h>
#include <linux/msm_kgsl.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_CYTTSP3
#include <linux/input/cyttsp3_core.h>
#endif
#ifdef CONFIG_TOUCHSCREEN_MELFAS100_TS
#include <linux/melfas100_ts.h>
#endif
#ifdef CONFIG_TOUCHSCREEN_ATMXT
#include <linux/input/atmxt.h>
#endif

#include <linux/dma-mapping.h>
#include <linux/platform_data/qcom_crypto_device.h>
#include <linux/platform_data/qcom_wcnss_device.h>
#include <linux/leds.h>
#include <linux/leds-pm8xxx.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/msm_tsens.h>
#include <linux/ks8851.h>
#include <linux/gpio_event.h>
#include <linux/of_fdt.h>
#include <linux/of.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/hardware/gic.h>
#include <asm/mach/mmc.h>
#include <linux/usb/android.h>
#ifdef CONFIG_USB_ANDROID_DIAG
#include <mach/usbdiag.h>
#endif

#include <mach/mmi_emu_det.h>
#include <mach/msm_iomap.h>
#include <mach/msm_spi.h>
#ifdef CONFIG_USB_MSM_OTG_72K
#include <mach/msm_hsusb.h>
#else
#include <linux/usb/msm_hsusb.h>
#endif
#include <linux/usb/android.h>
#include <mach/usbdiag.h>
#include <mach/socinfo.h>
#include <mach/rpm.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/dma.h>
#include <mach/msm_dsps.h>
#include <mach/msm_xo.h>
#include <mach/restart.h>
#include <mach/system.h>

#ifdef CONFIG_INPUT_CT406
#include <linux/ct406.h>
#endif
#ifdef CONFIG_BACKLIGHT_LM3532
#include <linux/i2c/lm3532.h>
#endif

#ifdef CONFIG_PN544
#include <linux/nfc/pn544.h>
#endif

#ifdef CONFIG_WCD9310_CODEC
#include <linux/slimbus/slimbus.h>
#include <linux/mfd/wcd9310/core.h>
#include <linux/mfd/wcd9310/pdata.h>
#endif

#ifdef CONFIG_LEDS_LM3559
#include <linux/leds-lm3559.h>
#endif

#include <linux/ion.h>
#include <mach/ion.h>
#include <linux/w1-gpio.h>

#include "timer.h"
#include "devices.h"
#include "devices-msm8x60.h"
#include "spm.h"
#include "board-msm8960.h"
#include "board-mmi.h"
#include "pm.h"
#include "cpuidle.h"
#include "rpm_resources.h"
#include "mpm.h"
#include "acpuclock.h"
#include "rpm_log.h"
#include "smd_private.h"
#include "pm-boot.h"

/* Initial PM8921 GPIO configurations */
static struct pm8xxx_gpio_init pm8921_gpios_teufel_m1[] = {
	PM8XXX_GPIO_DISABLE(6),				 			/* Disable unused */
	PM8XXX_GPIO_DISABLE(7),				 			/* Disable NFC */
	PM8XXX_GPIO_INPUT(16,	    PM_GPIO_PULL_UP_30), /* SD_CARD_WP */
	PM8XXX_GPIO_OUTPUT_FUNC(24, 0, PM_GPIO_FUNC_2),	 /* Red LED */
	PM8XXX_GPIO_OUTPUT_FUNC(25, 0, PM_GPIO_FUNC_2),	 /* Green LED */
	PM8XXX_GPIO_OUTPUT_FUNC(26, 0, PM_GPIO_FUNC_2),	 /* Blue LED */
	PM8XXX_GPIO_INPUT(22,	    PM_GPIO_PULL_UP_30), /* SD_CARD_DET_N */
	PM8XXX_GPIO_OUTPUT(43, 1),			 		/* DISP_RESET_N */
	PM8XXX_GPIO_OUTPUT(42, 0),                      /* USB 5V reg enable */
};

/* Initial PM8921 GPIO configurations */
static struct pm8xxx_gpio_init pm8921_gpios_teufel_m2[] = {
	PM8XXX_GPIO_DISABLE(6),				 			/* Disable unused */
	PM8XXX_GPIO_DISABLE(7),				 			/* Disable NFC */
	PM8XXX_GPIO_INPUT(16,	    PM_GPIO_PULL_UP_30), /* SD_CARD_WP */
	PM8XXX_GPIO_OUTPUT_FUNC(24, 0, PM_GPIO_FUNC_2),	 /* Red LED */
	PM8XXX_GPIO_OUTPUT_FUNC(25, 0, PM_GPIO_FUNC_2),	 /* Green LED */
	PM8XXX_GPIO_OUTPUT_FUNC(26, 0, PM_GPIO_FUNC_2),	 /* Blue LED */
	PM8XXX_GPIO_INPUT(20,	    PM_GPIO_PULL_UP_30), /* SD_CARD_DET_N */
	PM8XXX_GPIO_OUTPUT(43,	    0),			 		/* DISP_RESET_N */
	PM8XXX_GPIO_OUTPUT(42, 0),                      /* USB 5V reg enable */
};

/* Initial PM8921 GPIO configurations vanquish, quinara */
static struct pm8xxx_gpio_init pm8921_gpios_vanquish[] = {
	PM8XXX_GPIO_DISABLE(6),				 			/* Disable unused */
	PM8XXX_GPIO_DISABLE(7),				 			/* Disable NFC */
	PM8XXX_GPIO_INPUT(16,	    PM_GPIO_PULL_UP_30), /* SD_CARD_WP */
	PM8XXX_GPIO_OUTPUT(21,	    1),			 		/* Backlight Enable */
	PM8XXX_GPIO_DISABLE(22),			 			/* Disable NFC */
	PM8XXX_GPIO_OUTPUT_FUNC(24, 0, PM_GPIO_FUNC_2),	 /* Red LED */
	PM8XXX_GPIO_OUTPUT_FUNC(25, 0, PM_GPIO_FUNC_2),	 /* Green LED */
	PM8XXX_GPIO_OUTPUT_FUNC(26, 0, PM_GPIO_FUNC_2),	 /* Blue LED */
	PM8XXX_GPIO_INPUT(20,	    PM_GPIO_PULL_UP_30), /* SD_CARD_DET_N */
	PM8XXX_GPIO_OUTPUT(43,	    PM_GPIO_PULL_UP_1P5), /* DISP_RESET_N */
	PM8XXX_GPIO_OUTPUT_VIN(37, PM_GPIO_PULL_UP_30,
			PM_GPIO_VIN_L17),	/* DISP_RESET_N on P1C+ */
	PM8XXX_GPIO_OUTPUT(42, 0),                      /* USB 5V reg enable */
};

static struct pm8xxx_gpio_init pm8921_gpios_asanti[] = {
	PM8XXX_GPIO_DISABLE(6),				 /* Disable unused */
	PM8XXX_GPIO_DISABLE(7),				 /* Disable NFC */
	PM8XXX_GPIO_INPUT(16,	    PM_GPIO_PULL_UP_30), /* SD_CARD_WP */
	PM8XXX_GPIO_OUTPUT_FUNC(24, 0, PM_GPIO_FUNC_2),	 /* Red LED */
	PM8XXX_GPIO_OUTPUT_FUNC(25, 0, PM_GPIO_FUNC_2),	 /* Green LED */
	PM8XXX_GPIO_OUTPUT_FUNC(26, 0, PM_GPIO_FUNC_2),	 /* Blue LED */
	PM8XXX_GPIO_INPUT(22,	    PM_GPIO_PULL_UP_30), /* SD_CARD_DET_N */
	PM8XXX_GPIO_OUTPUT(43, 1),			 /* DISP_RESET_N */
	PM8XXX_GPIO_INPUT(33,	    PM_GPIO_PULL_UP_30), /* Volume Down key */
	PM8XXX_GPIO_OUTPUT(42, 0),                      /* USB 5V reg enable */
};

/* Initial PM8921 MPP configurations */
static struct pm8xxx_mpp_init pm8921_mpps[] __initdata = {
	/* External 5V regulator enable; shared by HDMI and USB_OTG switches. */
	PM8XXX_MPP_INIT(7, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_HIGH),
	PM8XXX_MPP_INIT(PM8XXX_AMUX_MPP_8, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH8,
								DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(11, D_BI_DIR, PM8921_MPP_DIG_LEVEL_S4, BI_PULLUP_1KOHM),
	PM8XXX_MPP_INIT(12, D_BI_DIR, PM8921_MPP_DIG_LEVEL_L17, BI_PULLUP_OPEN),
};

static u32 fdt_start_address; /* flattened device tree address */

static struct pm8xxx_gpio_init *pm8921_gpios = pm8921_gpios_vanquish;
static unsigned pm8921_gpios_size = ARRAY_SIZE(pm8921_gpios_vanquish);
static struct pm8xxx_keypad_platform_data *keypad_data = &mmi_keypad_data;
static int keypad_mode = MMI_KEYPAD_RESET;
/* Ulpi register setting  to increase eye digram strength for qinara HW */
static int phy_settings[] = {0x34, 0x82, 0x3f, 0x81, -1};

#define BOOT_MODE_MAX_LEN 64
static char boot_mode[BOOT_MODE_MAX_LEN + 1];
int __init board_boot_mode_init(char *s)
{
	strncpy(boot_mode, s, BOOT_MODE_MAX_LEN);
	boot_mode[BOOT_MODE_MAX_LEN] = '\0';
	return 1;
}
__setup("androidboot.mode=", board_boot_mode_init);

static int boot_mode_is_factory(void)
{
	return !strncmp(boot_mode, "factory", BOOT_MODE_MAX_LEN);
}

#ifdef CONFIG_EMU_DETECTION
static struct platform_device emu_det_device;
static bool core_power_init, enable_5v_init;
static int emu_det_enable_5v(int on);
static int emu_det_core_power(int on);

static int emu_det_core_power(int on)
{
	int rc = 0;
	static struct regulator *emu_vdd;

	if (!core_power_init) {
		emu_vdd = regulator_get(&emu_det_device.dev, "EMU_POWER");
		if (IS_ERR(emu_vdd)) {
			pr_err("unable to get EMU_POWER reg\n");
			return PTR_ERR(emu_vdd);
		}
		rc = regulator_set_voltage(emu_vdd, 2700000, 2700000);
		if (rc) {
			pr_err("unable to set voltage EMU_POWER reg\n");
			goto put_vdd;
		}
		core_power_init = true;
	}

	if (on) {
		rc = regulator_enable(emu_vdd);
		if (rc)
			pr_err("failed to enable EMU_POWER reg\n");
	} else {
		rc = regulator_disable(emu_vdd);
		if (rc)
			pr_err("failed to disable EMU_POWER reg\n");
	}
	return rc;

put_vdd:
	if (emu_vdd)
		regulator_put(emu_vdd);
	emu_vdd = NULL;
	return rc;
}

static int emu_det_enable_5v(int on)
{
	int rc = 0;
	static struct regulator *emu_ext_5v;

	if (!enable_5v_init) {
		emu_ext_5v = regulator_get(&emu_det_device.dev, "8921_usb_otg");
		if (IS_ERR(emu_ext_5v)) {
			pr_err("unable to get 5VS_OTG reg\n");
			return PTR_ERR(emu_ext_5v);
		}
		enable_5v_init = true;
	}

	if (on) {
		rc = regulator_enable(emu_ext_5v);
		if (rc)
			pr_err("failed to enable 5VS_OTG reg\n");
	} else {
		rc = regulator_disable(emu_ext_5v);
		if (rc)
			pr_err("failed to disable 5VS_OTG reg\n");
	}
	return rc;
}

static struct mmi_emu_det_platform_data mmi_emu_det_data = {
	.enable_5v = emu_det_enable_5v,
	.core_power = emu_det_core_power,
};

#define MSM8960_HSUSB_PHYS		0x12500000
#define MSM8960_HSUSB_SIZE		SZ_4K

static struct resource resources_emu_det[] = {
	{
		.start	= MSM8960_HSUSB_PHYS,
		.end	= MSM8960_HSUSB_PHYS + MSM8960_HSUSB_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "PHY_USB_IRQ",
		.start	= USB1_HS_IRQ,
		.end	= USB1_HS_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "EMU_SCI_OUT_GPIO",
		.start	= 90,	/* MSM GPIO */
		.end	= 90,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "EMU_ID_EN_GPIO",
		.start	= PM8921_MPP_PM_TO_SYS(9),	/* PM MPP */
		.end	= PM8921_MPP_PM_TO_SYS(9),
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "EMU_MUX_CTRL1_GPIO",
		.start	= 96,	/* MSM GPIO */
		.end	= 96,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "EMU_MUX_CTRL0_GPIO",
		.start	= 107,	/* MSM GPIO */
		.end	= 107,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "SEMU_ALT_MODE_EN_GPIO",
		.start	= PM8921_GPIO_PM_TO_SYS(17),	/* PM GPIO */
		.end	= PM8921_GPIO_PM_TO_SYS(17),
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "SEMU_PPD_DET_GPIO",
		.start	= PM8921_GPIO_PM_TO_SYS(35),	/* PM GPIO */
		.end	= PM8921_GPIO_PM_TO_SYS(35),
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "EMU_ID_GPIO",
		.start	= PM8921_MPP_PM_TO_SYS(10),	/* PM MPP */
		.end	= PM8921_MPP_PM_TO_SYS(10),
		.flags	= IORESOURCE_IO,
	},
};

static struct platform_device emu_det_device = {
	.name		= "emu_det",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(resources_emu_det),
	.resource	= resources_emu_det,
	.dev.platform_data = &mmi_emu_det_data,
};

static struct msm_otg_platform_data *otg_control_data = &msm_otg_pdata;

static __init void emu_mux_ctrl_config_pin(const char *res_name, int value)
{
	int rc;
	struct resource *res;

	res = platform_get_resource_byname(&emu_det_device,
		IORESOURCE_IO, res_name);
	if (!res) {
		pr_err("Resource %s cannot be configured\n", res_name);
		return;
	}
	rc = gpio_request(res->start, res_name);
	if (rc) {
		pr_err("Could not request %s for GPIO %d\n", res_name,
			res->start);
		return;
	}
	rc = gpio_direction_output(res->start, value);
	if (rc) {
		pr_err("Could not set %s for GPIO %d\n", res_name, res->start);
		gpio_free(res->start);
		return;
	}
}

static struct gpiomux_setting emu_det_gsbi12 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config emu_det_gsbi12_configs[] __initdata = {
	{
		.gpio      = 42,	/* GSBI12 WHISPER_TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &emu_det_gsbi12,
			[GPIOMUX_ACTIVE] = &emu_det_gsbi12,
		},
	},
	{
		.gpio      = 43,	/* GSBI12 WHISPER_RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &emu_det_gsbi12,
			[GPIOMUX_ACTIVE] = &emu_det_gsbi12,
		},
	},
};

#define MSM_GSBI12_PHYS		0x12480000
#define MSM_UART12DM_PHYS	(MSM_GSBI12_PHYS + 0x40000)
/* GSBIn HCLK register address */
#define GSBIn_HCLK_CTRL_REG(n)	(0x009029C0+(32*(n-1)))
/* Bit to Turn on Clk */
#define CLK_BRANCH_ENA		(1<<4)
/* Protocol for UART/I2C */
#define UART_I2C_PROTOCOL	(0x6<<4)

static struct resource resources_uart_gsbi12[] = {
	{
		.start	= GSBI12_UARTDM_IRQ,
		.end	= GSBI12_UARTDM_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= MSM_UART12DM_PHYS,
		.end	= MSM_UART12DM_PHYS + PAGE_SIZE - 1,
		.name	= "uartdm_resource",
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= 14,
		.end	= 15,
		.name	= "uartdm_channels",
		.flags	= IORESOURCE_DMA,
	},
	{
		.start	= 15,
		.end	= 14,
		.name	= "uartdm_crci",
		.flags	= IORESOURCE_DMA,
	},
};

static u64 msm_uart_dm12_dma_mask = DMA_BIT_MASK(32);
struct platform_device msm8960_device_uart_gsbi12 = {
	.name	= "msm_serial_hs",
	.id	= 1,
	.num_resources	= ARRAY_SIZE(resources_uart_gsbi12),
	.resource	= resources_uart_gsbi12,
	.dev	= {
		.dma_mask		= &msm_uart_dm12_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

static __init void mot_init_emu_det_resources(
			struct msm_otg_platform_data *ctrl_data)
{
	if (ctrl_data)
		msm8960_i2c_qup_gsbi12_pdata.use_gsbi_shared_mode = 1;
}

static __init void mot_emu_det_protocol_code(void)
{
	void *gsbi_pclk;
	void *gsbi_ctrl;
	uint32_t ClkStatus;

	gsbi_pclk = ioremap_nocache(GSBIn_HCLK_CTRL_REG(12), 4);
	if (IS_ERR_OR_NULL(gsbi_pclk)) {
		pr_err("unable to map clock ctrl register\n");
		return;
	}

	gsbi_ctrl = ioremap_nocache(MSM_GSBI12_PHYS, 4);
	if (IS_ERR_OR_NULL(gsbi_ctrl)) {
		iounmap(gsbi_pclk);
		pr_err("unable to map GSBI ctrl register\n");
		return;
	}

	ClkStatus = readl_relaxed(gsbi_pclk) & CLK_BRANCH_ENA;
	if (!ClkStatus) {
		writel_relaxed(CLK_BRANCH_ENA, gsbi_pclk);
		mb();
	}

	writel_relaxed(UART_I2C_PROTOCOL, gsbi_ctrl);
	mb();

	if (!ClkStatus)
		writel_relaxed(0x00, gsbi_pclk);

	iounmap(gsbi_pclk);
	iounmap(gsbi_ctrl);
}

static __init void mot_init_emu_detection(
			struct msm_otg_platform_data *ctrl_data)
{
	if (ctrl_data && !boot_mode_is_factory()) {
		ctrl_data->otg_control = OTG_ACCY_CONTROL;
		ctrl_data->pmic_id_irq = 0;
		ctrl_data->accy_pdev = &emu_det_device;

		msm_gpiomux_install(emu_det_gsbi12_configs,
			ARRAY_SIZE(emu_det_gsbi12_configs));
		mot_emu_det_protocol_code();
	} else {
		/* If platform data is not set, safely drive the MUX
		 * CTRL pins to the USB configuration.
		 */
		emu_mux_ctrl_config_pin("EMU_MUX_CTRL0_GPIO", 1);
		emu_mux_ctrl_config_pin("EMU_MUX_CTRL1_GPIO", 0);
	}
}
#endif

/* defaulting to qinara, atag parser will override */
/* todo: finalize the names, move display related stuff to board-msm8960-panel.c */
#if defined(CONFIG_FB_MSM_MIPI_MOT_CMD_HD_PT)
#define DEFAULT_PANEL_NAME "mipi_mot_cmd_auo_hd_450"
#elif defined(CONFIG_FB_MSM_MIPI_MOT_VIDEO_HD_PT)
#define DEFAULT_PANEL_NAME "mipi_mot_video_smd_hd_465"
#elif defined(CONFIG_FB_MSM_MIPI_MOT_CMD_QHD_PT)
#define DEFAULT_PANEL_NAME "mipi_mot_cmd_auo_qhd_430"
#else
#define DEFAULT_PANEL_NAME ""
#endif
#define PANEL_NAME_MAX_LEN	32
#define HDMI_PANEL_NAME	"hdmi_msm"

static char panel_name[PANEL_NAME_MAX_LEN + 1] = DEFAULT_PANEL_NAME;

static int is_smd(void) {
	return !strncmp(panel_name, "mipi_mot_video_smd_hd_465", PANEL_NAME_MAX_LEN);
}

static bool dsi_power_on;
static bool use_mdp_vsync = MDP_VSYNC_ENABLED;

static int mipi_dsi_panel_power(int on)
{
	static struct regulator *reg_vddio, *reg_l23, *reg_l2, *reg_vci;
	static struct regulator *ext_5v_vreg;
	static int disp_5v_en, lcd_reset;
	static int lcd_reset1; /* this is a hacked for vanquish phone */
	int rc;

	pr_info("%s: state : %d\n", __func__, on);

	if (!dsi_power_on) {

		if (is_smd() && system_rev >= HWREV_P1) {
			reg_vddio = regulator_get(&msm_mipi_dsi1_device.dev,
				"disp_vddio");
		} else {
			reg_vddio = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdc");
		}

		if (IS_ERR(reg_vddio)) {
			pr_err("could not get 8921_vddio/vdc, rc = %ld\n",
				PTR_ERR(reg_vddio));
			return -ENODEV;
		}
		reg_l23 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l23)) {
			pr_err("could not get 8921_l23, rc = %ld\n",
				PTR_ERR(reg_l23));
			return -ENODEV;
		}

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		if (is_smd() && system_rev >= HWREV_P1) {
			reg_vci = regulator_get(&msm_mipi_dsi1_device.dev,
					"disp_vci");
			if (IS_ERR(reg_vci)) {
				pr_err("could not get disp_vci, rc = %ld\n",
					PTR_ERR(reg_vci));
				return -ENODEV;
			}
		}

		rc = regulator_set_voltage(reg_vddio, 2700000, 2700000);
		if (rc) {

			pr_err("set_voltage l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_set_voltage(reg_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		if (is_smd() && system_rev >= HWREV_P1) {
			rc = regulator_set_voltage(reg_vci, 3100000, 3100000);
			if (rc) {
				pr_err("set_voltage vci failed, rc=%d\n", rc);
				return -EINVAL;
			}
		}

		/*
		 * TODO... this is a system voltage that will supply to
		 * HDMI, LED and the display. For the bring up purpose
		 * the display driver will enable it here, but it must
		 * be moved to the correct place, or the display driver
		 * must need to vote for this voltage.
		 */
		ext_5v_vreg = regulator_get(NULL, "ext_5v");
		if (IS_ERR(ext_5v_vreg)) {
			pr_err("could not get 8921_ext_5v, rc = %ld\n",
							PTR_ERR(ext_5v_vreg));
			return -ENODEV;
		}

		rc = regulator_enable(ext_5v_vreg);
		if (rc) {
			pr_err("regulator enable failed, rc=%d\n", rc);
			return  -EINVAL;
		}

		if (is_smd() && system_rev >= HWREV_P1) {
			rc = regulator_enable(reg_vci);
			if (rc) {
				pr_err("regulator enable failed, rc=%d\n", rc);
				return  -EINVAL;
			}
		}

		/*
		 * This is a work around for Vanquish P1C HW ONLY.
		 * There are 2 HW versions of vanquish P1C, wing board phone and
		 * normal phone. The wing P1C phone will use GPIO_PM 43 and
		 * normal P1C phone will use GPIO_PM 37  but both of them will
		 * have the same HWREV.
		 * To make both of them to work, then if HWREV=P1C, then we
		 * will toggle both GPIOs 43 and 37, but there will be one to
		 * be used, and there will be no harm if another doesn't use.
		 */
		if (is_smd()) {
			if (system_rev == HWREV_P1C) {
				lcd_reset = PM8921_GPIO_PM_TO_SYS(43);
				lcd_reset1 = PM8921_GPIO_PM_TO_SYS(37);
			} else if (system_rev > HWREV_P1C)
				lcd_reset = PM8921_GPIO_PM_TO_SYS(37);
			else
				lcd_reset = PM8921_GPIO_PM_TO_SYS(43);
		} else
			lcd_reset = PM8921_GPIO_PM_TO_SYS(43);

		rc = gpio_request(lcd_reset, "disp_rst_n");
		if (rc) {
			pr_err("request lcd_reset failed, rc=%d\n", rc);
			return -ENODEV;
		}

		if (is_smd() && lcd_reset1 != 0) {
			rc = gpio_request(lcd_reset1, "disp_rst_1_n");
			if (rc) {
				pr_err("request lcd_reset1 failed, rc=%d\n",
									rc);
				return -ENODEV;
			}
		}

		disp_5v_en = 13;
		rc = gpio_request(disp_5v_en, "disp_5v_en");
		if (rc) {
			pr_err("request disp_5v_en failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = gpio_direction_output(disp_5v_en, 1);
		if (rc) {
			pr_err("set output disp_5v_en failed, rc=%d\n", rc);
			return -ENODEV;
		}

		if (is_smd() && system_rev < HWREV_P1) {
			rc = gpio_request(12, "disp_3_3");
			if (rc) {
				pr_err("%s: unable to request gpio %d (%d)\n",
						__func__, 12, rc);
				return -ENODEV;
			}

			rc = gpio_direction_output(12, 1);
			if (rc) {
				pr_err("%s: Unable to set direction\n", __func__);;
				return -EINVAL;
			}
		}

		if (is_smd() && system_rev >= HWREV_P1) {
			rc = gpio_request(0, "dsi_vci_en");
			if (rc) {
				pr_err("%s: unable to request gpio %d (%d)\n",
						__func__, 0, rc);
				return -ENODEV;
			}

			rc = gpio_direction_output(0, 1);
			if (rc) {
				pr_err("%s: Unable to set direction\n", __func__);;
				return -EINVAL;
			}
		}

		dsi_power_on = true;
	}
	if (on) {
		rc = regulator_set_optimum_mode(reg_vddio, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		if (is_smd() && system_rev >= HWREV_P1) {
			rc = regulator_set_optimum_mode(reg_vci, 100000);
			if (rc < 0) {
				pr_err("set_optimum_mode vci failed, rc=%d\n", rc);
				return -EINVAL;
			}
		}

		rc = regulator_enable(reg_vddio);
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_l23);
		if (rc) {
			pr_err("enable l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		if (is_smd() && system_rev >= HWREV_P1) {
			rc = regulator_enable(reg_vci);
			if (rc) {
				pr_err("enable vci failed, rc=%d\n", rc);
				return -ENODEV;
			}
		}

		gpio_set_value(disp_5v_en, 1);
		if (is_smd() && system_rev < HWREV_P1)
			gpio_set_value_cansleep(12, 1);
		if (is_smd() && system_rev >= HWREV_P1)
			gpio_set_value_cansleep(0, 1);
		msleep(10);

		gpio_set_value_cansleep(lcd_reset, 1);

		if (is_smd() && lcd_reset1 != 0)
			gpio_set_value_cansleep(lcd_reset1, 1);

		msleep(20);
	} else {
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_vddio);
		if (rc) {
			pr_err("disable reg_l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_l23);
		if (rc) {
			pr_err("disable reg_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		if (is_smd() && system_rev >= HWREV_P1) {
			rc = regulator_disable(reg_vci);
			if (rc) {
				pr_err("disable reg_vci failed, rc=%d\n", rc);
				return -ENODEV;
			}
		}
		rc = regulator_set_optimum_mode(reg_vddio, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		if (is_smd() && system_rev >= HWREV_P1) {
			rc = regulator_set_optimum_mode(reg_vci, 100);
			if (rc < 0) {
				pr_err("set_optimum_mode vci failed, rc=%d\n", rc);
				return -EINVAL;
			}
		}
		gpio_set_value_cansleep(lcd_reset, 0);
		if (is_smd() && lcd_reset1 != 0)
			gpio_set_value_cansleep(lcd_reset1, 0);

		if (!(is_smd()) && system_rev >= HWREV_P2)
			gpio_set_value(disp_5v_en, 0);
		else if (is_smd())
			gpio_set_value(disp_5v_en, 0);
		if (is_smd() && system_rev < HWREV_P1) {
			gpio_set_value_cansleep(12, 0);
		}
		if (is_smd() && system_rev >= HWREV_P1) {
			gpio_set_value_cansleep(0, 0);
		}
	}
	return 0;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = MDP_VSYNC_GPIO,
	.dsi_power_save = mipi_dsi_panel_power,
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#ifdef CONFIG_MSM_BUS_SCALING
	msm_fb_register_device("dtv", &dtv_pdata);
#endif
}

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL

static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);

static struct msm_hdmi_platform_data hdmi_msm_data = {
	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
};

static int hdmi_enable_5v(int on)
{
	/* TBD: PM8921 regulator instead of 8901 */
	static struct regulator *reg_8921_hdmi_mvs;	/* HDMI_5V */
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8921_hdmi_mvs)
		reg_8921_hdmi_mvs = regulator_get(&hdmi_msm_device.dev,
			"hdmi_mvs");

	if (on) {
		rc = regulator_enable(reg_8921_hdmi_mvs);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
			return rc;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_hdmi_mvs);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
}

static int hdmi_core_power(int on, int show)
{
	static struct regulator *reg_8921_l23, *reg_8921_s4;
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	/* TBD: PM8921 regulator instead of 8901 */
	if (!reg_8921_l23) {
		reg_8921_l23 = regulator_get(&hdmi_msm_device.dev, "hdmi_avdd");
		if (IS_ERR(reg_8921_l23)) {
			pr_err("could not get reg_8921_l23, rc = %ld\n",
				PTR_ERR(reg_8921_l23));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_l23, rc=%d\n", rc);
			return -EINVAL;
		}
	}
	if (!reg_8921_s4) {
		reg_8921_s4 = regulator_get(&hdmi_msm_device.dev, "hdmi_vcc");
		if (IS_ERR(reg_8921_s4)) {
			pr_err("could not get reg_8921_s4, rc = %ld\n",
				PTR_ERR(reg_8921_s4));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_s4, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_s4, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_8921_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_8921_l23);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_avdd", rc);
			return rc;
		}
		rc = regulator_enable(reg_8921_s4);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_vcc", rc);
			return rc;
		}
		rc = gpio_request(100, "HDMI_DDC_CLK");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_CLK", 100, rc);
			goto error1;
		}
		rc = gpio_request(101, "HDMI_DDC_DATA");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_DATA", 101, rc);
			goto error2;
		}
		rc = gpio_request(102, "HDMI_HPD");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_HPD", 102, rc);
			goto error3;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(100);
		gpio_free(101);
		gpio_free(102);

		rc = regulator_disable(reg_8921_l23);
		if (rc) {
			pr_err("disable reg_8921_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_8921_s4);
		if (rc) {
			pr_err("disable reg_8921_s4 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_8921_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;

error3:
	gpio_free(101);
error2:
	gpio_free(100);
error1:
	regulator_disable(reg_8921_l23);
	regulator_disable(reg_8921_s4);
	return rc;
}

static int hdmi_cec_power(int on)
{
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(99, "HDMI_CEC_VAR");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_CEC_VAR", 99, rc);
			goto error;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(99);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
error:
	return rc;
}
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

#ifdef CONFIG_LEDS_LM3559
static struct lm3559_platform_data camera_flash_3559;
static unsigned short flash_hw_enable;

static struct lm3559_platform_data camera_flash_3559 = {
	.flags = (LM3559_PRIVACY | LM3559_TORCH |
			LM3559_FLASH | LM3559_FLASH_LIGHT |
			LM3559_MSG_IND | LM3559_ERROR_CHECK),
	.enable_reg_def = 0x00,
	.gpio_reg_def = 0x00,
	.adc_delay_reg_def = 0xc0,
	.vin_monitor_def = 0xff,
	.torch_brightness_def = 0x5b,
	.flash_brightness_def = 0xaa,
	.flash_duration_def = 0x0f,
	.flag_reg_def = 0x00,
	.config_reg_1_def = 0x6a,
	.config_reg_2_def = 0x00,
	.privacy_reg_def = 0x10,
	.msg_ind_reg_def = 0x00,
	.msg_ind_blink_reg_def = 0x1f,
	.pwm_reg_def = 0x00,
	.torch_enable_val = 0x1a,
	.flash_enable_val = 0x1b,
	.privacy_enable_val = 0x19,
	.pwm_val = 0x02,
	.msg_ind_val = 0xa0,
	.msg_ind_blink_val = 0x1f,
	.hw_enable = 0,
};

static struct i2c_board_info msm_camera_flash_boardinfo[] __initdata = {
	{
	I2C_BOARD_INFO("lm3559_led", 0x53),
	.platform_data = &camera_flash_3559,
	},
};
#endif /* CONFIG_LEDS_LM3559 */

#ifdef CONFIG_MSM_CAMERA

static struct i2c_board_info msm_camera_boardinfo[] __initdata = {
#ifdef CONFIG_MOTSOC1
	{
	I2C_BOARD_INFO("motsoc1", 0x1F),
	},
#endif
#ifdef CONFIG_MT9M114
	{
	I2C_BOARD_INFO("mt9m114", 0x48),
	},
#endif
};

#ifdef CONFIG_MOTSOC1
static struct msm_camera_sensor_flash_data flash_motsoc1 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_platform_info sensor_board_info_motsoc1 = {
	.mount_angle  = 90,
	.sensor_reset = 0,
	.sensor_pwd   = 0,
	.vcm_pwd      = 0,
	.vcm_enable   = 0,
};

static struct msm_camera_sensor_info msm_camera_sensor_motsoc1_data = {
	.sensor_name          = "motsoc1",
	.pdata                = &msm_camera_csi_device_data[0],
	.flash_data           = &flash_motsoc1,
	.sensor_platform_info = &sensor_board_info_motsoc1,
	.gpio_conf            = &msm_camif_gpio_conf,
	.csi_if               = 1,
	.camera_type          = BACK_CAMERA_2D,
};

struct platform_device msm8960_camera_sensor_motsoc1 = {
	.name    = "msm_camera_motsoc1",
	.dev     = {
		.platform_data = &msm_camera_sensor_motsoc1_data,
	},
};
#endif
#ifdef CONFIG_MT9M114
static struct msm_camera_sensor_flash_data flash_mt9m114 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_platform_info sensor_board_info_mt9m114 = {
	.mount_angle    = 270,
	.sensor_reset   = 0,
	.sensor_pwd     = 0,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9m114_data = {
	.sensor_name          = "mt9m114",
	.pdata                = &msm_camera_csi_device_data[1],
	.flash_data           = &flash_mt9m114,
	.sensor_platform_info = &sensor_board_info_mt9m114,
	.gpio_conf            = &msm_camif_gpio_conf,
	.csi_if               = 1,
	.camera_type          = FRONT_CAMERA_2D,
};

struct platform_device msm8960_camera_sensor_mt9m114 = {
	.name    = "msm_camera_mt9m114",
	.dev     = {
		.platform_data = &msm_camera_sensor_mt9m114_data,
	},
};
#endif

void __init msm8960_init_cam(void)
{
	int i;
	struct platform_device *cam_dev[] = {
#ifdef CONFIG_MOTSOC1
		&msm8960_camera_sensor_motsoc1,
#endif
#ifdef CONFIG_MT9M114
		&msm8960_camera_sensor_mt9m114,
#endif
	};

	for (i = 0; i < ARRAY_SIZE(cam_dev); i++) {
		struct msm_camera_sensor_info *s_info;
		s_info = cam_dev[i]->dev.platform_data;
		msm_get_cam_resources(s_info);
		platform_device_register(cam_dev[i]);
	}

	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);
}
#endif

static void __init msm8960_init_cam_flash(unsigned short hw_enable)
{
#ifdef CONFIG_LEDS_LM3559
	if (hw_enable) {
		camera_flash_3559.hw_enable = hw_enable;
	}
#endif /* CONFIG_LEDS_LM3559 */
}

static int msm_fb_detect_panel(const char *name)
{
	if (!strncmp(name, panel_name, PANEL_NAME_MAX_LEN)) {
		pr_info("%s: detected %s\n", __func__, name);
		return 0;
	}
	if (!strncmp(name, HDMI_PANEL_NAME,
			strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	pr_warning("%s: not supported '%s'", __func__, name);
	return -ENODEV;
}

#ifdef CONFIG_USB_MSM_OTG_72K
static struct msm_otg_platform_data msm_otg_pdata;
#else
#define USB_5V_EN		42
static void msm_hsusb_vbus_power(bool on)
{
	int rc;
	static bool vbus_is_on;
	static struct regulator *mvs_otg_switch;

	if (vbus_is_on == on)
		return;

	if (on) {
		mvs_otg_switch = regulator_get(&msm8960_device_otg.dev,
					       "vbus_otg");
		if (IS_ERR(mvs_otg_switch)) {
			pr_err("Unable to get mvs_otg_switch\n");
			return;
		}

		rc = gpio_request(PM8921_GPIO_PM_TO_SYS(USB_5V_EN),
						"usb_5v_en");
		if (rc < 0) {
			pr_err("failed to request usb_5v_en gpio\n");
			goto put_mvs_otg;
		}

		rc = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(USB_5V_EN), 1);
		if (rc) {
			pr_err("%s: unable to set_direction for gpio [%d]\n",
				__func__, PM8921_GPIO_PM_TO_SYS(USB_5V_EN));
			goto free_usb_5v_en;
		}

		if (regulator_enable(mvs_otg_switch)) {
			pr_err("unable to enable mvs_otg_switch\n");
			goto err_ldo_gpio_set_dir;
		}

		vbus_is_on = true;
		return;
	}
	regulator_disable(mvs_otg_switch);
err_ldo_gpio_set_dir:
	gpio_set_value(PM8921_GPIO_PM_TO_SYS(USB_5V_EN), 0);
free_usb_5v_en:
	gpio_free(PM8921_GPIO_PM_TO_SYS(USB_5V_EN));
put_mvs_otg:
	regulator_put(mvs_otg_switch);
	vbus_is_on = false;
}
#endif
static struct led_pwm_gpio pm8xxx_pwm_gpio_leds[] = {
	[0] = {
		.name			= "red",
		.default_trigger	= "none",
		.pwm_id = 0,
		.gpio = PM8921_GPIO_PM_TO_SYS(24),
		.active_low = 0,
		.retain_state_suspended = 1,
		.default_state = 0,
	},
	[1] = {
		.name			= "green",
		.default_trigger	= "none",
		.pwm_id = 1,
		.gpio = PM8921_GPIO_PM_TO_SYS(25),
		.active_low = 0,
		.retain_state_suspended = 1,
		.default_state = 0,
	},
	[2] = {
		.name			= "blue",
		.default_trigger	= "none",
		.pwm_id = 2,
		.gpio = PM8921_GPIO_PM_TO_SYS(26),
		.active_low = 0,
		.retain_state_suspended = 1,
		.default_state = 0,
	},
};

static struct led_pwm_gpio_platform_data pm8xxx_rgb_leds_pdata = {
	.num_leds = ARRAY_SIZE(pm8xxx_pwm_gpio_leds),
	.leds = pm8xxx_pwm_gpio_leds,
};

static struct platform_device pm8xxx_rgb_leds_device = {
	.name	= "pm8xxx_rgb_leds",
	.id	= -1,
	.dev	= {
		.platform_data = &pm8xxx_rgb_leds_pdata,
	},
};

static void w1_gpio_enable_regulators(int enable);

#define BATT_EPROM_GPIO 93

static struct w1_gpio_platform_data msm8960_w1_gpio_device_pdata = {
	.pin = BATT_EPROM_GPIO,
	.is_open_drain = 0,
	.enable_external_pullup = w1_gpio_enable_regulators,
};

static struct platform_device msm8960_w1_gpio_device = {
	.name	= "w1-gpio",
	.dev	= {
		.platform_data = &msm8960_w1_gpio_device_pdata,
	},
};

static void w1_gpio_enable_regulators(int enable)
{
	static struct regulator *vdd1;
	static struct regulator *vdd2;
	int rc;

	if (!vdd1)
		vdd1 = regulator_get(&msm8960_w1_gpio_device.dev, "8921_l7");

	if (!vdd2)
		vdd2 = regulator_get(&msm8960_w1_gpio_device.dev, "8921_l17");

	if (enable) {
		if (!IS_ERR_OR_NULL(vdd1)) {
			rc = regulator_set_voltage(vdd1, 1850000, 2950000);
			if (!rc)
				rc = regulator_enable(vdd1);
			if (rc)
				pr_err("w1_gpio Failed 8921_l7 to 2.7V\n");
		}
		if (!IS_ERR_OR_NULL(vdd2)) {
			rc = regulator_set_voltage(vdd2, 2700000, 2950000);
			if (!rc)
				rc = regulator_enable(vdd2);
			if (rc)
				pr_err("w1_gpio Failed 8921_l17 to 2.7V\n");
		}
	} else {
		if (!IS_ERR_OR_NULL(vdd1)) {
			rc = regulator_disable(vdd1);
			if (rc)
				pr_err("w1_gpio unable to disable 8921_l7\n");
		}
		if (!IS_ERR_OR_NULL(vdd2)) {
			rc = regulator_disable(vdd2);
			if (rc)
				pr_err("w1_gpio unable to disable 8921_l17\n");
		}
	}
}

static struct platform_device *mmi_devices[] __initdata = {
	&msm8960_w1_gpio_device,
	&msm_8960_q6_lpass,
	&msm_8960_q6_mss_fw,
	&msm_8960_q6_mss_sw,
	&msm8960_device_otg,
	&msm8960_device_gadget_peripheral,
	&msm_device_hsusb_host,
	&android_usb_device,
	&msm_pcm,
	&msm_pcm_routing,
	&msm_cpudai0,
	&msm_cpudai1,
	&msm_cpudai_hdmi_rx,
	&msm_cpudai_bt_rx,
	&msm_cpudai_bt_tx,
	&msm_cpudai_fm_rx,
	&msm_cpudai_fm_tx,
	&msm_cpudai_auxpcm_rx,
	&msm_cpudai_auxpcm_tx,
	&msm_cpu_fe,
	&msm_stub_codec,
	&msm_kgsl_3d0,
#ifdef CONFIG_MSM_KGSL_2D 			/* OpenVG support */
	&msm_kgsl_2d0,
	&msm_kgsl_2d1,
#endif
#ifdef CONFIG_MSM_GEMINI  			/* Inline JPEG engine */
	&msm8960_gemini_device,
#endif
	&msm_voice,
	&msm_voip,
	&msm_lpa_pcm,
	&msm_cpudai_afe_01_rx,
	&msm_cpudai_afe_01_tx,
	&msm_cpudai_afe_02_rx,
	&msm_cpudai_afe_02_tx,
	&msm_pcm_afe,
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL	/* HDMI support */
	&hdmi_msm_device,
#endif
	&msm_pcm_hostless,
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
	&msm_tsens_device,
#ifdef CONFIG_EMU_DETECTION
	&msm8960_device_uart_gsbi12,
#endif
	&pm8xxx_rgb_leds_device,
};

#ifdef CONFIG_I2C

enum i2c_type {
	TOUCHSCREEN_MELFAS100_TS = 0,
	TOUCHSCREEN_CYTTSP3,
	TOUCHSCREEN_ATMEL,
	CAMERA_MSM,
	ALS_CT406,
	BACKLIGHT_LM3532,
	NFC_PN544,
	CAMERA_FLASH_MSM
};

struct i2c_registry {
	unsigned				enabled;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

#ifdef CONFIG_PN544
struct pn544_i2c_platform_data pn544_pdata = {
		.irq_gpio = -1,
		.ven_gpio = -1,
		.firmware_gpio = -1,
};

static struct i2c_board_info pn544_i2c_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("pn544", 0x28),
		.platform_data = &pn544_pdata,
		.irq = MSM_GPIO_TO_INT(GPIO_NFC_IRQ),
	},
};

static void __init msm8960_pn544_init(void)
{
	printk(KERN_DEBUG "msm8960_pn544_init: is called, set gpio numbers.\n");
	pn544_pdata.ven_gpio = GPIO_NFC_VEN;
	pn544_pdata.irq_gpio = GPIO_NFC_IRQ;
	pn544_pdata.firmware_gpio = GPIO_NFC_FW_UPDATE;
}
#endif /* CONFIG_PN544 */

#ifdef CONFIG_INPUT_CT406
static struct i2c_board_info ct406_i2c_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("ct406", 0x39),
		.platform_data = &mp_ct406_pdata,
		.irq = MSM_GPIO_TO_INT(CT406_IRQ_GPIO),
	},
};
#endif

#ifdef CONFIG_BACKLIGHT_LM3532
static struct i2c_board_info lm3532_i2c_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("lm3532", 0x38),
		.platform_data = &mp_lm3532_pdata,
	},
};
#endif

#ifdef CONFIG_TOUCHSCREEN_MELFAS100_TS
static struct i2c_board_info i2c_bus3_melfas_ts_info[] __initdata = {
	{
		I2C_BOARD_INFO(MELFAS_TS_NAME, 0x48),
		.irq = MSM_GPIO_TO_INT(MELFAS_TOUCH_INT_GPIO),
		.platform_data = &touch_pdata,
	},
};
#endif

#ifdef CONFIG_TOUCHSCREEN_CYTTSP3
static struct i2c_board_info cyttsp_i2c_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO(CY3_I2C_NAME, 0x3B),
		.platform_data = &ts_platform_data_cyttsp3,
		.irq = MSM_GPIO_TO_INT(CYTT_GPIO_INTR),
	},
};
#endif
#ifdef CONFIG_TOUCHSCREEN_ATMXT
static struct i2c_board_info atmxt_i2c_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO(ATMXT_I2C_NAME, 0x4A),
		.platform_data = &ts_platform_data_atmxt,
		.irq = MSM_GPIO_TO_INT(ATMXT_GPIO_INTR),
	},
};
#endif

static struct i2c_registry msm8960_i2c_devices[] __initdata = {
#ifdef CONFIG_TOUCHSCREEN_MELFAS100_TS
	[TOUCHSCREEN_MELFAS100_TS] = {
		0,
		MSM_8960_GSBI3_QUP_I2C_BUS_ID,
		i2c_bus3_melfas_ts_info,
		ARRAY_SIZE(i2c_bus3_melfas_ts_info),
	},
#endif
#ifdef CONFIG_TOUCHSCREEN_CYTTSP3
	[TOUCHSCREEN_CYTTSP3] = {
		0,
		MSM_8960_GSBI3_QUP_I2C_BUS_ID,
		cyttsp_i2c_boardinfo,
		ARRAY_SIZE(cyttsp_i2c_boardinfo),
	},
#endif
#ifdef CONFIG_TOUCHSCREEN_ATMXT
	[TOUCHSCREEN_ATMEL] = {
		0,
		MSM_8960_GSBI3_QUP_I2C_BUS_ID,
		atmxt_i2c_boardinfo,
		ARRAY_SIZE(atmxt_i2c_boardinfo),
	},
#endif
#ifdef CONFIG_MSM_CAMERA
	[CAMERA_MSM] = {
		0,
		MSM_8960_GSBI4_QUP_I2C_BUS_ID,
		msm_camera_boardinfo,
		ARRAY_SIZE(msm_camera_boardinfo),
	},
#endif
#ifdef CONFIG_LEDS_LM3559
	[CAMERA_FLASH_MSM] = {
		0,
		MSM_8960_GSBI4_QUP_I2C_BUS_ID,
		msm_camera_flash_boardinfo,
		ARRAY_SIZE(msm_camera_flash_boardinfo),
	},
#endif
#ifdef CONFIG_INPUT_CT406
	[ALS_CT406] = {
		0,
		MSM_8960_GSBI10_QUP_I2C_BUS_ID,
		ct406_i2c_boardinfo,
		ARRAY_SIZE(ct406_i2c_boardinfo),
	},
#endif
#ifdef CONFIG_BACKLIGHT_LM3532
	[BACKLIGHT_LM3532] = {
		0,
		MSM_8960_GSBI10_QUP_I2C_BUS_ID,
		lm3532_i2c_boardinfo,
		ARRAY_SIZE(lm3532_i2c_boardinfo),
	},
#endif
#ifdef CONFIG_PN544
    [NFC_PN544] = {
        0,
        MSM_8960_GSBI10_QUP_I2C_BUS_ID,
        pn544_i2c_boardinfo,
        ARRAY_SIZE(pn544_i2c_boardinfo),
    },
#endif /* CONFIG_PN544 */
};

#define ENABLE_I2C_DEVICE(device)	{ msm8960_i2c_devices[device].enabled = 1; }

#endif /* CONFIG_I2C */

static void __init register_i2c_devices(void)
{
#ifdef CONFIG_I2C
	int i;

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(msm8960_i2c_devices); ++i) {
		if (msm8960_i2c_devices[i].enabled)
			i2c_register_board_info(msm8960_i2c_devices[i].bus,
						msm8960_i2c_devices[i].info,
						msm8960_i2c_devices[i].len);
	}
#endif
}

static unsigned sdc_detect_gpio = 20;

static struct gpiomux_setting mdp_disp_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdp_disp_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm8960_mdp_5v_en_configs[] __initdata = {
	{
		.gpio = 43,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mdp_disp_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdp_disp_suspend_cfg,
		},
	}
};

#ifdef CONFIG_INPUT_GPIO
static struct gpiomux_setting slide_det_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config mot_slide_detect_configs[] __initdata = {
	{
		.gpio = 11,
		.settings = {
			[GPIOMUX_ACTIVE]    = &slide_det_cfg,
			[GPIOMUX_SUSPENDED] = &slide_det_cfg,
		},
	},
};
#endif

#ifdef CONFIG_VIB_TIMED
/* vibrator GPIO configuration */
static struct gpiomux_setting vib_setting_suspended = {
		.func = GPIOMUX_FUNC_GPIO, /*suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting vib_setting_active = {
		.func = GPIOMUX_FUNC_GPIO, /*active*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm8960_vib_configs[] = {
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_ACTIVE]    = &vib_setting_active,
			[GPIOMUX_SUSPENDED] = &vib_setting_suspended,
		},
	},
	{
		.gpio = 47,
		.settings = {
			[GPIOMUX_ACTIVE]    = &vib_setting_active,
			[GPIOMUX_SUSPENDED] = &vib_setting_suspended,
		},
	},
};
#endif

static struct gpiomux_setting batt_eprom_setting_suspended = {
		.func = GPIOMUX_FUNC_GPIO, /*suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting batt_eprom_setting_active = {
		.func = GPIOMUX_FUNC_GPIO, /*active*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm8960_batt_eprom_configs[] = {
	{
		.gpio = BATT_EPROM_GPIO,
		.settings = {
			[GPIOMUX_ACTIVE]    = &batt_eprom_setting_active,
			[GPIOMUX_SUSPENDED] = &batt_eprom_setting_suspended,
		},
	},
};

static void __init mot_gpiomux_init(unsigned kp_mode)
{
	msm_gpiomux_install(msm8960_mdp_5v_en_configs,
			ARRAY_SIZE(msm8960_mdp_5v_en_configs));
#ifdef CONFIG_INPUT_GPIO
	if (kp_mode & MMI_KEYPAD_SLIDER)
		msm_gpiomux_install(mot_slide_detect_configs,
				ARRAY_SIZE(mot_slide_detect_configs));
#endif
#ifdef CONFIG_VIB_TIMED
	msm_gpiomux_install(msm8960_vib_configs,
			ARRAY_SIZE(msm8960_vib_configs));
#endif
	msm_gpiomux_install(msm8960_batt_eprom_configs,
			ARRAY_SIZE(msm8960_batt_eprom_configs));
}

static int mot_tcmd_export_gpio(void)
{
	int rc;

	rc = gpio_request(1, "USB_HOST_EN");
	if (rc) {
		pr_err("request USB_HOST_EN failed, rc=%d\n", rc);
		return -ENODEV;
	}
	rc = gpio_direction_output(1, 0);
	if (rc) {
		pr_err("set output USB_HOST_EN failed, rc=%d\n", rc);
		return -ENODEV;
	}
	rc = gpio_export(1, 0);
	if (rc) {
		pr_err("export USB_HOST_EN failed, rc=%d\n", rc);
		return -ENODEV;
	}
	rc = gpio_request(75, "Factory Kill Disable");
	if (rc) {
		pr_err("request Factory Kill Disable failed, rc=%d\n", rc);
		return -ENODEV;
	}
	/* Set Factory Kill Disable to OUTPUT/LOW to disable the circuitry that
	 * powers down upon factory cable removal.
	 *
	 * Disabling is only needed for HW revision 'A' hardware:
	 * Teufel:   {'A', "M1",   0x2100}
	 * Asanti:   {'A', "M1",   0x2100}
	 * Becker:   {'A', "M1",   0x2100}
	 * Qinara:   {'A', "P1",   0x8100}
	 * Vanquish: {'A', "P1",   0x8100}
	 * Volta:    {'A', "M1",   0x2100}
	 */
	if ((system_rev == HWREV_M1) || (system_rev == HWREV_P1)) {
		pr_info("Disabling Factory Kill.\n");
		rc = gpio_direction_output(75, 0);
		if (rc) {
			pr_err("Factory Kill Disable, output error: %d\n", rc);
			return -ENODEV;
		}
		rc = gpio_export(75, 0);
		if (rc) {
			pr_err("Factory Kill Disable, export error: %d\n", rc);
			return -ENODEV;
		}
	} else {
		pr_info("Enabling Factory Kill.\n");
		rc = gpio_direction_output(75, 1);
		if (rc) {
			pr_err("Factory Kill Enable, output error: %d\n", rc);
			return -ENODEV;
		}
		rc = gpio_export(75, 0);
		if (rc) {
			pr_err("Factory Kill Enable, export error: %d\n", rc);
			return -ENODEV;
		}
	}

	rc = gpio_request(PM8921_GPIO_PM_TO_SYS(36), "SIM_DET");
	if (rc) {
		pr_err("request gpio SIM_DET failed, rc=%d\n", rc);
		return -ENODEV;
	}
	rc = gpio_direction_input(PM8921_GPIO_PM_TO_SYS(36));
	if (rc) {
		pr_err("set output SIM_DET failed, rc=%d\n", rc);
		return -ENODEV;
	}
	rc = gpio_export(PM8921_GPIO_PM_TO_SYS(36), 0);
	if (rc) {
		pr_err("export gpio SIM_DET failed, rc=%d\n", rc);
		return -ENODEV;
	}
	return 0;
}

static struct msm_spi_platform_data msm8960_qup_spi_gsbi1_pdata = {
	.max_clock_speed = 15060000,
};

static struct led_info msm8960_mmi_button_backlight = {
	.name = "button-backlight",
	.default_trigger = "none",
};


#define EXPECTED_MBM_PROTOCOL_VERSION 1
static uint32_t mbm_protocol_version;

static void __init msm8960_mmi_init(void)
{
	if (mbm_protocol_version == 0)
		pr_err("ERROR: ATAG MBM_PROTOCOL_VERSION is not present."
			" Bootloader update is required\n");
	else if (EXPECTED_MBM_PROTOCOL_VERSION != mbm_protocol_version)
		arch_reset(0, "mbm_proto_ver_mismatch");

	if (meminfo_init(SYS_MEMORY, SZ_256M) < 0)
		pr_err("meminfo_init() failed!\n");

	msm8960_init_rpm();

	pmic_reset_irq = PM8921_IRQ_BASE + PM8921_RESOUT_IRQ;
	regulator_suppress_info_printing();
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");
	msm8960_init_regulators();
	msm_clock_init(&msm8960_clock_init_data);

	gpiomux_init(use_mdp_vsync);
	mot_gpiomux_init(keypad_mode);
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	msm8960_init_hdmi(&hdmi_msm_device, &hdmi_msm_data);
#endif

	pm8921_init(keypad_data, boot_mode_is_factory(), 0, 0);

	/* Init the bus, but no devices at this time */
	msm8960_spi_init(&msm8960_qup_spi_gsbi1_pdata, NULL, 0);

#ifdef CONFIG_EMU_DETECTION
	/* This function has to be called prior I2C init */
	mot_init_emu_det_resources(otg_control_data);
#endif
	msm8960_i2c_init(400000);
	msm8960_gfx_init();
	msm8960_spm_init();
	msm8960_init_buses();

	/* Setup correct button backlight LED name */
	pm8xxx_set_led_info(1, &msm8960_mmi_button_backlight);

#ifdef CONFIG_TOUCHSCREEN_CYTTSP3
	if (msm8960_i2c_devices[TOUCHSCREEN_CYTTSP3].enabled)
		mot_setup_touch_cyttsp3();
#endif
#ifdef CONFIG_TOUCHSCREEN_ATMXT
	if (msm8960_i2c_devices[TOUCHSCREEN_ATMEL].enabled)
		mot_setup_touch_atmxt();
#endif
#ifdef CONFIG_TOUCHSCREEN_MELFAS100_TS
	if (msm8960_i2c_devices[TOUCHSCREEN_MELFAS100_TS].enabled)
		melfas_ts_platform_init();
#endif
#ifdef CONFIG_VIB_TIMED
	mmi_vibrator_init();
#endif
	mmi_keypad_init(keypad_mode);

	platform_add_devices(msm_footswitch_devices,
		msm_num_footswitch_devices);
	msm8960_add_common_devices(msm_fb_detect_panel);

	pm8921_gpio_mpp_init(pm8921_gpios, pm8921_gpios_size,
							pm8921_mpps, ARRAY_SIZE(pm8921_mpps));

	msm8960_init_usb(msm_hsusb_vbus_power);

#ifdef CONFIG_EMU_DETECTION
	mot_init_emu_detection(otg_control_data);
#endif

	platform_add_devices(mmi_devices, ARRAY_SIZE(mmi_devices));
#ifdef CONFIG_MSM_CAMERA
	msm8960_init_cam();
#endif
	msm8960_init_cam_flash(flash_hw_enable);
	msm8960_init_mmc(sdc_detect_gpio);
	acpuclk_init(&acpuclk_8960_soc_data);
	register_i2c_devices();
	msm_fb_add_devices();
	msm8960_init_slim();
	msm8960_init_dsps();

#ifdef CONFIG_PN544	/* NFC */
	msm8960_pn544_init();
#endif

	msm8960_sensors_init();
	msm8960_pm_init(RPM_APCC_CPU0_WAKE_UP_IRQ);
	mot_tcmd_export_gpio();

	change_memory_power = &msm8960_change_memory_power;
	BUG_ON(msm_pm_boot_init(MSM_PM_BOOT_CONFIG_TZ, NULL));
}

static int __init mot_parse_atag_baseband(const struct tag *tag)
{
	const struct tag_baseband *baseband_tag = &tag->u.baseband;

	pr_info("%s: %s\n", __func__, baseband_tag->baseband);

	return 0;
}
__tagtable(ATAG_BASEBAND, mot_parse_atag_baseband);

static int __init mot_parse_atag_display(const struct tag *tag)
{
	const struct tag_display *display_tag = &tag->u.display;
	strncpy(panel_name, display_tag->display, PANEL_NAME_MAX_LEN);
	panel_name[PANEL_NAME_MAX_LEN] = '\0';
	pr_info("%s: %s\n", __func__, panel_name);
	return 0;
}
__tagtable(ATAG_DISPLAY, mot_parse_atag_display);

static int __init mot_parse_atag_mbm_protocol_version(const struct tag *tag)
{
	const struct tag_mbm_protocol_version *mbm_protocol_version_tag =
		&tag->u.mbm_protocol_version;
	pr_info("%s: 0x%x\n", __func__, mbm_protocol_version_tag->value);
	mbm_protocol_version = mbm_protocol_version_tag->value;
	return 0;
}
__tagtable(ATAG_MBM_PROTOCOL_VERSION, mot_parse_atag_mbm_protocol_version);

static void __init set_emu_detection_resource(const char *res_name, int value)
{
	struct resource *res = platform_get_resource_byname(
				&emu_det_device,
				IORESOURCE_IO, res_name);
	if (res) {
		res->start = res->end = value;
		pr_info("resource (%s) set to %d\n",
				res_name, value);
	} else
		pr_err("cannot set resource (%s)\n", res_name);
}

/* process flat device tree for hardware configuration */
static int __init parse_tag_flat_dev_tree_address(const struct tag *tag)
{
	struct tag_flat_dev_tree_address *fdt_addr =
		(struct tag_flat_dev_tree_address *)&tag->u.fdt_addr;

	if (fdt_addr->size)
		fdt_start_address = (u32)phys_to_virt(fdt_addr->address);

	pr_info("flat_dev_tree_address=0x%08x, flat_dev_tree_size == 0x%08X\n",
			fdt_addr->address, fdt_addr->size);

	return 0;
}
__tagtable(ATAG_FLAT_DEV_TREE_ADDRESS, parse_tag_flat_dev_tree_address);

static void __init mmi_init_early(void)
{
	msm8960_allocate_memory_regions();

	if (fdt_start_address) {
		pr_info("Unflattening device tree: 0x%08x\n",
				fdt_start_address);
		initial_boot_params =
			(struct boot_param_header *)fdt_start_address;
		unflatten_device_tree();
	}
}

static __init void teufel_init(void)
{
	if (system_rev <= HWREV_M1) {
		sdc_detect_gpio = 22;
		pm8921_gpios = pm8921_gpios_teufel_m1;
		pm8921_gpios_size = ARRAY_SIZE(pm8921_gpios_teufel_m1);
	} else {
		pm8921_gpios = pm8921_gpios_teufel_m2;
		pm8921_gpios_size = ARRAY_SIZE(pm8921_gpios_teufel_m2);
	}

	flash_hw_enable = 2;

	if (is_smd()) {
		ENABLE_I2C_DEVICE(TOUCHSCREEN_MELFAS100_TS);
	} else {
		ENABLE_I2C_DEVICE(TOUCHSCREEN_CYTTSP3);
		ENABLE_I2C_DEVICE(BACKLIGHT_LM3532);
	}

#ifdef CONFIG_EMU_DETECTION
	if (system_rev <= HWREV_P2)
		otg_control_data = NULL;
	else
		set_emu_detection_resource("EMU_ID_EN_GPIO", 94);
#endif
	ENABLE_I2C_DEVICE(CAMERA_MSM);
#ifdef CONFIG_INPUT_CT406
	if (system_rev >= HWREV_P2)
		ENABLE_I2C_DEVICE(ALS_CT406);
#endif
#ifdef CONFIG_PN544
	ENABLE_I2C_DEVICE(NFC_PN544);
#endif
	ENABLE_I2C_DEVICE(CAMERA_FLASH_MSM);

	msm8960_mmi_init();
}

MACHINE_START(TEUFEL, "Teufel")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = teufel_init,
	.init_early = mmi_init_early,
	.init_very_early = msm8960_early_memory,
MACHINE_END

static __init void qinara_init(void)
{
	/* For qinara HW, to improve the signal strength
	 * extra settings to ulpi_registers are needed
	 */
	msm_otg_pdata.phy_init_seq = phy_settings;
#ifdef CONFIG_EMU_DETECTION
	if (system_rev < HWREV_P2)
		otg_control_data = NULL;
#endif
	flash_hw_enable = 2;

	ENABLE_I2C_DEVICE(TOUCHSCREEN_CYTTSP3);
	ENABLE_I2C_DEVICE(CAMERA_MSM);
	if (system_rev >= HWREV_P2)
		ENABLE_I2C_DEVICE(ALS_CT406);
	ENABLE_I2C_DEVICE(CAMERA_FLASH_MSM);
	ENABLE_I2C_DEVICE(BACKLIGHT_LM3532);
#ifdef CONFIG_PN544
	ENABLE_I2C_DEVICE(NFC_PN544);
#endif

	msm8960_mmi_init();
}

MACHINE_START(QINARA, "Qinara")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = qinara_init,
	.init_early = mmi_init_early,
	.init_very_early = msm8960_early_memory,
MACHINE_END

static __init void vanquish_init(void)
{
#ifdef CONFIG_EMU_DETECTION
	if (system_rev < HWREV_P1B2)
		otg_control_data = NULL;
#endif
	flash_hw_enable = 2;

	ENABLE_I2C_DEVICE(TOUCHSCREEN_MELFAS100_TS);
	ENABLE_I2C_DEVICE(CAMERA_MSM);
	ENABLE_I2C_DEVICE(CAMERA_FLASH_MSM);
	ENABLE_I2C_DEVICE(ALS_CT406);
#ifdef CONFIG_PN544
	ENABLE_I2C_DEVICE(NFC_PN544);
#endif
	use_mdp_vsync = MDP_VSYNC_DISABLED;
	msm8960_mmi_init();
}

MACHINE_START(VANQUISH, "Vanquish")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = vanquish_init,
	.init_early = mmi_init_early,
	.init_very_early = msm8960_early_memory,
MACHINE_END

static __init void volta_init(void)
{
	ENABLE_I2C_DEVICE(TOUCHSCREEN_CYTTSP3);
	ENABLE_I2C_DEVICE(CAMERA_MSM);
	ENABLE_I2C_DEVICE(ALS_CT406);
	ENABLE_I2C_DEVICE(BACKLIGHT_LM3532);
#ifdef CONFIG_PN544
	ENABLE_I2C_DEVICE(NFC_PN544);
#endif

	msm8960_mmi_init();
}

MACHINE_START(VOLTA, "Volta")
    .map_io = msm8960_map_io,
    .reserve = msm8960_reserve,
    .init_irq = msm8960_init_irq,
    .timer = &msm_timer,
    .init_machine = volta_init,
	.init_early = mmi_init_early,
	.init_very_early = msm8960_early_memory,
MACHINE_END

static __init void becker_init(void)
{
	ENABLE_I2C_DEVICE(TOUCHSCREEN_ATMEL);
	ENABLE_I2C_DEVICE(CAMERA_MSM);
	ENABLE_I2C_DEVICE(ALS_CT406);
	ENABLE_I2C_DEVICE(BACKLIGHT_LM3532);
#ifdef CONFIG_PN544
	ENABLE_I2C_DEVICE(NFC_PN544);
#endif

	msm8960_mmi_init();
}

MACHINE_START(BECKER, "Becker")
    .map_io = msm8960_map_io,
    .reserve = msm8960_reserve,
    .init_irq = msm8960_init_irq,
    .timer = &msm_timer,
    .init_machine = becker_init,
	.init_early = mmi_init_early,
    .init_very_early = msm8960_early_memory,
MACHINE_END

static __init void asanti_init(void)
{
	otg_control_data = NULL;
	pm8921_gpios = pm8921_gpios_asanti;
	pm8921_gpios_size = ARRAY_SIZE(pm8921_gpios_asanti);

	ENABLE_I2C_DEVICE(TOUCHSCREEN_ATMEL);
	ENABLE_I2C_DEVICE(CAMERA_MSM);
	ENABLE_I2C_DEVICE(ALS_CT406);
	ENABLE_I2C_DEVICE(BACKLIGHT_LM3532);
#ifdef CONFIG_PN544
	ENABLE_I2C_DEVICE(NFC_PN544);
#endif
	keypad_data = &mmi_qwerty_keypad_data;
	keypad_mode = MMI_KEYPAD_RESET|MMI_KEYPAD_SLIDER;

	/* Enable keyboard backlight */
	strncpy((char *)&mp_lm3532_pdata.ctrl_b_name, "keyboard-backlight",
		sizeof(mp_lm3532_pdata.ctrl_b_name)-1);
	mp_lm3532_pdata.ctrl_b_usage = LM3532_LED_DEVICE;

	msm8960_mmi_init();
}

MACHINE_START(ASANTI, "Asanti")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = asanti_init,
	.init_early = mmi_init_early,
	.init_very_early = msm8960_early_memory,
MACHINE_END