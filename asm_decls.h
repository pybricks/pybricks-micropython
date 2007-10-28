/* Preprocessor definitions for assembler code.
 */

#ifndef __NXOS_ASM_DECLS_H__
#define __NXOS_ASM_DECLS_H__

/* This bitmask disables IRQ and FIQ handling in the CPSR. */
#define IRQ_FIQ_MASK 0xC0

/* Processor modes. */
#define MODE_USR 0x10 /* User */
#define MODE_FIQ 0x11 /* FIQ */
#define MODE_IRQ 0x12 /* IRQ */
#define MODE_SVC 0x13 /* Supervisor */
#define MODE_ABT 0x17 /* Abort */
#define MODE_UND 0x1B /* Undefined */
#define MODE_SYS 0x1F /* System */

#endif /* __NXOS_ASM_DECLS_H__ */
