/*
    GNU linker script for STM32F446
*/

/* Specify the memory areas */
MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 508K
    USER_FLASH (rx) : ORIGIN = 0x0807F000, LENGTH = 4K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
}

_app_data_flash_start = ORIGIN(USER_FLASH);
_app_data_flash_end = ORIGIN(USER_FLASH) + LENGTH(USER_FLASH);
_stack_size = 8K;
