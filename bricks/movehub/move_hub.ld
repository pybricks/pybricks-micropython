/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2018-2020 The Pybricks Authors */

/*
    GNU linker script for LEGO BOOST Move Hub
*/

/* Specify the memory areas */
MEMORY
{
    /* Flash size is 128K, bootloader resides in first 20K, last 2K seems to be reserved */
    FLASH (rx)      : ORIGIN = 0x08005000, LENGTH = 106K
    USER_FLASH (rx) : ORIGIN = 0x0801F800, LENGTH = 2K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 16K
}

"MAGIC_OFFSET" = 0x100;
