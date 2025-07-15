OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(Entry)

MEMORY
{
    SRAM       (rwx) : ORIGIN = 0x80000000, LENGTH = 128K
    DDR_unused (rwx) : ORIGIN = 0xC0000000, LENGTH = 0x8000
    DDR        (rwx) : ORIGIN = 0xC0008000, LENGTH = (64M - 0x8000)
    ARM_LRAM   (rwx) : ORIGIN = 0xFFFF0000, LENGTH = 8K
}

_minimal_stack_size = 4M;
pbdrv_stack_end = ORIGIN(DDR) + LENGTH(DDR) - 4;

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text.*)
    } > DDR

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata.*)
    } > DDR

    /* External firmware binaries */
    .firmware :
    {
        . = ALIGN(4);
        _pru0_start = .;
        KEEP(*(.pru0));
        . = ALIGN(4);
        _pru0_end = .;
        _pru1_start = .;
        KEEP(*(.pru1));
        . = ALIGN(4);
        _pru1_end = .;
    } >DDR

    .data :
    {
        . = ALIGN(4);
        *(.data)
        *(.data.*)
    } > DDR

    .bss :
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        . = ALIGN(4);
        _bss_end = .;
    } > DDR

    /* Uninitialized (not zeroed at startup) data section */

    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit)
    } > DDR

    /* Exception vector table */
    .vector :
    {
        . = ALIGN(4);
        KEEP(*(.vector));
    } > ARM_LRAM

    .stack :
    {
        . = ALIGN(4);
        pbdrv_stack_start = .;
        . = . + _minimal_stack_size; /* will cause linker error if there is not enough space for stack. */
        . = ALIGN(4);
    } > DDR
}
