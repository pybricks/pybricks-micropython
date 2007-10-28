#ifndef __NXOS_MEMMAP_H__
#define __NXOS_MEMMAP_H__

#include "mytypes.h"

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
#define RAMTEXT_START SYMADDR(__ramtext_ram_start__)
#define RAMTEXT_END SYMADDR(__ramtext_ram_end__)
#define RAMTEXT_SIZE SECSIZE(RAMTEXT_START, RAMTEXT_END)

#define TEXT_START SYMADDR(__text_start__)
#define TEXT_END SYMADDR(__text_end__)
#define TEXT_SIZE SECSIZE(TEXT_START, TEXT_END)

#define DATA_START SYMADDR(__data_ram_start__)
#define DATA_END SYMADDR(__data_ram_end__)
#define DATA_SIZE SECSIZE(DATA_RAM_START, DATA_RAM_END)

#define BSS_START SYMADDR(__bss_start__)
#define BSS_END SYMADDR(__bss_end__)
#define BSS_SIZE SECSIZE(BSS_START, BSS_END)

#define STACK_START SYMADDR(__stack_start__)
#define STACK_END SYMADDR(__stack_end__)
#define STACK_SIZE SECSIZE(STACK_START, STACK_END)

#define USERSPACE_START SYMADDR(__userspace_start__)
#define USERSPACE_END SYMADDR(__userspace_end__)
#define USERSPACE_SIZE SECSIZE(USERSPACE_START, USERSPACE_END)

#define BOOT_FROM_SAMBA ((U32)SYMADDR(__boot_from_samba__) != NULL ? TRUE : \
                                                                     FALSE)
#define BOOT_FROM_ROM (!BOOT_FROM_SAMBA)

#endif
