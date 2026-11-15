/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
/*
 * Copyright 2013 Marvell Tech. Ltd.
 * Copyright 2026 postmarketOS goyawifi port
 *
 * Pinmux and pinconf defines for Marvell PXA988 / PXA986 using pinctrl-single.
 */

#ifndef __DT_BINDINGS_PINCTRL_MARVELL_PXA988_PINFUNC_H
#define __DT_BINDINGS_PINCTRL_MARVELL_PXA988_PINFUNC_H

/* Drive strength: value and mask (bits [12:11]) */
#define DS_SLOW0		0x0000
#define DS_SLOW1		0x0800
#define DS_MEDIUM		0x1000
#define DS_FAST			0x1800
#define DS_MASK			0x1800

/* Bias pull-up/down values and masks (bits [15:13]) */
#define PULL_NONE		0x0000
#define PULL_UP			0xc000
#define PULL_DOWN		0xa000
#define PULL_FLOAT		0x8000
#define PULL_MASK		0xe000

/* Low-power modes (sleep output / float / drive): value and mask */
#define LPM_NONE		0x000
#define LPM_DRIVE_LOW		0x208
#define LPM_DRIVE_HIGH		0x308
#define LPM_FLOAT		0x288
#define LPM_MASK		0x388

/* MFP Alternate Functions 0-7 */
#define AF0			0x0
#define AF1			0x1
#define AF2			0x2
#define AF3			0x3
#define AF4			0x4
#define AF5			0x5
#define AF6			0x6
#define AF7			0x7

/* Pin names and MFPR byte offsets from pmx base 0xd401e000 */
#define DF_IO15			0x0004
#define DF_IO14			0x0008
#define DF_IO13			0x000c
#define DF_IO12			0x0010
#define DF_IO11			0x0014
#define DF_IO10			0x0018
#define DF_IO9			0x001c
#define DF_IO8			0x0020
#define DF_IO7			0x0024
#define DF_IO6			0x0028
#define DF_IO5			0x002c
#define DF_IO4			0x0030
#define DF_IO3			0x0034
#define DF_IO2			0x0038
#define DF_IO1			0x003c
#define DF_IO0			0x0040

#define DF_NCS0_SM_NCS2		0x0044
#define DF_NCS1_SM_NCS3		0x0048
#define SM_NCS0			0x004c
#define SM_NCS1			0x0050
#define DF_WEN			0x0054
#define DF_REN			0x0058
#define DF_CLE_SM_OEN		0x005c
#define DF_ALE_SM_WEN		0x0060
#define SM_SCLK			0x0064
#define DF_RDY0			0x0068
#define SM_BE0			0x006c
#define SM_BE1			0x0070
#define SM_ADV			0x0074
#define DF_RDY1			0x0078
#define SM_ADVMUX		0x007c
#define SM_RDY			0x0080

#define MMC1_DAT7		0x0084
#define MMC1_DAT6		0x0088
#define MMC1_DAT5		0x008c
#define MMC1_DAT4		0x0090
#define MMC1_DAT3		0x0094
#define MMC1_DAT2		0x0098
#define MMC1_DAT1		0x009c
#define MMC1_DAT0		0x00a0
#define MMC1_CMD		0x00a4
#define MMC1_CLK		0x00a8
#define MMC1_CD			0x00ac
#define MMC1_WP			0x00b0

#define CLK_REQ			0x00cc
#define GPIO124			0x00d0
#define VCXO_REQ		0x00d4
#define VCXO_OUT		0x00d8

