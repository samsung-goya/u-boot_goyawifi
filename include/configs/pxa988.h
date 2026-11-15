/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Marvell PXA988 / PXA986 boards chain-loaded by a vendor bootloader
 */

#ifndef __PXA988_H
#define __PXA988_H

#define CFG_SYS_SDRAM_BASE	0x00000000

/* PXA UARTs need IER.UUE (unit enable) set, or they stay off */
#define CFG_SYS_NS16550_IER	0x40

#define CFG_SYS_BAUDRATE_TABLE	{ 115200, 230400, 460800, 921600 }

#endif
