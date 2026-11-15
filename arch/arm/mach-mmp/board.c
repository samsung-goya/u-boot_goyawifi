// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024
 * Duje Mihanović <duje.mihanovic@skole.hr>
 */
#include <errno.h>
#include <init.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

__weak int board_init(void)
{
	return 0;
}

int dram_init(void)
{
	if (fdtdec_setup_mem_size_base() != 0)
		puts("fdtdec_setup_mem_size_base() has failed\n");

	return 0;
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

#if CONFIG_IS_ENABLED(SAVE_PREV_BL_FDT_ADDR)
/* Stolen from arch/arm/mach-snapdragon/board.c */
int board_fdt_blob_setup(void **fdtp)
{
	struct fdt_header *fdt;
	bool internal_valid, external_valid;
	int ret = 0;

	fdt = (struct fdt_header *)get_prev_bl_fdt_addr();
	external_valid = fdt && !fdt_check_header(fdt);
	internal_valid = !fdt_check_header(*fdtp);

	/*
	 * There is no point returning an error here, U-Boot can't do anything useful in this situation.
	 * Bail out while we can still print a useful error message.
	 */
	if (!internal_valid && !external_valid)
		panic("Internal FDT is invalid and no external FDT was provided! (fdt=%#llx)\n",
		      (phys_addr_t)fdt);

	if (internal_valid) {
		debug("Using built in FDT\n");
		ret = -EEXIST;
	} else {
		debug("Using external FDT\n");
		/* So we can use it before returning */
		*fdtp = fdt;
	}

	return ret;
}
#endif
