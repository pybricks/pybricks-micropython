OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

SECTIONS
{
    . = 0xC1000000;
    . = ALIGN(4);
    .startcode :
    {
            *startup.o (.text)
    }
    . = ALIGN(4);
    .text :
    {
            *(.text)
    }
    . = ALIGN(4);
    .data : 
    { 
            *(.data) 
    }
    . = ALIGN(4);
    _bss_start = .;
    .bss : 
    { 
            *(.bss) 
    }
    . = ALIGN(4);
    _bss_end = .;
    . = ALIGN(8);
    heap_low = .; /* for _sbrk */
    . = . + 0x10000; /* 64 KB of heap memory */
    heap_top = .; /* for _sbrk */

    _stack = 0xC2000000; /* For initialization */
    __system_stack__ = 0xC3000000;
}
