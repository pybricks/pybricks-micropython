/*
    GNU linker script for STM32F446
*/

/* Specify the memory areas */
MEMORY
{
    FLASH_FIRMWARE (rx) : ORIGIN = 0x08000000, LENGTH = 512K /* entire flash */
    RAM (xrw)           : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Minimal stack size to allow the build to proceed. The actual stack size is whatever is left after
   statically allocated memory. It can be tuned by setting PBSYS_CONFIG_PROGRAM_LOAD_RAM_SIZE. */
_minimal_stack_size = 8K;
