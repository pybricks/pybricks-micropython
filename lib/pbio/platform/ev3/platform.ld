OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(Entry)

MEMORY
{
    SRAM       (rwx) : ORIGIN = 0x80000000, LENGTH = 128K
    DDR_unused (rwx) : ORIGIN = 0xC0000000, LENGTH = 0x8000
    DDR        (rwx) : ORIGIN = 0xC0008000, LENGTH = (64M - 0x8000)
}

_minimal_stack_size = 64K;
pbdrv_stack_end = ORIGIN(DDR) + LENGTH(DDR) - 4;

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

    .pru :
    {
        . = ALIGN(4);
        _pru0_start = .;
        KEEP(*(.pru0));
        . = ALIGN(4);
        _pru0_end = .;
    } >DDR

    .display_frames (NOLOAD) :
    {
        . = ALIGN(4);
        *(.pbdrv_display_user_frame)
        . = ALIGN(4);
        *(.st7586s_send_buf)
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
        pbdrv_stack_start = .;
        . = . + _minimal_stack_size; /* will cause linker error if there is not enough space for stack. */
        . = ALIGN(4);
    } > DDR
}
