#ifndef __NXOS_MEMMAP_H__
#define __NXOS_MEMMAP_H__

#include "base/types.h"

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

/* Wrapping of the raw symbols into something usable by the rest of
 * the kernel. We also define a _SIZE constant for each section, which
 * is just the number of bytes it uses.
 */
#define NX_RAMTEXT_START SYMADDR(__ramtext_ram_start__)
#define NX_RAMTEXT_END SYMADDR(__ramtext_ram_end__)
#define NX_RAMTEXT_SIZE SECSIZE(NX_RAMTEXT_START, NX_RAMTEXT_END)

#define NX_TEXT_START SYMADDR(__text_start__)
#define NX_TEXT_END SYMADDR(__text_end__)
#define NX_TEXT_SIZE SECSIZE(NX_TEXT_START, NX_TEXT_END)

#define NX_DATA_START SYMADDR(__data_ram_start__)
#define NX_DATA_END SYMADDR(__data_ram_end__)
#define NX_DATA_SIZE SECSIZE(NX_DATA_RAM_START, NX_DATA_RAM_END)

#define NX_BSS_START SYMADDR(__bss_start__)
#define NX_BSS_END SYMADDR(__bss_end__)
#define NX_BSS_SIZE SECSIZE(NX_BSS_START, NX_BSS_END)

#define NX_STACK_START SYMADDR(__stack_start__)
#define NX_STACK_END SYMADDR(__stack_end__)
#define NX_STACK_SIZE SECSIZE(NX_STACK_START, NX_STACK_END)

#define NX_USERSPACE_START SYMADDR(__userspace_start__)
#define NX_USERSPACE_END SYMADDR(__userspace_end__)
#define NX_USERSPACE_SIZE SECSIZE(NX_USERSPACE_START, NX_USERSPACE_END)

#define NX_BOOT_FROM_SAMBA ((U32)SYMADDR(__boot_from_samba__) != NULL ? TRUE : \
                                                                     FALSE)
#define NX_BOOT_FROM_ROM (!BOOT_FROM_SAMBA)

#endif
