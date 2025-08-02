OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(Entry)

MEMORY
{
    SRAM_PRU0  (rw)  : ORIGIN = 0x80000000, LENGTH = 64K
    SRAM_PRU1  (rw)  : ORIGIN = 0x80010000, LENGTH = 64K
    DDR_unused (rwx) : ORIGIN = 0xC0000000, LENGTH = 0x8000
    DDR        (rwx) : ORIGIN = 0xC0008000, LENGTH = (64M - 0x8000)
    ARM_LRAM   (rwx) : ORIGIN = 0xFFFF0000, LENGTH = (8K - 16)
}

_minimal_stack_size = 4M;
pbdrv_stack_end = ORIGIN(DDR) + LENGTH(DDR) - 4;
/* Extra heap for large allocations (images, etc). */
pb_umm_heap_size = 2M;
/*
We declare this in this style rather than creating a section.
If we do create a section, the loader (U-boot) will always clear it.
This defeats what we are trying to do to determine reset causes.
*/
ev3_persistent_data = ORIGIN(ARM_LRAM) + LENGTH(ARM_LRAM);

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
        /* Put DMA RX buffers here so that they never share cache lines with
           unrelated data. This makes sure that they can be invalidated properly. */
        . = ALIGN(32);
        *(.dma)
        . = ALIGN(32);
        _bss_end = .;
    } > DDR

    /* Uninitialized (not zeroed at startup) data section */

    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit)

        . = ALIGN(4);
        pb_umm_heap_start = .;
        . = . + pb_umm_heap_size;
        pb_umm_heap_end = .;
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

    /* Shared on-chip SRAM */
    .shared0 (NOLOAD) :
    {
        *(.shared0)
    } > SRAM_PRU0
    .shared1 (NOLOAD) :
    {
        *(.shared1)
    } > SRAM_PRU1
}
