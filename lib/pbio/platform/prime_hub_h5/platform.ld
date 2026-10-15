/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2018-2026 The Pybricks Authors */

/*
 * GNU linker script for LEGO SPIKE Prime hub H5 variant.
 */

/* Specify the memory areas */
MEMORY
{
    /* Non-erasable DFU bootloader (mboot). */
    FLASH_BOOTLOADER (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    /* The firmware. Installed via DFU bootloader */
    FLASH_FIRMWARE (rx)   : ORIGIN = 0x08010000, LENGTH = 1M - 64K
    /* SRAM1 (256K), SRAM2 (64K) and SRAM3 (320K) are treated as one continuous block */
    RAM (xrw)             : ORIGIN = 0x20000000, LENGTH = 640K
}

/* Minimal stack size to allow the build to proceed. The actual stack size is whatever is left after
   statically allocated memory. It can be tuned by setting PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE. */
_minimal_stack_size = 16K;

/* Read by bootloader */
"FW_INFO_OFFSET" = 0x264;

/* Entry point of mboot, used to enter firmware update mode. */
_pbdrv_reset_mboot_start = ORIGIN(FLASH_BOOTLOADER);
