// SPDX-License-Identifier: GPL-2.0+
/*
 * Marvell PXA988 / PXA986 SoC support, for U-Boot chain-loaded by a vendor
 * bootloader (Samsung S-Boot) that has already initialised DRAM, PLLs, the
 * PMIC and the display.
 *
 * Register layout taken from the Marvell PXA988 vendor kernel (3.4) and the
 * goyawifi Linux 4.19 port (drivers/clk/mmp/clk-of-pxa988.c,
 * arch/arm/mach-mmp/pxa988-dt.c).
 */

#include <env.h>
#include <init.h>
#include <log.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/system.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/string.h>

/* Main / application processor power management units */
#define MPMU_BASE		0xd4050000
#define MPMU_UART_PLL		(MPMU_BASE + 0x0014)
#define MPMU_CPRR		(MPMU_BASE + 0x0020)
#define MPMU_WDTPCR		(MPMU_BASE + 0x0200)
#define MPMU_APRR		(MPMU_BASE + 0x1020)
#define  MPMU_APRR_CPR		BIT(0)
#define  MPMU_APRR_WDTR		BIT(4)

#define APMU_BASE		0xd4282800
#define APMU_SDH0		(APMU_BASE + 0x0054)
#define APMU_SDH1		(APMU_BASE + 0x0058)
#define APMU_SDH2		(APMU_BASE + 0x00e0)

/* APB clock unit */
#define APBC_BASE		0xd4015000
#define APBC_UART1		(APBC_BASE + 0x0000)	/* 0xd4017000, "AP UART0" */
#define APBC_UART2		(APBC_BASE + 0x0004)	/* 0xd4018000 */
#define APBC_TIMERS0		(APBC_BASE + 0x0034)
#define  APBC_APBCLK		BIT(0)
#define  APBC_FNCLK		BIT(1)
#define  APBC_RST		BIT(2)
#define  APBC_FNCLKSEL(x)	(((x) & 7) << 4)

/* AP watchdog (TMR_W* registers of the watchdog timer block) */
#define WDT_BASE		0xd4080000
#define WDT_WMER		(WDT_BASE + 0x64)
#define WDT_WMR			(WDT_BASE + 0x68)
#define WDT_WVR			(WDT_BASE + 0x6c)
#define WDT_WSR			(WDT_BASE + 0x70)
#define WDT_WFAR		(WDT_BASE + 0x9c)
#define WDT_WSAR		(WDT_BASE + 0xa0)

/* RTC backup register 0, read by S-Boot to pick the boot mode */
#define RTC_BR0			0xd4010014
#define RTC_BR0_RECOVERY	0x7265636f		/* "reco" */

/* PL310 L2 cache controller */
#define L2X0_BASE		0xd1dfb000
#define L2X0_CTRL		(L2X0_BASE + 0x100)
#define L2X0_CACHE_SYNC		(L2X0_BASE + 0x730)
#define L2X0_CLEAN_INV_WAY	(L2X0_BASE + 0x7fc)

/* SDH functional clock: pll1_416 / (7 + 1) = 52 MHz, see clk-of-pxa988.c */
#define SDH_CLK_RST_N		BIT(0)
#define SDH_CLK_AXI_RST_N	BIT(1)
#define SDH_CLK_ACLK_EN		BIT(3)
#define SDH_CLK_FCLK_EN		BIT(4)
#define SDH_CLK_SEL_624		BIT(6)
#define SDH_CLK_DIV(x)		(((x) & 7) << 8)
#define SDH_CLK_FC		BIT(11)

/* r0..r2 as handed over by the previous bootloader, see lowlevel_pxa988.S */
extern u32 pxa988_prev_bl_regs[3];

static void pxa988_l2_disable(void)
{
	/*
	 * S-Boot may leave the PL310 enabled. U-Boot runs with
	 * SYS_L2CACHE_OFF, so flush it and turn it off before we enable our
	 * own L1 caches; Linux re-enables it later.
	 */
	if (!(readl(L2X0_CTRL) & 1))
		return;

	writel(0xffff, L2X0_CLEAN_INV_WAY);
	while (readl(L2X0_CLEAN_INV_WAY) & 0xffff)
		;
	writel(0, L2X0_CACHE_SYNC);
	writel(0, L2X0_CTRL);
	dsb();
}

static void pxa988_uart_clk_init(void)
{
	/* uart_pll = pll1_4 * 1536 / (8125 * 2) = 14.7456 MHz */
	writel((8125 << 16) | 1536, MPMU_UART_PLL);

	/* FNCLKSEL 1 = uart_pll; enable APB + functional clock, out of reset */
	writel(APBC_APBCLK | APBC_FNCLK | APBC_FNCLKSEL(1), APBC_UART1);
}

