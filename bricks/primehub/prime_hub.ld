/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2018-2020 The Pybricks Authors */

/*
    GNU linker script for LEGO SPIKE Prime hub
*/

/* Specify the memory areas */
MEMORY
{
    /* Flash size is 1M, bootloader resides in first 32K */
    FLASH (rx)      : ORIGIN = 0x08008000, LENGTH = 960K
    USER_FLASH (rx) : ORIGIN = 0x080F8000, LENGTH = 32K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 320K
}

/* top end of the stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* RAM extents for the garbage collector */
_ram_start = ORIGIN(RAM);
_ram_end = ORIGIN(RAM) + LENGTH(RAM);
_heap_start = _ebss; /* heap starts just after statically allocated memory */
_heap_end = 0x2001c000; /* tunable */

/* define output sections */
SECTIONS
{
    /* The program code and other data goes into FLASH */
    .text :
    {
        . = ALIGN(4);
        _fw_isr_vector_src = .;   /* The isr vector table must be copied to SRAM since the
                                     firmware doesn't start at the beginning of the flash memory */
        KEEP(*(.isr_vector)) /* isr vector table */
        . = 0x200;
        *(.boot)           /* .boot section for info read by bootloader */
        KEEP(*(.boot))
        *(.text)           /* .text sections (code) */
        *(.text*)          /* .text* sections (code) */
        *(.rodata)         /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */

        . = ALIGN(4);
        _etext = .;        /* define a global symbol at end of code */
        _sidata = _etext;  /* This is used by the startup in order to initialize the .data section */
    } >FLASH

    .user :
    {
        LONG(0);
        LONG(MPYSIZE);
        KEEP(*(.mpy));       /* Compiled main.mpy file. This section will be dropped once external flash is enabled */
    } >USER_FLASH

    __user_flash_start = ORIGIN(USER_FLASH);

    /* used by the startup to initialize data */
    _sidata = LOADADDR(.data);

    /* This is the initialized data section
    The program executes knowing that the data is in the RAM
    but the loader puts the initial values in the FLASH (inidata).
    It is one task of the startup to copy the initial values from FLASH to RAM. */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* create a global symbol at data start; used by startup code in order to initialise the .data section in RAM */
        *(.data*)          /* .data* sections */

        . = ALIGN(4);
        _edata = .;        /* define a global symbol at data end; used by startup code in order to initialise the .data section in RAM */
    } >RAM AT> FLASH

    /* Uninitialized data section */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;         /* define a global symbol at bss start; used by startup code */
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;         /* define a global symbol at bss end; used by startup code */
    } >RAM

    .checksum :
    {
        . = ALIGN(4);
        _checksum = .;
        LONG(CHECKSUM)
    } >FLASH

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
