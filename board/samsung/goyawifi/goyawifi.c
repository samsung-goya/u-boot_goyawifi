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
#include <time.h>
#include <vsprintf.h>

/* Marvell 88PM822 PMIC on TWSI2 */
#define PM822_I2C_BUS			2
#define PM822_I2C_ADDR			0x30
#define PM800_WAKEUP1			0x0d
#define  PM800_SW_PDOWN			BIT(5)
#define PM822_GENERAL_USE_REG		0xef
#define  PM822_SHUTDOWN_BY_POWEROFF	0xa0

#define APBCP_TWSI2			0xd403b028

/* Keypad controller (S-Boot leaves the keys in keypad mode) */
#define KPC_PC				0xd4012000
#define APBC_KPC			0xd4015030
#define APMU_WAKE_CLR			0xd428287c
#define  APMU_WAKE_CLR_KPC		BIT(3)

/* MFPR pads */
#define MFPR(off)			(0xd401e000 + (off))
#define MFPR_GPIO1			0x00e0	/* Volume Up */
#define MFPR_GPIO2			0x00e4	/* Volume Down */
#define MFPR_GPIO13			0x0110	/* Home */
#define  MFPR_AF_MASK			0x7
#define  MFPR_PULL_MASK			(BIT(15) | BIT(14) | BIT(13))
#define  MFPR_PULL_UP			(BIT(15) | BIT(14))

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

/*
 * S-Boot drives the volume keys through the PXA keypad controller
 * (pads in KP_MKIN function). Do what the vendor kernel does for
 * gpio-keys mode: stop the keypad controller and its clock, then put the
 * pads into GPIO mode with the internal pull-ups the volume keys rely on
 * (Home has an external pull-up). U-Boot's pinctrl-single only muxes, it
 * does not apply "pinctrl-single,bias-pullup".
 */
static void goyawifi_keys_init(void)
{
	if (readl(APBC_KPC) & BIT(0)) {
		writel(0, KPC_PC);
		writel(0, APBC_KPC);
	}
	setbits_le32(APMU_WAKE_CLR, APMU_WAKE_CLR_KPC);

	clrsetbits_le32(MFPR(MFPR_GPIO1), MFPR_PULL_MASK | MFPR_AF_MASK,
			MFPR_PULL_UP);
	clrsetbits_le32(MFPR(MFPR_GPIO2), MFPR_PULL_MASK | MFPR_AF_MASK,
			MFPR_PULL_UP);
	clrsetbits_le32(MFPR(MFPR_GPIO13), MFPR_PULL_MASK | MFPR_AF_MASK, 0);
	udelay(100);	/* let the pull-ups settle before the first read */
}

int board_init(void)
{
	goyawifi_keys_init();

	return 0;
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

/*
 * keytest [seconds]: show the key pad setup and report every GPIO level
 * change across all 128 GPIOs, so wrongly wired keys show up on screen.
 */
#define GPIO_GPLR(bank)	(0xd4019000 + ((bank) < 3 ? (bank) * 4 : 0x100 + ((bank) - 3) * 4))

static int do_keytest(struct cmd_tbl *cmdtp, int flag, int argc,
		      char *const argv[])
{
	ulong secs = argc > 1 ? simple_strtoul(argv[1], NULL, 10) : 10;
	ulong start = get_timer(0);
	u32 old[4], now;
	int bank, bit;

	printf("MFPR GPIO1=%08x GPIO2=%08x GPIO13=%08x KPC=%08x/%08x\n",
	       readl(MFPR(MFPR_GPIO1)), readl(MFPR(MFPR_GPIO2)),
	       readl(MFPR(MFPR_GPIO13)), readl(APBC_KPC),
	       readl(APBC_KPC) & BIT(0) ? readl(KPC_PC) : 0);
	for (bank = 0; bank < 4; bank++)
		old[bank] = readl(GPIO_GPLR(bank));
	printf("GPLR %08x %08x %08x %08x\n", old[0], old[1], old[2], old[3]);
	printf("Press keys now (%lus)...\n", secs);

	while (get_timer(start) < secs * 1000) {
		for (bank = 0; bank < 4; bank++) {
			now = readl(GPIO_GPLR(bank));
			for (bit = 0; bit < 32; bit++)
				if ((now ^ old[bank]) & BIT(bit))
					printf("GPIO%d -> %d\n", bank * 32 + bit,
					       !!(now & BIT(bit)));
			old[bank] = now;
		}
		mdelay(20);
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(keytest, 2, 0, do_keytest,
	   "show GPIO level changes (key wiring test)", "[seconds]");
