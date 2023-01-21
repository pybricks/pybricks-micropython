/** @file asm_decls.h
 *  @brief Preprocessor declarations for Assembler code.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_ASM_DECLS_H__
#define __NXOS_BASE_ASM_DECLS_H__

/** @addtogroup kernel */
/*@{*/

/** @defgroup asm Declarations for assembler code
 *
 * Assembler code usually needs to touch a lot of the same things:
 * processor modes, instructions that have no mapping in C code, and so
 * forth. These definitions provide symbolic names for a few useful
 * things when writing assembler code.
 */
/*@{*/

/** This bitmask disables IRQ and FIQ handling in the CPSR.
 *
 * It should be <tt>orr</tt>'d or <tt>bic</tt>'d with a CPSR or SPSR.
 */
#define IRQ_FIQ_MASK 0xC0

/** @name Processor modes
 *
 * The 5 least significant bits of the CPSR/SPSR define the processor
 * mode. The following defines give symbolic names to those
 * modes. Please refer to your ARM7 architecture documentation for
 * details about each processor mode, if you do not know what they are.
 */
/*@{*/
#define MODE_USR 0x10 /**< User mode. */
#define MODE_FIQ 0x11 /**< FIQ mode. */
#define MODE_IRQ 0x12 /**< IRQ mode. */
#define MODE_SVC 0x13 /**< Supervisor mode. */
#define MODE_ABT 0x17 /**< Data/Prefetch Abort mode. */
#define MODE_UND 0x1B /**< Undefined instruction mode. */
#define MODE_SYS 0x1F /**< System mode. */

/*@}*/

/** @name Memory locations
 *
 * The following defines symbolic names for "important" points in
 * memory.
 */
/*@{*/

#define MEM_START 0x0 /**< The first address of memory. */
#define ROM_START 0x100000 /**< The first address of flash. */
#define RAM_START 0x200000 /**< The first address of RAM. */

/*@}*/
/*@}*/
/*@}*/

#endif /* __NXOS_BASE_ASM_DECLS_H__ */
