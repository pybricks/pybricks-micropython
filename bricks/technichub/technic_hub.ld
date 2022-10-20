/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2019-2022 The Pybricks Authors */

/*
    GNU linker script for LEGO Technic Hub
*/

/* Memory areas. NB: Total storage size (FLASH_USER_0 + FLASH_USER_1)
   MUST match PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32_SIZE */
MEMORY
{
    /* Non-erasable BLE bootloader (LEGO LWP3). Device boots only if all words
       in FLASH_FIRMWARE and FLASH_USER_0 add up to 0*/
    FLASH_BOOTLOADER (rx) : ORIGIN = 0x08000000, LENGTH = 32K
    /* The firmware. Installed via BLE bootloader. */
    FLASH_FIRMWARE (rx)   : ORIGIN = 0x08008000, LENGTH = 208K
    /* User data written by the firmware during shutdown. */
    FLASH_USER_0 (rx)     : ORIGIN = 0x0803C000, LENGTH = 12K
    /* As above, but this part is not counted by bootloader checksum. */
    FLASH_USER_1 (rx)     : ORIGIN = 0x0803F000, LENGTH = 4K
    RAM (xrw)             : ORIGIN = 0x20000000, LENGTH = 64K
}

"MAGIC_OFFSET" = 0x200;
_stack_size = 8K;

/* Start of data storage. */
_pbdrv_block_device_storage_start = ORIGIN(FLASH_USER_0);

/* Portion of the data parsed for the checksum on boot. */
_pbsys_program_load_checked_size = LENGTH(FLASH_USER_0);
