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

/**
 * External flash storage size for 1 application program. The full user program
 * is loaded into RAM to run it, so the storage must not be bigger than RAM.
 * We also need to allow some RAM for bss and heap, so set storage a bit lower.
 */
_pbdrv_block_device_storage_size = 256K;