#define GPIO0			0x00dc
#define GPIO1			0x00e0
#define GPIO2			0x00e4
#define GPIO3			0x00e8
#define GPIO4			0x00ec
#define GPIO5			0x00f0
#define GPIO6			0x00f4
#define GPIO7			0x00f8
#define GPIO8			0x00fc
#define GPIO9			0x0100
#define GPIO10			0x0104
#define GPIO11			0x0108
#define GPIO12			0x010c
#define GPIO13			0x0110
#define GPIO14			0x0114
#define GPIO15			0x0118
#define GPIO16			0x011c
#define GPIO17			0x0120
#define GPIO18			0x0124
#define GPIO19			0x0128
#define GPIO20			0x012c
#define GPIO21			0x0130
#define GPIO22			0x0134
#define GPIO23			0x0138
#define GPIO24			0x013c
#define GPIO25			0x0140
#define GPIO26			0x0144
#define GPIO27			0x0148
#define GPIO28			0x014c
#define GPIO29			0x0150
#define GPIO30			0x0154
#define GPIO31			0x0158
#define GPIO32			0x015c
#define GPIO33			0x0160
#define GPIO34			0x0164
#define GPIO35			0x0168
#define GPIO36			0x016c
#define GPIO37			0x0170
#define GPIO38			0x0174
#define GPIO39			0x0178
#define GPIO40			0x017c
#define GPIO41			0x0180
#define GPIO42			0x0184
#define GPIO43			0x0188
#define GPIO44			0x018c
#define GPIO45			0x0190
#define GPIO46			0x0194
#define GPIO47			0x0198
#define GPIO48			0x019c
#define GPIO49			0x01a0
#define GPIO50			0x01a4
#define GPIO51			0x01a8
#define GPIO52			0x01ac
#define GPIO53			0x01b0
#define GPIO54			0x01b4

#define GPIO67			0x01b8
#define GPIO68			0x01bc
#define GPIO69			0x01c0
#define GPIO70			0x01c4
#define GPIO71			0x01c8
#define GPIO72			0x01cc
#define GPIO73			0x01d0
#define GPIO74			0x01d4
#define GPIO75			0x01d8
#define GPIO76			0x01dc
#define GPIO77			0x01e0
#define GPIO78			0x01e4
#define GPIO79			0x01e8
#define GPIO80			0x01ec
#define GPIO81			0x01f0
#define GPIO82			0x01f4
#define GPIO83			0x01f8
#define GPIO84			0x01fc
#define GPIO85			0x0200
#define GPIO86			0x0204
#define GPIO87			0x0208
#define GPIO88			0x020c
#define GPIO89			0x0210
#define GPIO90			0x0214
#define GPIO91			0x0218
#define GPIO92			0x021c
#define GPIO93			0x0220
#define GPIO94			0x0224
#define GPIO95			0x0228
#define GPIO96			0x022c
#define GPIO97			0x0230
#define GPIO98			0x0234

#define GPIO100			0x0238
#define GPIO101			0x023c
#define GPIO102			0x0240
#define GPIO103			0x0244
#define GPIO104			0x0248
#define GPIO105			0x024c
#define GPIO106			0x0250
#define GPIO107			0x0254
#define GPIO108			0x0258
#define GPIO109			0x025c

#define ANT_SW4			0x026c

#define GPIO110			0x0298
#define GPIO111			0x029c
#define GPIO112			0x02a0
#define GPIO113			0x02a4
#define GPIO114			0x02a8
#define GPIO115			0x02ac
#define GPIO116			0x02b0

/* Aliases matching vendor pin names */
#define ND_IO15			DF_IO15
#define ND_IO14			DF_IO14
#define ND_IO13			DF_IO13
#define ND_IO12			DF_IO12
#define ND_IO11			DF_IO11
#define ND_IO10			DF_IO10
#define ND_IO9			DF_IO9
#define ND_IO8			DF_IO8
#define ND_IO7			DF_IO7
#define ND_IO6			DF_IO6
#define ND_IO5			DF_IO5
#define ND_IO4			DF_IO4
#define ND_IO3			DF_IO3
#define ND_IO2			DF_IO2
#define ND_IO1			DF_IO1
#define ND_IO0			DF_IO0

#define ND_CLE_SM_OEN		DF_CLE_SM_OEN
#define SM_BEN0			SM_BE0
#define SM_BEN1			SM_BE1
#define SM_RDY_GPIO_3		SM_RDY

#endif /* __DT_BINDINGS_PINCTRL_MARVELL_PXA988_PINFUNC_H */
