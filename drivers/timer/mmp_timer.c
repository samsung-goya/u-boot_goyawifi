// SPDX-License-Identifier: GPL-2.0+
/*
 * Marvell MMP (PXA168 / PXA910 / PXA988) APB timer
 *
 * Each timer block holds three 32-bit up-counters. Counter 1 is run in
 * free-running mode as the U-Boot tick source, mirroring what Linux does in
 * arch/arm/mach-mmp/time.c. The count register itself must not be read
 * directly (metastability, see the PXA168 manual); a read is requested via
 * CVWR and the latched value is read back after a few bus cycles.
 */

#include <dm.h>
#include <timer.h>
#include <asm/io.h>
#include <linux/bitops.h>

#define TMR_CCR			0x00
#define  TMR_CCR_CS_0(x)	(((x) & 0x3) << 0)
#define  TMR_CCR_CS_1(x)	(((x) & 0x7) << 2)
#define TMR_PLCR(n)		(0x58 + ((n) << 2))
#define TMR_IER(n)		(0x40 + ((n) << 2))
#define TMR_ICR(n)		(0x74 + ((n) << 2))
#define TMR_CER			0x84
#define TMR_CMR			0x88
#define TMR_CVWR(n)		(0xa4 + ((n) << 2))

#define MMP_TIMER_CNT		1

struct mmp_timer_priv {
	void __iomem *base;
};

static u32 mmp_timer_read(void __iomem *base)
{
	u32 val = 0;
	int delay = 3;

	writel(1, base + TMR_CVWR(MMP_TIMER_CNT));
	while (delay--)
		val = readl(base + TMR_CVWR(MMP_TIMER_CNT));

	return val;
}

static u64 mmp_timer_get_count(struct udevice *dev)
{
	struct mmp_timer_priv *priv = dev_get_priv(dev);

	return timer_conv_64(mmp_timer_read(priv->base));
}

static void mmp_timer_hw_init(void __iomem *base)
{
	u32 cer = readl(base + TMR_CER);

	/* Already free-running? Leave it alone so time keeps going up. */
	if ((cer & BIT(MMP_TIMER_CNT)) &&
	    (readl(base + TMR_CMR) & BIT(MMP_TIMER_CNT)))
		return;

	writel(cer & ~BIT(MMP_TIMER_CNT), base + TMR_CER);
	clrbits_le32(base + TMR_CCR, TMR_CCR_CS_1(3));	/* fast clock */
	setbits_le32(base + TMR_CMR, BIT(MMP_TIMER_CNT));	/* free-running */
	writel(0, base + TMR_PLCR(MMP_TIMER_CNT));
	writel(0x7, base + TMR_ICR(MMP_TIMER_CNT));
	writel(0, base + TMR_IER(MMP_TIMER_CNT));
	writel(cer | BIT(MMP_TIMER_CNT), base + TMR_CER);
}

static int mmp_timer_probe(struct udevice *dev)
{
	struct timer_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct mmp_timer_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base)
		return -EINVAL;

	if (!uc_priv->clock_rate)
		uc_priv->clock_rate = dev_read_u32_default(dev, "clock-frequency",
							   3250000);

	mmp_timer_hw_init(priv->base);

	return 0;
}

static const struct timer_ops mmp_timer_ops = {
	.get_count = mmp_timer_get_count,
};

static const struct udevice_id mmp_timer_ids[] = {
	{ .compatible = "mrvl,mmp-timer" },
	{ }
};

U_BOOT_DRIVER(mmp_timer) = {
	.name		= "mmp_timer",
	.id		= UCLASS_TIMER,
	.of_match	= mmp_timer_ids,
	.probe		= mmp_timer_probe,
	.ops		= &mmp_timer_ops,
	.priv_auto	= sizeof(struct mmp_timer_priv),
	.flags		= DM_FLAG_PRE_RELOC,
};
