/*
 * Copyright (C) 2011 Motorola Mobility, Inc.
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
 * Based on ds2780-battery.c which is:
 * Copyright (C) 2010 Indesign, LLC
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/param.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/power/mmi-battery.h>
#include <linux/printk.h>
#include <linux/idr.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "../w1/w1.h"
#include "../w1/slaves/w1_ds2502.h"

#define MAX_EEPROM_READ_CNT 5
#define MAX_CPYRT_LEN 25

struct mmi_battery_info {
	struct device *dev;
	struct w1_slave *w1_dev;
	unsigned char eeprom[EEPROM_SIZE];
	unsigned char uid[UID_SIZE];
	struct delayed_work work;
	unsigned char eeprom_read_cnt;
	struct mmi_battery_cell cell_info;
};

static struct mmi_battery_info *the_batt;

struct mmi_battery_cell *mmi_battery_get_info(void)
{
	if (the_batt)
		return &(the_batt->cell_info);
	else
		return NULL;
}
EXPORT_SYMBOL(mmi_battery_get_info);

static unsigned char mmi_battery_calc_checksum(unsigned char *data_start,
						 unsigned char data_size)
{
	unsigned char index;
	unsigned char checksum = 0;

	for (index = 0; index < data_size; index++)
		checksum += data_start[index];

	return checksum;
}

static unsigned char mmi_battery_is_cpyrght_vld(unsigned char *rom_data)
{
	unsigned int part_1_vld;
	unsigned int part_2_vld;

	part_1_vld =
		strncmp((const char *) &rom_data[ROM_DATA_CPYRGHT_PART_1_BGN],
			CPYRGHT_STRING_PART_1,
			strnlen(CPYRGHT_STRING_PART_1, MAX_CPYRT_LEN)) == 0;

	part_2_vld =
		strncmp((const char *) &rom_data[ROM_DATA_CPYRGHT_PART_2_BGN],
			CPYRGHT_STRING_PART_2,
			strnlen(CPYRGHT_STRING_PART_2, MAX_CPYRT_LEN)) == 0;

	if ((part_1_vld == 1) && (part_2_vld == 1))
		return 1;
	else
		return 0;
}

static int mmi_battery_valid_eeprom(struct mmi_battery_info *dev_info)
{
	unsigned char page_2_checksum;
	unsigned char page_3_checksum;
	unsigned char cpyrght_vld;
	unsigned char *data_start;

	data_start = &dev_info->eeprom[ROM_DATA_PAGE_2_BGN];
	page_2_checksum = mmi_battery_calc_checksum(data_start,
						    ROM_DATA_PAGE_2_END -
						    ROM_DATA_PAGE_2_BGN + 1);
	pr_info("MMI Battery EEPROM Page 2 Checksum = %d\n", page_2_checksum);

	data_start = &dev_info->eeprom[ROM_DATA_PAGE_3_BGN];
	page_3_checksum = mmi_battery_calc_checksum(data_start,
						    ROM_DATA_PAGE_3_END -
						    ROM_DATA_PAGE_3_BGN + 1);
	pr_info("MMI Battery EEPROM Page 3 Checksum = %d\n", page_3_checksum);

	cpyrght_vld = mmi_battery_is_cpyrght_vld(dev_info->eeprom);
	pr_info("MMI Battery EEPROM Copyright Valid = %d\n", cpyrght_vld);

	if ((page_2_checksum == 0) &&
	    (page_3_checksum == 0) &&
	    (cpyrght_vld == 1))
		return MMI_BATTERY_VALID;
	else {
		if (dev_info->eeprom_read_cnt <= MAX_EEPROM_READ_CNT)
			return MMI_BATTERY_UNKNOWN;
		else
			return MMI_BATTERY_INVALID;
	}
}

static void mmi_battery_decode_eeprom(struct mmi_battery_info *dev_info)
{
	unsigned char *data = dev_info->eeprom;

	dev_info->cell_info.capacity =
		(unsigned short)(data[ROM_DATA_CMN_BATT_CPCTY]);
	pr_info("MMI Battery Full Capacity 0x%2X\n",
		dev_info->cell_info.capacity);

	dev_info->cell_info.peak_voltage =
		(unsigned short)(data[ROM_DATA_GSM_TDMA_VLT_MAX]);
	pr_info("MMI Battery Peak Voltage 0x%2X\n",
		dev_info->cell_info.peak_voltage);

	dev_info->cell_info.dc_impedance =
		(unsigned short)(data[ROM_DATA_DC_IMPEDANCE]);
	pr_info("MMI Battery DC Impedance 0x%2X\n",
		dev_info->cell_info.dc_impedance);
}

static void mmi_battery_eeprom_read_work(struct work_struct *work)
{
	struct  mmi_battery_info *dev_info =
		container_of(work, struct mmi_battery_info, work.work);
	int read_size = 0;
	int batt_valid = MMI_BATTERY_UNKNOWN;

	dev_info->eeprom_read_cnt++;

	read_size = w1_ds2502_read_eeprom(dev_info->w1_dev,
					  dev_info->eeprom,
					  EEPROM_SIZE);
	pr_info("MMI Battery EEPROM Read %d Bytes\n", read_size);

	w1_ds2502_read_uid(dev_info->w1_dev, dev_info->uid, UID_SIZE);

	if (read_size != EEPROM_SIZE) {
		if (dev_info->eeprom_read_cnt <= MAX_EEPROM_READ_CNT)
			schedule_delayed_work(&dev_info->work,
					      msecs_to_jiffies(100));
	} else {
		batt_valid = mmi_battery_valid_eeprom(dev_info);
		if (batt_valid == MMI_BATTERY_VALID)
			mmi_battery_decode_eeprom(dev_info);
		else if (dev_info->eeprom_read_cnt <= MAX_EEPROM_READ_CNT)
			schedule_delayed_work(&dev_info->work,
					      msecs_to_jiffies(100));
	}

	dev_info->cell_info.batt_valid = batt_valid;
}

static int __devinit mmi_battery_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mmi_battery_info *dev_info;

	dev_info = kzalloc(sizeof(*dev_info), GFP_KERNEL);
	if (!dev_info) {
		ret = -ENOMEM;
		goto fail;
	}

	platform_set_drvdata(pdev, dev_info);

	dev_info->dev = &pdev->dev;
	dev_info->w1_dev = container_of(pdev->dev.parent, struct w1_slave, dev);

	dev_info->eeprom_read_cnt = 0;
	dev_info->cell_info.batt_valid = MMI_BATTERY_UNKNOWN;

	the_batt = dev_info;

	INIT_DELAYED_WORK(&dev_info->work,  mmi_battery_eeprom_read_work);
	schedule_delayed_work(&dev_info->work, msecs_to_jiffies(100));

fail:
	return ret;
}

static int __devexit mmi_battery_remove(struct platform_device *pdev)
{
	struct mmi_battery_info *dev_info = platform_get_drvdata(pdev);

	kfree(dev_info);
	return 0;
}

MODULE_ALIAS("platform:mmi-battery");

static struct platform_driver mmi_battery_driver = {
	.driver = {
		.name = "mmi-battery",
	},
	.probe	  = mmi_battery_probe,
	.remove   = mmi_battery_remove,
};

static int __init mmi_battery_init(void)
{
	return platform_driver_register(&mmi_battery_driver);
}

static void __exit mmi_battery_exit(void)
{
	platform_driver_unregister(&mmi_battery_driver);
}

module_init(mmi_battery_init);
module_exit(mmi_battery_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Motorola Mobility Inc.");
MODULE_DESCRIPTION("Motorola Mobility Inc. Battery driver");