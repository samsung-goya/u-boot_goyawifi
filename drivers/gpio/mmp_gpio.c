// SPDX-License-Identifier: GPL-2.0+
/*
 * Marvell MMP (PXA910 / PXA988 / PXA1908) GPIO controller
 *
 * 32 GPIOs per bank; banks 0-2 sit at 0x000/0x004/0x008, banks 3+ at
 * 0x100/0x104/... (see Linux drivers/gpio/gpio-pxa.c, BANK_OFF()).
 */

#include <dm.h>
#include <errno.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/bitops.h>

#define GPLR	0x00	/* pin level */
#define GPDR	0x0c	/* direction (1 = output) */
#define GPSR	0x18	/* set output */
#define GPCR	0x24	/* clear output */
#define GSDR	0x54	/* set direction bits */
#define GCDR	0x60	/* clear direction bits */

struct mmp_gpio_priv {
	void __iomem *base;
};

static void __iomem *mmp_gpio_bank(struct udevice *dev, unsigned int gpio)
{
	struct mmp_gpio_priv *priv = dev_get_priv(dev);
	unsigned int bank = gpio / 32;

	return priv->base + (bank < 3 ? bank << 2 : 0x100 + ((bank - 3) << 2));
}

static int mmp_gpio_direction_input(struct udevice *dev, unsigned int gpio)
{
	writel(BIT(gpio % 32), mmp_gpio_bank(dev, gpio) + GCDR);

	return 0;
}

static int mmp_gpio_direction_output(struct udevice *dev, unsigned int gpio,
				     int value)
{
	void __iomem *bank = mmp_gpio_bank(dev, gpio);

	writel(BIT(gpio % 32), bank + (value ? GPSR : GPCR));
	writel(BIT(gpio % 32), bank + GSDR);

	return 0;
}

static int mmp_gpio_get_value(struct udevice *dev, unsigned int gpio)
{
	return !!(readl(mmp_gpio_bank(dev, gpio) + GPLR) & BIT(gpio % 32));
}

static int mmp_gpio_set_value(struct udevice *dev, unsigned int gpio,
			      int value)
{
	writel(BIT(gpio % 32), mmp_gpio_bank(dev, gpio) + (value ? GPSR : GPCR));

	return 0;
}

static int mmp_gpio_get_function(struct udevice *dev, unsigned int gpio)
{
	if (readl(mmp_gpio_bank(dev, gpio) + GPDR) & BIT(gpio % 32))
		return GPIOF_OUTPUT;

	return GPIOF_INPUT;
}

static const struct dm_gpio_ops mmp_gpio_ops = {
	.direction_input	= mmp_gpio_direction_input,
	.direction_output	= mmp_gpio_direction_output,
	.get_value		= mmp_gpio_get_value,
	.set_value		= mmp_gpio_set_value,
	.get_function		= mmp_gpio_get_function,
};

static int mmp_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct mmp_gpio_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base)
		return -EINVAL;

	uc_priv->gpio_count = dev_read_u32_default(dev, "ngpios", 128);
	uc_priv->bank_name = "GPIO";

	return 0;
}

static const struct udevice_id mmp_gpio_ids[] = {
	{ .compatible = "marvell,mmp-gpio" },
	{ .compatible = "marvell,pxa1908-gpio" },
	{ }
};

U_BOOT_DRIVER(mmp_gpio) = {
	.name		= "mmp_gpio",
	.id		= UCLASS_GPIO,
	.of_match	= mmp_gpio_ids,
	.probe		= mmp_gpio_probe,
	.ops		= &mmp_gpio_ops,
	.priv_auto	= sizeof(struct mmp_gpio_priv),
};
