/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2018-2021 The Pybricks Authors */

/*
    GNU linker script for LEGO SPIKE Prime hub
*/

/* Specify the memory areas */
MEMORY
{
    /* Flash size is 1M. Bootloader resides in first 32K. */
    FLASH (rx)      : ORIGIN = 0x08008000, LENGTH = 992K
    /* SRAM1 (256K) and SRAM2 (64K) are treated as one continuous block */
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 320K
}

_stack_size = 16K;
