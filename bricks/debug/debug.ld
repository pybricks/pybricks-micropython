/*
    GNU linker script for STM32F446
*/

/* Specify the memory areas */
MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 512K /* entire flash */
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
}

_stack_size = 8K;
