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

/* top end of the stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);

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
        *(.text)           /* .text sections (code) */
        *(.text*)          /* .text* sections (code) */
        *(.rodata)         /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */

        . = ALIGN(4);
        LONG(CHECKSUM)

        . = ALIGN(4);
        _etext = .;        /* define a global symbol at end of code */
        _sidata = _etext;  /* This is used by the startup in order to initialize the .data section */
    } >FLASH

    .user :
    {
        BYTE(0)             /* In official LEGO firmware, this is where brick name is stored.
                               It needs to be zeroed so that when the official firmware is
                               reloaded, it will use the default name */

        KEEP(*(.mpy))       /* Compiled main.mpy file */
    } >USER_FLASH

    /* This section is for special needs of the Move Hub firmware - must be first section in SRAM! */
    .magic :
    {
        _fw_isr_vector_dst = .;     /* The firmware will copy isr_vector here (0x20000000) */
        . = ALIGN(4);
        . = 0x100;                  /* offset of bootloader magic value (0x20000100), also has the effect of
                                       reserving more than enough space for the isr vector table copy */
        *(.magic)                   /* used for passing magic value from firmware to bootloader */
    } >RAM

    /* This is the initialized data section
    The program executes knowing that the data is in the RAM
    but the loader puts the initial values in the FLASH (inidata).
    It is one task of the startup to copy the initial values from FLASH to RAM. */
    .data : AT ( _sidata )
    {
        . = ALIGN(4);
        _sdata = .;        /* create a global symbol at data start; used by startup code in order to initialise the .data section in RAM */
        *(.data)           /* .data sections */
        *(.data*)          /* .data* sections */

        . = ALIGN(4);
        _edata = .;        /* define a global symbol at data end; used by startup code in order to initialise the .data section in RAM */
    } >RAM

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

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