static void pxa988_timer_clk_init(void)
{
	/* 3.25 MHz timer clock (FNCLKSEL 3), as programmed by Linux */
	writel(APBC_APBCLK | APBC_RST, APBC_TIMERS0);
	writel(APBC_APBCLK | APBC_FNCLK | APBC_FNCLKSEL(3), APBC_TIMERS0);
}

static void pxa988_sdh_clk_init(void)
{
	const u32 val = SDH_CLK_RST_N | SDH_CLK_AXI_RST_N | SDH_CLK_FCLK_EN |
			SDH_CLK_DIV(7);

	/*
	 * Put every SDHCI on a known 52 MHz base clock so the DT
	 * "clock-frequency" is true regardless of what S-Boot programmed.
	 * The shared SDH AXI clock enable lives in the SDH0 register.
	 */
	writel(val | SDH_CLK_ACLK_EN | SDH_CLK_FC, APMU_SDH0);
	writel(val | SDH_CLK_FC, APMU_SDH1);
	writel(val | SDH_CLK_FC, APMU_SDH2);
	udelay(10);
}

int arch_cpu_init(void)
{
	pxa988_l2_disable();
	pxa988_uart_clk_init();
	pxa988_timer_clk_init();

	return 0;
}

int arch_misc_init(void)
{
	pxa988_sdh_clk_init();

	return 0;
}

void reset_cpu(void)
{
	u32 val;

	/* Hold the CP and reset the MSA so it cannot cancel the WDT reset */
	setbits_le32(MPMU_APRR, MPMU_APRR_CPR);
	udelay(10);
	setbits_le32(MPMU_CPRR, BIT(2) | BIT(3));
	udelay(10);

	/* Reset and enable the watchdog clock */
	writel(0x7, MPMU_WDTPCR);
	readl(MPMU_WDTPCR);
	writel(0x3, MPMU_WDTPCR);
	readl(MPMU_WDTPCR);

	/* Enable watchdog reset */
	writel(0xbaba, WDT_WFAR);
	writel(0xeb10, WDT_WSAR);
	writel(0x3, WDT_WMER);

	/* Let the WDT actually reset the AP */
	setbits_le32(MPMU_APRR, MPMU_APRR_WDTR);

	/* Clear status and arm the match register a few ticks out */
	writel(0xbaba, WDT_WFAR);
	writel(0xeb10, WDT_WSAR);
	writel(0, WDT_WSR);

	val = readl(WDT_WVR);
	writel(0xbaba, WDT_WFAR);
	writel(0xeb10, WDT_WSAR);
	writel((val + 0x20) & 0xffff, WDT_WMR);

	while (1)
		;
}

/* Ask S-Boot to boot the RECOVERY partition on the next reset */
void pxa988_set_reboot_recovery(void)
{
	writel(RTC_BR0_RECOVERY, RTC_BR0);
}

/*
 * Walk the ATAG list S-Boot passed in r2 and export useful bits to the
 * environment: the command line (it carries androidboot.mode, lpm_boot,
 * the serial number, ...) and the location of the ATAGs themselves.
 */
static void pxa988_save_prev_bl_atags(void)
{
	u32 addr = pxa988_prev_bl_regs[2];
	struct tag *t;
	int n = 0;

	env_set_hex("prevbl_r1", pxa988_prev_bl_regs[1]);
	env_set_hex("prevbl_r2", addr);

	/* Must be 4-byte aligned and within the first GiB of DRAM */
	if (!addr || (addr & 3) || addr >= 0x40000000)
		return;

	t = (struct tag *)(uintptr_t)addr;
	if (t->hdr.tag != ATAG_CORE)
		return;

	for (; t->hdr.size && n < 64; t = tag_next(t), n++) {
		if (t->hdr.tag == ATAG_NONE)
			break;
		if (t->hdr.tag == ATAG_CMDLINE) {
			env_set("prevbl_cmdline", t->u.cmdline.cmdline);
			if (strstr(t->u.cmdline.cmdline, "lpm_boot=1"))
				env_set("prevbl_lpm_boot", "1");
		}
		if (t->hdr.tag == ATAG_INITRD2)
			env_set_hex("prevbl_initrd_start_addr", t->u.initrd.start);
	}
}

int pxa988_late_init(void)
{
	pxa988_save_prev_bl_atags();

	return 0;
}
