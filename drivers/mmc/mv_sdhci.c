// SPDX-License-Identifier: GPL-2.0+
/*
 * Marvell SD Host Controller Interface
 */

#include <dm.h>
#include <malloc.h>
#include <sdhci.h>
#include <asm/gpio.h>
#include <linux/mbus.h>

#define MVSDH_NAME "mv_sdh"

#define SDHCI_WINDOW_CTRL(win)		(0x4080 + ((win) << 4))
#define SDHCI_WINDOW_BASE(win)		(0x4084 + ((win) << 4))

/* PXAv3 (MMP) vendor registers */
#define SD_CLOCK_BURST_SIZE_SETUP	0x10a
#define  SDCLK_SEL			BIT(8)
#define  SDCLK_DELAY_SHIFT		9
#define  SDCLK_DELAY_MASK		0x1f

enum mv_sdhci_type {
	MV_SDHCI_ARMADA_380,
	MV_SDHCI_PXAV3,
};

struct mv_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
};

static void __maybe_unused sdhci_mvebu_mbus_config(void __iomem *base)
{
	const struct mbus_dram_target_info *dram;
	int i;

	dram = mvebu_mbus_dram_info();

	for (i = 0; i < 4; i++) {
		writel(0, base + SDHCI_WINDOW_CTRL(i));
		writel(0, base + SDHCI_WINDOW_BASE(i));
	}

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		/* Write size, attributes and target id to control register */
		writel(((cs->size - 1) & 0xffff0000) | (cs->mbus_attr << 8) |
		       (dram->mbus_dram_target_id << 4) | 1,
		       base + SDHCI_WINDOW_CTRL(i));

		/* Write base address to base register */
		writel(cs->base, base + SDHCI_WINDOW_BASE(i));
	}
}

/*
 * PXAv3: optionally delay the sampling clock to fix read data/command
 * timing (Linux: "mrvl,clk-delay-cycles"). Re-applied on every clock change.
 */
static int pxav3_sdhci_set_delay(struct sdhci_host *host)
{
	u32 cycles = dev_read_u32_default(host->mmc->dev,
					  "mrvl,clk-delay-cycles", 0);
	u16 tmp;

#if CONFIG_IS_ENABLED(DM_GPIO)
	/*
	 * Boards like goyawifi wire microSD card-detect to a GPIO, not to
	 * the controller; feed the GPIO state to the controller through the
	 * card-detect test level so it does not consider the slot empty.
	 */
	if (dm_gpio_is_valid(&host->cd_gpio)) {
		u8 ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);

		ctrl |= SDHCI_CTRL_CD_TEST;
		if (dm_gpio_get_value(&host->cd_gpio) > 0)
			ctrl |= SDHCI_CTRL_CD_TEST_INS;
		else
			ctrl &= ~SDHCI_CTRL_CD_TEST_INS;
		sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);
	}
#endif

	if (!cycles)
		return 0;

	tmp = sdhci_readw(host, SD_CLOCK_BURST_SIZE_SETUP);
	tmp &= ~(SDCLK_DELAY_MASK << SDCLK_DELAY_SHIFT);
	tmp |= (cycles & SDCLK_DELAY_MASK) << SDCLK_DELAY_SHIFT;
	tmp |= SDCLK_SEL;
	sdhci_writew(host, tmp, SD_CLOCK_BURST_SIZE_SETUP);

	return 0;
}

static const struct sdhci_ops pxav3_sdhci_ops = {
	.set_delay	= pxav3_sdhci_set_delay,
};

static int mv_sdhci_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct mv_sdhci_plat *plat = dev_get_plat(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	enum mv_sdhci_type type = dev_get_driver_data(dev);
	int ret;

	host->name = MVSDH_NAME;
	host->ioaddr = dev_read_addr_ptr(dev);
	host->quirks = SDHCI_QUIRK_32BIT_DMA_ADDR | SDHCI_QUIRK_WAIT_SEND_CMD;
	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	host->mmc->priv = host;

	if (type == MV_SDHCI_PXAV3) {
		/*
		 * The PXAv3 capability register does not report a usable base
		 * clock; it comes from the SoC clock unit instead.
		 */
		host->max_clk = dev_read_u32_default(dev, "clock-frequency", 0);
		host->ops = &pxav3_sdhci_ops;
	}

	ret = mmc_of_parse(dev, &plat->cfg);
	if (ret)
		return ret;

	ret = sdhci_setup_cfg(&plat->cfg, host, 0, 0);
	if (ret)
		return ret;

#if IS_ENABLED(CONFIG_ARCH_MVEBU)
	/* Configure SDHCI MBUS mbus bridge windows */
	if (type == MV_SDHCI_ARMADA_380)
		sdhci_mvebu_mbus_config(host->ioaddr);
#endif

	upriv->mmc = host->mmc;

	return sdhci_probe(dev);
}

static int mv_sdhci_bind(struct udevice *dev)
{
	struct mv_sdhci_plat *plat = dev_get_plat(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id mv_sdhci_ids[] = {
	{ .compatible = "marvell,armada-380-sdhci", .data = MV_SDHCI_ARMADA_380 },
	{ .compatible = "mrvl,pxav3-mmc", .data = MV_SDHCI_PXAV3 },
	{ }
};

U_BOOT_DRIVER(mv_sdhci_drv) = {
	.name		= MVSDH_NAME,
	.id		= UCLASS_MMC,
	.of_match	= mv_sdhci_ids,
	.bind		= mv_sdhci_bind,
	.probe		= mv_sdhci_probe,
	.ops		= &sdhci_ops,
	.priv_auto	= sizeof(struct sdhci_host),
	.plat_auto	= sizeof(struct mv_sdhci_plat),
};
