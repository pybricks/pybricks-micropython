/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2018-2021 The Pybricks Authors */

/*
    GNU linker script for LEGO SPIKE Essential hub
*/

/* Specify the memory areas */
MEMORY
{
    /* Flash size is 1M. Bootloader resides in first 32K. */
    FLASH (rx)      : ORIGIN = 0x08008000, LENGTH = 992K
    /* SRAM1 (256K) and SRAM2 (64K) are treated as one continuous block */
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 320K
}

"FW_INFO_OFFSET" = 0x200;
_stack_size = 16K;

/* On this hub, user data is stored on the external flash, so it does not
 * directly affect the internal flash layout. We still define these symbols
 * here so we can ensure there is enough available RAM to load the program.
 */
_app_data_flash_start = 512K;
_app_data_flash_end = _app_data_flash_start + 256K;
