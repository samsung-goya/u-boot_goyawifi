/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Save the registers handed over by the previous-stage bootloader.
 *
 * Samsung S-Boot enters the "kernel" following the 32-bit ARM Linux boot
 * protocol: r0 = 0, r1 = machine type, r2 = ATAG list (or FDT) pointer.
 * This runs before the stack exists, so it must be leaf, register-only and
 * position independent.
 */

#include <linux/linkage.h>

ENTRY(save_boot_params)
	adr	r12, 1f
	ldr	r3, [r12]
	add	r12, r12, r3
	stmia	r12, {r0 - r2}
	b	save_boot_params_ret
1:	.word	pxa988_prev_bl_regs - 1b
ENDPROC(save_boot_params)

	.section .data
	.align	2
	.globl	pxa988_prev_bl_regs
pxa988_prev_bl_regs:
	.word	0, 0, 0
