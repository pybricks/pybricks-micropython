/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2018-2022 The Pybricks Authors */

/*
    GNU linker script for LEGO SPIKE Prime hub
*/

/* Specify the memory areas */
MEMORY
{
    /* Non-erasable DFU bootloader. */
    FROZEN_DFU_BOOTLOADER (rx) : ORIGIN = 0x08000000, LENGTH = 32K
    /* Second stage DFU bootloader for compatibility with official firmware. */
    SECOND_DFU_BOOTLOADER (rx) : ORIGIN = 0x08008000, LENGTH = 32K
    /* The firmware. Installed via DFU bootloader */
    FLASH_FIRMWARE (rx)   : ORIGIN = 0x8010000, LENGTH = 960K
    /* SRAM1 (256K) and SRAM2 (64K) are treated as one continuous block */
    RAM (xrw)             : ORIGIN = 0x20000000, LENGTH = 320K
}

/* Minimal stack size to allow the build to proceed. The actual stack size is whatever is left after
   statically allocated memory. It can be tuned by setting PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE. */
_minimal_stack_size = 12K;

/* Read by bootloader */
"FW_INFO_OFFSET" = 0x1f0;
"FW_INFO_OFFSET2" = 0x264;

/* Entry point of the second stage mboot, used to enter firmware update mode. */
_pbdrv_reset_mboot_start = ORIGIN(SECOND_DFU_BOOTLOADER);
