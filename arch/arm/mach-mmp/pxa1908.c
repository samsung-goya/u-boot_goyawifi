// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024
 * Duje Mihanović <duje.mihanovic@skole.hr>
 */
#include <init.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/bitops.h>

DECLARE_GLOBAL_DATA_PTR;

/* Timer constants */
#define APBC_COUNTER_CLK_SEL	0xd4015064
#define COUNTER_BASE		0xd4101000
#define COUNTER_EN		BIT(0)
#define COUNTER_HALT_ON_DEBUG	BIT(1)

int timer_init(void)
{
	u32 tmp = readl(APBC_COUNTER_CLK_SEL);

	if ((tmp >> 16) != 0x319)
		return -1;

	/* Set timer frequency to 26MHz */
	writel(tmp | 1, APBC_COUNTER_CLK_SEL);
	writel(COUNTER_EN | COUNTER_HALT_ON_DEBUG, COUNTER_BASE);

	gd->arch.timer_rate_hz = 26000000;

	return 0;
}

#ifndef CONFIG_SYSRESET
void reset_cpu(void)
{
}
#endif
