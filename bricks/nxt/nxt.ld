/* SPDX-License-Identifier: MPL-1.0 */
/**
 * This script is for a system that runs with the memory
 * mapped so that ram is visible at both 0 and 2M.
 *
 * Notes on alignment:
 * 1) Section to be relocated should be at least 4- or even 16-byte aligned.
 * 2) Same holds for sections to be initialized.
 *
 * That will facilitate fast copying/initialization based on LDM/STM.
 *
 *
 * Memory layout:
 *   ROM: .data .text
 *   RAM: .data .bss .stack <heap>
 *
 * On boot, .data is copied from ROM to RAM in 16-byte chunks.
 * Then .bss is overwritten with zeros, also in 16-byte chunks.
 * As we use 16-byte chunks, we may copy and initialize more
 * than needed, but who cares ...
 */


/**
 * Memory definitions for SAM7S256.
 */
MEMORY {
  ROM : ORIGIN = 1M, LENGTH = 256k
  RAM : ORIGIN = 0M, LENGTH =  64K
}

SECTIONS {

  .data : {
    KEEP(*(.vectors))
    *(.ramfunc .ramfunc.*)
    *(.data .data.*)
  } >RAM AT>ROM

  __relocate_vma_start__ = ADDR(.data);
  __relocate_vma_end__   = ADDR(.data) + SIZEOF(.data);
  __relocate_lma_start__ = LOADADDR(.data);
  __relocate_lma_end__   = LOADADDR(.data) + SIZEOF(.data);

  .text : {
    *(.rodata .rodata.*)
    *(.text .text.*)
  } >ROM

  .bss (NOLOAD) : ALIGN(4) {
    *(.bss .bss.*)
  } >RAM

  __bss_start__ = ADDR(.bss);
  __bss_end__   = ADDR(.bss) + SIZEOF(.bss);

  .stack (NOLOAD) : {
    *(.stack .stack.*)
  } >RAM

  _estack = ADDR(.stack) + SIZEOF(.stack);

  __free_ram_start__ = ALIGN(ADDR(.stack) + SIZEOF(.stack), 16);
  __free_ram_end__   = ORIGIN(RAM) + LENGTH(RAM);
}
