/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2019-2021 The Pybricks Authors */

/*
    GNU linker script for LEGO Technic Hub
*/

/* Specify the memory areas */
MEMORY
{
    /* Flash size is 256K, bootloader resides in first 32K, last 4K is for user data */
    FLASH (rx)      : ORIGIN = 0x08008000, LENGTH = 220K
    USER_FLASH (rx) : ORIGIN = 0x0803F000, LENGTH = 4K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 64K
}

"MAGIC_OFFSET" = 0x200;
_minimum_stack_size = 4K;
