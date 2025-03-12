OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(Entry)

MEMORY
{
    SRAM       (rwx) : ORIGIN = 0x80000000, LENGTH = 128K
    DDR_unused (rwx) : ORIGIN = 0xC0000000, LENGTH = 0x8000
    DDR        (rwx) : ORIGIN = 0xC0008000, LENGTH = (64M - 0x8000)
}

_stack = 0xC2000000;
_minimal_stack_size = 4K;
_estack = ORIGIN(DDR) + LENGTH(DDR);

SECTIONS
{
    .startcode :
    {
        . = ALIGN(4);
        *startup.o (.text)
    } > DDR

    .text :
    {
        . = ALIGN(4);
        *(.text)
    } > DDR

    .data :
    {
        . = ALIGN(4);
        *(.data)
    } > DDR

    .bss :
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        . = ALIGN(4);
        _bss_end = .;
    } > DDR

    /* Uninitialized (not zeroed at startup) data section */

    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit)
    } > DDR

    .stack :
    {
        . = ALIGN(4);
        _sstack = .;
        . = . + _minimal_stack_size; /* will cause linker error if there is not enough space for stack. */
        . = ALIGN(4);
    } > DDR
}
