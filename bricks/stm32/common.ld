/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2013, 2014 Damien P. George */
/* Copyright (c) 2018-2022 The Pybricks Authors */

/* top end of the stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);
_sstack = _estack - _stack_size;

/* define output sections */
SECTIONS
{
    /* The program code and other data goes into FLASH_FIRMWARE */
    .text :
    {
        . = ALIGN(4);
        _fw_isr_vector_src = .;   /* The isr vector table must be copied to SRAM since the
                                     firmware doesn't start at the beginning of the flash memory */
        KEEP(*(.isr_vector)) /* isr vector table */
        . = DEFINED(FW_INFO_OFFSET) ? FW_INFO_OFFSET : .;
        *(.fw_info) /* .fw_info section for info read during SPIKE firmware update */
        KEEP(*(.fw_info))
        *(.text)           /* .text sections (code) */
        *(.text*)          /* .text* sections (code) */
        *(.rodata)         /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */

        . = ALIGN(4);
        _etext = .;        /* define a global symbol at end of code */
        _sidata = _etext;  /* This is used by the startup in order to initialize the .data section */
    } >FLASH_FIRMWARE

    /* This section is for special needs of the Move Hub firmware - must be first section in SRAM! */
    .magic :
    {
        _fw_isr_vector_dst = .;     /* The firmware will copy isr_vector here */
        . = ALIGN(4);
        . = DEFINED(MAGIC_OFFSET) ? MAGIC_OFFSET : .;
                                    /* Offset of bootloader magic value. Also has the effect of
                                       reserving more than enough space for the isr vector table copy */
        *(.magic)                   /* used for passing magic value from firmware to bootloader */
    } >RAM

    /* used by the startup to initialize data */
    _sidata = LOADADDR(.data);

    /* This is the initialized data section
    The program executes knowing that the data is in the RAM
    but the loader puts the initial values in the FLASH_FIRMWARE (inidata).
    It is one task of the startup to copy the initial values from FLASH_FIRMWARE to RAM. */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* create a global symbol at data start; used by startup code in order to initialize the .data section in RAM */
        *(.data*)          /* .data* sections */

        . = ALIGN(4);
        _edata = .;        /* define a global symbol at data end; used by startup code in order to initialize the .data section in RAM */
    } >RAM AT> FLASH_FIRMWARE

    /* Uninitialized (zeroed at startup) data section */
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

    /* Uninitialized (not zeroed at startup) data section */

    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit)
    } >RAM

    .stack :
    {
        . = ALIGN(4);
        . = . + _stack_size; /* will cause linker error if there is not enough space for stack. */
        . = ALIGN(4);
    } >RAM

    .name :
    {
        *(.name) /* customizable hub name */
    } >FLASH_FIRMWARE

    .user :
    {
        . = ALIGN(4);
        _pb_user_mpy_size = .;
        LONG(SIZEOF(.user) - (. + 4 - LOADADDR(.user))); /* Size of compiled main.mpy file in bytes */
        _pb_user_mpy_data = .;
        KEEP(*(.mpy));       /* Compiled main.mpy file */
    } >FLASH_FIRMWARE

    .checksum :
    {
        . = ALIGN(4);
        _checksum = .;
        LONG(0) /* This gets replaced later with proper checksum by other tools. */
    } >FLASH_FIRMWARE

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
