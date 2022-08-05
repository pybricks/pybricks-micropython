/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2018-2021 The Pybricks Authors */

/*
    GNU linker script for LEGO Powered Up City Hub
*/

/* Specify the memory areas */
MEMORY
{
    /* Flash size is 256K, bootloader resides in first 20K. */
    FLASH (rx)      : ORIGIN = 0x08005000, LENGTH = 232K
    USER_FLASH (rx) : ORIGIN = 0x0803F000, LENGTH = 4K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 32K
}

"MAGIC_OFFSET" = 0x100;
_stack_size = 4K;

/* User data is saved on shutdown at the end of the internal flash. */
_app_data_flash_start = ORIGIN(USER_FLASH);
_app_data_flash_end = ORIGIN(USER_FLASH) + LENGTH(USER_FLASH);

