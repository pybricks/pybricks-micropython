/** @file memmap.h
 *  @brief Documentation group definitions.
 *
 * Kernels often need to know where things are in RAM and how they
 * started up, if only to know where the code expects the stack to be,
 * or where there is free space that can be
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_MEMMAP_H__
#define __NXOS_BASE_MEMMAP_H__

#include "base/types.h"

/** @addtogroup kernel */
/*@{*/

/** @defgroup memmap Memory map
 *
 * These are just a bunch of definitions that are filled in by the
 * linker. A kernel can use these to figure out where things are in RAM
 * and how it started up (bootloader or a bare metal ROM bootup).
 *
 * For each memory region, three macro symbols are defined: a start
 * pointer, an end pointer, and a size, which is just (end-start) with
 * the arithmetic done right.
 */
/*@{*/

/** @cond DOXYGEN_SKIP */

/* The following constants are defined by GNU ld at the linking
 * phase. They describe the memory map of the NXT in terms of
 * symbols.
 */
extern U8 __userspace_start__;
extern U8 __userspace_end__;

extern U8 __ramtext_ram_start__;
extern U8 __ramtext_ram_end__;

extern U8 __text_start__;
extern U8 __text_end__;

extern U8 __data_ram_start__;
extern U8 __data_ram_end__;

extern U8 __bss_start__;
extern U8 __bss_end__;

extern U8 __stack_start__;
extern U8 __stack_end__;

extern U8 __boot_from_samba__;

/* Helper macro that converts a symbol value into a regular
 * integer. If we just addressed eg. __free_ram_start__ directly, the
 * C compiler would dereference and give us some random value (or a
 * data abort).
 *
 * So, we need to grab the symbol's address, and then cast it to a
 * pointer to U8. That way, we have pointers pointing to the correct
 * place.
 */
#define SYMADDR(sym) ((U8*)&(sym))

/* Section sizes requires a little ugly casting, so we define it once
 * here as well.
 */
#define SECSIZE(start, end) ((U32)(end - start))

/** @endcond */

/*
 * Wrapping of the raw symbols into something usable by the rest of
 * the kernel. We also define a _SIZE constant for each section, which
 * is just the number of bytes it uses.
 */

/** @name RAM text section
 *
 * RAM text is code that must be placed in RAM to function
 * correctly. This is usually code that writes to the flash memory,
 * since the NXT's flash controller is single-plane, and therefore
 * cannot be read and written simultaneously.
 */
/*@{*/
#define NX_RAMTEXT_START SYMADDR(__ramtext_ram_start__)
#define NX_RAMTEXT_END SYMADDR(__ramtext_ram_end__)
#define NX_RAMTEXT_SIZE SECSIZE(NX_RAMTEXT_START, NX_RAMTEXT_END)
/*@}*/

/** @name Text section
 *
 * The text section contains the executable code. It is usually placed
 * in ROM to save precious RAM, but will be placed in RAM for a SAM-BA
 * boot.
 */
/*@{*/
#define NX_TEXT_START SYMADDR(__text_start__)
#define NX_TEXT_END SYMADDR(__text_end__)
#define NX_TEXT_SIZE SECSIZE(NX_TEXT_START, NX_TEXT_END)
/*@}*/

/** @name Data section
 *
 * The data section contains statically initialized variables.
 */
/*@{*/
#define NX_DATA_START SYMADDR(__data_ram_start__)
#define NX_DATA_END SYMADDR(__data_ram_end__)
#define NX_DATA_SIZE SECSIZE(NX_DATA_RAM_START, NX_DATA_RAM_END)
/*@}*/

/** @name BSS section
 *
 * The BSS section contains uninitialized variables. Its contents is
 * zeroed out at startup, as required by ISO C.
 */
/*@{*/
#define NX_BSS_START SYMADDR(__bss_start__)
#define NX_BSS_END SYMADDR(__bss_end__)
#define NX_BSS_SIZE SECSIZE(NX_BSS_START, NX_BSS_END)
/*@}*/

/** @name Supervisor stack section.
 *
 * The Supervisor mode stack is the stack that is used when the kernel
 * first runs. It is at a known location in RAM, and is zeroed out at
 * startup.
 */
/*@{*/
#define NX_STACK_START SYMADDR(__stack_start__)
#define NX_STACK_END SYMADDR(__stack_end__)
#define NX_STACK_SIZE SECSIZE(NX_STACK_START, NX_STACK_END)
/*@}*/

/** @name Userspace section.
 *
 * The userspace section is the portion of RAM that is not used by the
 * kernel. Application kernels are free to do whatever they want with
 * this area, and it will not affect the Baseplate in any way.
 */
/*@{*/
#define NX_USERSPACE_START SYMADDR(__userspace_start__)
#define NX_USERSPACE_END SYMADDR(__userspace_end__)
#define NX_USERSPACE_SIZE SECSIZE(NX_USERSPACE_START, NX_USERSPACE_END)
/*@}*/

/** @name Boot method
 *
 * Only one of these symbols evaluates to TRUE, and defines how the
 * kernel was booted: by the SAM-BA bootloader, or a cold startup from
 * ROM.
 */
/*@{*/
#define NX_BOOT_FROM_SAMBA ((U32)SYMADDR(__boot_from_samba__) != 0 ? TRUE : FALSE)
#define NX_BOOT_FROM_ROM (!BOOT_FROM_SAMBA)
/*@}*/

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_MEMMAP_H__ */
