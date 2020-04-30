/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2018-2020 The Pybricks Authors */

/*
    GNU linker script for LEGO Powered Up City Hub
*/

/* Specify the memory areas */
MEMORY
{
    /* Flash size is 256K, bootloader resides in first 20K, last 4K seems to be reserved */
    FLASH (rx)      : ORIGIN = 0x08005000, LENGTH = 232K
    USER_FLASH (rx) : ORIGIN = 0x0803F000, LENGTH = 4K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 32K
}

"MAGIC_OFFSET" = 0x100;
