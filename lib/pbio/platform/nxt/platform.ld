/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (c) 2007,2008 the NxOS developers
 * See AUTHORS for a full list of the developers.
 * Copyright (c) 2023 The Pybricks Authors
 */

/* NXT linker script source.
 *
 *  Notes on alignment:
 *  1) Sections should be 4-byte aligned otherwise ARM fetches will be
 *     misaligned.
 *  2) The stack should be 8-byte aligned for the APCS. That's because
 *     STRD and LDRD assume that they are on 8-byte boundaries.
 */

/*
 * Memory definitions.
 */

MEMORY {
rom : ORIGIN = 1M, LENGTH =256k
vector_ram : ORIGIN = 2M, LENGTH = 64
ram : ORIGIN = 2M + 64, LENGTH = 64K - 64
}

ROM_BASE = 1M;
ROM_SIZE = 256k;
RAM_BASE = 2M;
RAM_SIZE = 64k;
SAMBA_RESERVED_SIZE = 8k;

SECTIONS {
  /*
   * Interrupt vectors. These are loaded to the bottom of memory at
   * boot time.
   */
  .vectors : ALIGN(4) {
    KEEP(*(.isr_vector))
  } > vector_ram
  AT> rom


  /*
   * This section contains code that is relocated to RAM before
   * execution.
   *
   * This section is used for code that has to reside in ram to
   * function correctly (like flash writing code)
   */
  .ram_text : ALIGN(4) {
    *.oram (*.text *.text.* *.glue*)
    . = ALIGN(4);
    *.oram (*.rodata *.rodata.*)
  } > ram
  AT> rom


  /*
   * Read/Write initialized data. Runs from RAM, gets copied there
   * from ROM if booting from flash.
   */
  .data : ALIGN(4) {
    * (*.data *.data.*)
  } > ram
  AT> rom


  /*
   * The main kernel executable code, as well as all the read-only
   * data. If possible, this goes into flash, to free up more RAM.
   */
  .text : ALIGN(4) {
    * (*.text *.text.* *.glue*)
    . = ALIGN(4);
    * (*.rodata *.rodata.*)
  } > rom


  /*
   * The BSS section is zero-initialized data. The section does not
   * take any space in the final image, but the linker helpfully
   * defines the symbols we need to be able to initialize the section
   * properly.
   */
  .bss : ALIGN(4) {
    * (*.bss *.bss.*)
  } > ram

  .noinit (NOLOAD) :
  {
      . = ALIGN(4);
      *(.noinit)
  } > ram

  /*
   * The various kernel stacks.
   *
   * Although not strictly required, this zone is zeroed out at boot
   * time to provide a clean environment for the kernel.
   */
  .stack : ALIGN(8) {
    /* irq stack */
    . += 0x60; /* 96b, which is 3*8*4, the maximum irq stack size. */
    . = ALIGN(8);
    __irq_stack__ = .;

    /* supervisor stack */
    __supervisor_stack_bottom__ = . ;
    . += 0x800;
    . = ALIGN(8);
    __supervisor_stack__ = .;
    __supervisor_stack_top__ = . ;
  } > ram


  /* SAMBA stage 0.5 */

  .name :
  {
      *(.name) /* customizable hub name */
  } > rom

  /*
   * The magic metadata struct is tacked onto the start of the kernel binary.
   */
   .magic : {
    /* Magic identifier */
    LONG(0xDEADBEEF);
    /* Kernel in-ram size */
    LONG(SIZEOF(.vectors) + SIZEOF(.ram_text) +
         SIZEOF(.data) + SIZEOF(.bss) + SIZEOF(.stack)
     );
    /* Kernel in-rom size */
    LONG(SIZEOF(.vectors) + SIZEOF(.ram_text) + SIZEOF(.data) + SIZEOF(.text));
    /* Write address */
    LONG(ROM_BASE);
    /* Bootup address */
    LONG(ROM_BASE);
    /* SAM-BA kernel? */
    BYTE(0);
  } > rom

  /* Discard anything not mapped at this point. */
  /DISCARD/ : {
    * (*);
  }

  /*
   * Symbol definitions for the use of the kernel code.
   */
  __vectors_ram_start__  = ADDR(.vectors);
  __vectors_ram_end__    = (ADDR(.vectors) + SIZEOF(.vectors));
  __vectors_load_start__ = LOADADDR(.vectors);
  __vectors_load_end__   = (LOADADDR(.vectors) + SIZEOF(.vectors));

  __ramtext_ram_start__ = ADDR(.ram_text);
  __ramtext_ram_end__   = ADDR(.ram_text) + SIZEOF(.ram_text);
  __ramtext_load_start__ = LOADADDR(.ram_text);
  __ramtext_load_end__ = __ramtext_load_start__ + SIZEOF(.ram_text) ;

  __data_ram_start__ = ADDR(.data);
  __data_ram_end__   = ADDR(.data) + SIZEOF(.data);
  __data_load_start__ = LOADADDR(.data);
  __data_load_end__ = __data_load_start__ + SIZEOF(.data) ;

  __text_start__ = ADDR(.text);
  __text_end__ = ADDR(.text) + SIZEOF(.text);

  __bss_start__ = ADDR(.bss);
  __bss_end__   = (ADDR(.bss) + SIZEOF(.bss));

  pbdrv_stack_start = ADDR(.stack);
  pbdrv_stack_end = (ADDR(.stack) + SIZEOF(.stack));

  __boot_from_samba__ = 0;

  __rom_userspace_start__ = ADDR(.text) + SIZEOF(.text);
  __rom_userspace_end__ = (ROM_BASE + ROM_SIZE);

  __ram_userspace_start__ = pbdrv_stack_end;
  __ram_userspace_end__ = (RAM_BASE + RAM_SIZE);
}
