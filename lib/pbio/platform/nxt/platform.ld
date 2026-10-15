/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (c) 2007,2008 the NxOS developers
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 * Copyright (c) 2023-2026 The Pybricks Authors
 */

/* Linker script for the LEGO MINDSTORMS NXT (AT91SAM7S256).
 *
 * The chip has 256K of flash at 0x100000, organized as 1024 pages of 256
 * bytes and 16 lock regions of 16K, plus 64K of SRAM at 0x200000. SRAM is
 * mirrored at address 0 once startup.S performs the remap command, which is
 * what puts the exception vectors in place, so .vectors has to be the first
 * thing in SRAM.
 *
 * Alignment notes:
 *  1) Sections are 4-byte aligned, otherwise Arm fetches are misaligned.
 *  2) Stacks are 8-byte aligned for the AAPCS, since LDRD/STRD assume it.
 */

MEMORY {
    /* The firmware, installed via SAM-BA. Occupies 15 of the 16 lock
       regions, so that erasing user data can never disturb it. */
    FLASH_FIRMWARE : ORIGIN = 0x00100000, LENGTH = 240K
    /* User data, written by the firmware during shutdown. This is the last
       lock region. Its length must match PBDRV_CONFIG_BLOCK_DEVICE_NXT_SIZE. */
    FLASH_USER     : ORIGIN = 0x00100000 + 240K, LENGTH = 16K
    RAM            : ORIGIN = 0x00200000, LENGTH = 64K
}

/* Start of data storage. */
_pbdrv_block_device_storage_start = ORIGIN(FLASH_USER);

/* Extra heap for large allocations. Only user Image buffers come from here,
   at one byte per pixel, so a full screen is 100 x 64 = 6400 bytes. */
pb_umm_heap_size = 8K;

/* Entry stack for IRQ mode. nx__irq_handler pushes two words per nesting
   level and then switches to supervisor mode, so with eight AIC priority
   levels this only ever needs 64 bytes. */
_irq_stack_size = 128;

/* Entry stack for the abort modes. Both handlers tail-call nx__abort, which
   disables interrupts and halts, so this only needs a few words. */
_abort_stack_size = 128;

/* The supervisor stack is the C stack for the whole firmware, including
   interrupt handlers, and gets all of the RAM that is left over. This is the
   floor below which the link fails rather than silently leaving too little. */
_minimal_stack_size = 8K;

/* Top of the supervisor stack, which grows down from the end of RAM. */
pbdrv_stack_end = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    /* Exception vectors. Must be first in RAM so that they end up at address
       0 after the remap command in startup.S. */
    .vectors : ALIGN(4) {
        KEEP(*(.isr_vector))
    } > RAM AT> FLASH_FIRMWARE

    /* Code that is copied to RAM at startup because it cannot run from
       flash, i.e. the routine that programs the flash. */
    .ram_text : ALIGN(4) {
        *(.ram_text .ram_text.*)
    } > RAM AT> FLASH_FIRMWARE

    /* Initialized data, copied from flash to RAM at startup. */
    .data : ALIGN(4) {
        *(.data .data.*)
    } > RAM AT> FLASH_FIRMWARE

    /* Code and read-only data, executed in place from flash. */
    .text : ALIGN(4) {
        *(.text .text.* .glue*)
        . = ALIGN(4);
        *(.rodata .rodata.*)
    } > FLASH_FIRMWARE

    /* Zero-initialized data, cleared by startup.S. */
    .bss (NOLOAD) : ALIGN(4) {
        *(.bss .bss.*)
        *(COMMON)
    } > RAM

    /* Data that survives a warm restart, plus the umm_malloc heap. This is
       where the block device RAM disk and the display frame buffer live, so
       it is by far the largest consumer of RAM. */
    .noinit (NOLOAD) : ALIGN(4) {
        *(.noinit)

        . = ALIGN(4);
        pb_umm_heap_start = .;
        . = . + pb_umm_heap_size;
        pb_umm_heap_end = .;
    } > RAM

    /* Stacks, zeroed by startup.S. The small exception mode stacks go at the
       bottom so that an overflow of the supervisor stack runs into them
       instead of into live data. */
    .stack (NOLOAD) : ALIGN(8) {
        . += _irq_stack_size;
        . = ALIGN(8);
        __irq_stack__ = .;

        . += _abort_stack_size;
        . = ALIGN(8);
        __abort_stack__ = .;

        /* All remaining RAM is the supervisor stack, so that the size
           reported for this section is the real one. If RAM is overcommitted
           the linker reports "cannot move location counter backwards" here,
           and the difference between the two addresses is how much has to be
           recovered elsewhere. */
        pbdrv_stack_start = .;
        . = ORIGIN(RAM) + LENGTH(RAM);
    } > RAM

    /* Customizable hub name. The firmware metadata reports this section's
       start and size as where to patch the name, so it must hold the name
       and nothing else. In particular, do not align inside the section. */
    .name : {
        *(.name)
    } > FLASH_FIRMWARE

    /* Discard anything not mapped at this point, so that nothing unexpected
       can end up in the firmware image. Note that this also drops debug
       info, including in DEBUG=1 builds. */
    /DISCARD/ : {
        *(*);
    }

    /* Symbols used by startup.S to set up RAM. */
    __vectors_ram_start__  = ADDR(.vectors);
    __vectors_load_start__ = LOADADDR(.vectors);
    __vectors_load_end__   = LOADADDR(.vectors) + SIZEOF(.vectors);

    __ramtext_ram_start__  = ADDR(.ram_text);
    __ramtext_load_start__ = LOADADDR(.ram_text);
    __ramtext_load_end__   = LOADADDR(.ram_text) + SIZEOF(.ram_text);

    __data_ram_start__     = ADDR(.data);
    __data_load_start__    = LOADADDR(.data);
    __data_load_end__      = LOADADDR(.data) + SIZEOF(.data);

    __bss_start__          = ADDR(.bss);
    __bss_end__            = ADDR(.bss) + SIZEOF(.bss);
}

ASSERT(pbdrv_stack_end - pbdrv_stack_start >= _minimal_stack_size,
    "NXT: not enough RAM left for the supervisor stack")
