/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Boot0 header for chain-loading U-Boot from vendor bootloaders that expect
 * a 32-bit ARM Linux zImage (e.g. Samsung S-Boot on PXA986/PXA988).
 *
 * The first 8 words are the normal exception vector table (U-Boot points
 * VBAR at _start), so the vendor loader enters via "b reset". The zImage
 * magic number lives at offset 0x24, exactly where arch/arm/boot/compressed
 * places it, so loaders that sanity-check the kernel image accept U-Boot.
 */

_start:
	ARM_VECTORS			/* 0x00 - 0x1f */
	b	reset			/* 0x20: zImage "b 1f" slot */
	.word	0x016f2818		/* 0x24: zImage magic */
	.word	0			/* 0x28: start (position independent) */
	.word	_image_binary_end - _start /* 0x2c: end / image size */
	.word	0x04030201		/* 0x30: endianness flag (LE) */
	.word	0x45454545		/* 0x34: extension table magic */
	.word	0			/* 0x38: no extension table */
