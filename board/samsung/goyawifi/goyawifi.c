// SPDX-License-Identifier: GPL-2.0+
/*
 * Samsung Galaxy Tab 3 Lite 7.0 Wi-Fi (SM-T110, goyawifi)
 */

#include <button.h>
#include <command.h>
#include <dm.h>
#include <env.h>
#include <i2c.h>
#include <init.h>
#include <log.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>

/* Marvell 88PM822 PMIC on TWSI2 */
#define PM822_I2C_BUS			2
#define PM822_I2C_ADDR			0x30
#define PM800_WAKEUP1			0x0d
#define  PM800_SW_PDOWN			BIT(5)
#define PM822_GENERAL_USE_REG		0xef
#define  PM822_SHUTDOWN_BY_POWEROFF	0xa0

#define APBCP_TWSI2			0xd403b028

int pxa988_late_init(void);
void pxa988_set_reboot_recovery(void);

static bool goyawifi_button_pressed(const char *label)
{
	struct udevice *dev;

	if (!IS_ENABLED(CONFIG_BUTTON))
		return false;
	if (button_get_by_label(label, &dev))
		return false;

	return button_get_state(dev) == BUTTON_ON;
}

int board_late_init(void)
{
	pxa988_late_init();

	/*
	 * Key combos checked once at boot:
	 *   Volume Down  -> stay in U-Boot (bootmenu / console)
	 *   Volume Up    -> reboot into the stock RECOVERY partition
	 */
	if (goyawifi_button_pressed("Volume Down")) {
		log_info("Volume Down held: stopping autoboot\n");
		env_set("bootdelay", "-1");
		env_set("goyawifi_key", "voldown");
	} else if (goyawifi_button_pressed("Volume Up")) {
		env_set("goyawifi_key", "volup");
	}

	return 0;
}

static int do_reboot_recovery(struct cmd_tbl *cmdtp, int flag, int argc,
			      char *const argv[])
{
	pxa988_set_reboot_recovery();
	do_reset(NULL, 0, 0, NULL);

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(reboot_recovery, 1, 0, do_reboot_recovery,
	   "reboot into the Samsung RECOVERY partition (via S-Boot)", "");

int do_poweroff(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct udevice *dev;
	int ret, val;

	/* S-Boot talks to the PMIC, but make sure the TWSI2 clock is running */
	clrsetbits_le32(APBCP_TWSI2, BIT(2), BIT(0) | BIT(1));

	ret = i2c_get_chip_for_busnum(PM822_I2C_BUS, PM822_I2C_ADDR, 1, &dev);
	if (ret) {
		printf("88PM822 not found (%d)\n", ret);
		return CMD_RET_FAILURE;
	}

	/* Tell S-Boot why we went down (so it does not show the charger UI) */
	dm_i2c_reg_write(dev, PM822_GENERAL_USE_REG, PM822_SHUTDOWN_BY_POWEROFF);

	val = dm_i2c_reg_read(dev, PM800_WAKEUP1);
	if (val < 0)
		return CMD_RET_FAILURE;
	dm_i2c_reg_write(dev, PM800_WAKEUP1, val | PM800_SW_PDOWN);

	mdelay(1000);
	printf("Still powered after poweroff request\n");

	return CMD_RET_FAILURE;
}
