/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2018-2022 The Pybricks Authors */

/*
    GNU linker script for LEGO BOOST Move Hub
*/

/* Specify the memory areas */
MEMORY
{
    /* Non-erasable BLE bootloader (LEGO LWP3). */
    FLASH_BOOTLOADER (rx) : ORIGIN = 0x08000000, LENGTH = 20K
    /* The firmware. Installed via BLE bootloader. */
    FLASH_FIRMWARE (rx)   : ORIGIN = 0x08005000, LENGTH = 106K
    /* User data written by the firmware during shutdown. */
    FLASH_USER (rx)       : ORIGIN = 0x0801F800, LENGTH = 2K
    RAM (xrw)             : ORIGIN = 0x20000000, LENGTH = 16K
}

"MAGIC_OFFSET" = 0x100;
_stack_size = 3K;
_pbdrv_block_device_storage_start = ORIGIN(FLASH_USER);
_pbdrv_block_device_storage_size = LENGTH(FLASH_USER);
