// SPDX-License-Identifier: GPL-2.0+
/*
 * Marvell PXA988 / PXA986 USB 2.0 UTMI PHY
 *
 * Calibration values match the Samsung goyawifi vendor kernel
 * (board-goya.c usb_phy_init_internal()) and the goyawifi Linux 4.19
 * ChipIdea glue (drivers/usb/chipidea/ci_hdrc_pxa.c).
 */

#include <dm.h>
#include <dm/device_compat.h>
#include <generic-phy.h>
#include <log.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/iopoll.h>

#define PHY_PLL_CTRL1		0x08
#define  PHY_PLL_PU		BIT(13)
#define  PHY_PLL_READY		BIT(15)
#define PHY_ANA_CTRL1		0x34
#define  PHY_ANA_PU		BIT(14)
#define PHY_POWER_CTRL		0x104
#define  PHY_POWER_ON		0x10901003
#define  PHY_POWER_OFF		0x10901000

/* APMU USB clock: release AXI reset and enable the clock */
#define APMU_USB_CLK_EN		0x9

struct pxa988_usb_phy_priv {
	void __iomem *base;
	void __iomem *clk_reg;
};

static const struct {
	u16 offset;
	u16 value;
} pxa988_usb_phy_regs[] = {
	{ 0x04, 0x5af0 },	/* PLLVDD18=1, REFDIV=13, FBDIV=240 */
	{ 0x08, 0x3133 },	/* PU_PLL, LOCK_BYPASS, ICP=1, KVCO=3, CAL12=3 */
	{ 0x10, 0x0188 },	/* IMPCAL_VTH=1, HS/FS_RCAL=8 */
	{ 0x14, 0x07d4 },	/* TXVDD15=1, TXVDD12=3, LOWVDD, AMP=5, PHSEL=4 */
	{ 0x18, 0x0aff },	/* SLEWRATE=2, CAL_DLY=2, FS/HS_DRV=15 */
	{ 0x20, 0xaac1 },	/* SQ_THRESH=12 (goya) */
	{ 0x24, 0x3892 },
	{ 0x28, 0x0105 },
	{ 0x30, 0x0110 },
	{ 0x34, 0x5680 },
	{ 0x3c, 0x2586 },
	{ 0x40, 0xe400 },
	{ 0x58, 0x0000 },
};

static int pxa988_usb_phy_init(struct phy *phy)
{
	struct pxa988_usb_phy_priv *priv = dev_get_priv(phy->dev);
	unsigned int i;
	u16 val;
	int ret;

	if (priv->clk_reg) {
		writel(APMU_USB_CLK_EN, priv->clk_reg);
		readl(priv->clk_reg);
		udelay(20);
	}

	writel(PHY_POWER_ON, priv->base + PHY_POWER_CTRL);

	for (i = 0; i < ARRAY_SIZE(pxa988_usb_phy_regs); i++)
		writew(pxa988_usb_phy_regs[i].value,
		       priv->base + pxa988_usb_phy_regs[i].offset);

	/* The vendor code does not insist on PLL_READY; neither do we */
	ret = readw_poll_timeout(priv->base + PHY_PLL_CTRL1, val,
				 val & PHY_PLL_READY, 200000);
	if (ret)
		dev_warn(phy->dev, "PLL not ready (%04x), continuing\n", val);

	return 0;
}

static int pxa988_usb_phy_exit(struct phy *phy)
{
	struct pxa988_usb_phy_priv *priv = dev_get_priv(phy->dev);

	clrbits_le16(priv->base + PHY_PLL_CTRL1, PHY_PLL_PU);
	clrbits_le16(priv->base + PHY_ANA_CTRL1, PHY_ANA_PU);
	writel(PHY_POWER_OFF, priv->base + PHY_POWER_CTRL);

	return 0;
}

static int pxa988_usb_phy_probe(struct udevice *dev)
{
	struct pxa988_usb_phy_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr_index_ptr(dev, 0);
	if (!priv->base)
		return -EINVAL;

	/* Optional: APMU USB clock register (no PXA988 clock driver yet) */
	priv->clk_reg = dev_read_addr_index_ptr(dev, 1);

	return 0;
}

static const struct phy_ops pxa988_usb_phy_ops = {
	.init	= pxa988_usb_phy_init,
	.exit	= pxa988_usb_phy_exit,
};

static const struct udevice_id pxa988_usb_phy_ids[] = {
	{ .compatible = "marvell,pxa988-usb-phy" },
	{ }
};

U_BOOT_DRIVER(pxa988_usb_phy) = {
	.name		= "pxa988_usb_phy",
	.id		= UCLASS_PHY,
	.of_match	= pxa988_usb_phy_ids,
	.ops		= &pxa988_usb_phy_ops,
	.probe		= pxa988_usb_phy_probe,
	.priv_auto	= sizeof(struct pxa988_usb_phy_priv),
};
