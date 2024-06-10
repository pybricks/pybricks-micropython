@tary, 22:42 2012/11/27
@*******************************************************************************
@*                     the startup code for am18x_lib                          *
@*******************************************************************************

#include "linkage.h"
#include "arm920t.h"
#include "am18x_map_s.h"

.equ		usr_stack_size, 0x00001000
.equ		isr_stack_size, 0x00000200
.equ		und_stack_size, 0x00000200
.equ		und_stack_size, 0x00000200

		.section .init, "ax"
		.arm

		.macro	ldr_fix, Rx, symbol
		LDR	\Rx, = \symbol
		ADD	\Rx, \Rx, R9
		.endm

		.global	_start
_start:
__load_addr:
__vector_start:
		B	HandleReset
		LDR	PC, undef_addr
		LDR	PC, swi_addr
		LDR	PC, pabt_addr
		LDR	PC, dabt_addr
		LDR	PC, notused_addr
		LDR	PC, irq_addr
		LDR	PC, fiq_addr
__vector_jump:	.long	HandleReset
undef_addr:	.long	HandleUndef
swi_addr:	.long	HandleSWI
pabt_addr:	.long	HandlePrefetchAbort
dabt_addr:	.long	HandleDataAbort
notused_addr:	.long	HandleNotUsed
irq_addr:	.long	HandleIRQ
fiq_addr:	.long	HandleFIQ
__vector_end:

HandleSWI:
HandleNotUsed:
HandleFIQ:
__dummy_handler:
		B	__dummy_handler


HandleIRQ:
		.extern	c_irq_handler
		SUB	LR, LR, #4
		STMDB	SP!, {R0-R12, LR}

		BL	c_irq_handler

		LDMIA	SP!, {R0-R12, PC}^

HandleReset:
		@ retrive image offset (load address - link address)
		LDR	R0, __link_addr
		ADR	R9, __load_addr
		SUB	R9, R9, R0

		@ enter user mode and set its stack pointer
		MSR	CPSR_c, #XPSR_Mode_System | XPSR_F_FIQ | XPSR_F_IRQ
		ldr_fix R0, stack_usr
		MOV	SP, R0

		MSR	CPSR_c, #XPSR_Mode_IRQ | XPSR_F_FIQ | XPSR_F_IRQ
		ldr_fix	R0, stack_isr
		MOV	SP, R0

		MSR	CPSR_c, #XPSR_Mode_Abort | XPSR_F_FIQ | XPSR_F_IRQ
		ldr_fix	R0, stack_abt
		MOV	SP, R0

		MSR	CPSR_c, #XPSR_Mode_Undefined | XPSR_F_FIQ | XPSR_F_IRQ
		ldr_fix	R0, stack_und
		MOV	SP, R0

		MSR	CPSR_c, #XPSR_Mode_System | XPSR_F_FIQ | XPSR_F_IRQ

		.extern	arm_intr_disable
		BL	arm_intr_disable

		@ fix vector jump address
		MOV	R0, R9
		ldr_fix	R1, __vector_jump
		ldr_fix R2, __vector_end
		BL	__reloc_got

		@ copy vectors to high memory
		LDR	R0, =ARMLocalRAM_BASE
		ldr_fix	R1, __vector_start
		ldr_fix	R2, __vector_end
		BL	__copy_mem

		@ Set the IVT to high memory, leave MMU & caches disabled
		MRC	P15, #0, R0, C1, C0, #0
		BIC	R0, R0, #0x00003300
		BIC	R0, R0, #0x00000087
		ORR	R0, R0, #0x00000002
		ORR	R0, R0, #0x00002000
		MCR	P15, #0, R0, C1, C0, #0
		NOP

		ldr_fix SL, __load_addr

		#if 0
		@ got relocation by __reloc_dyn() instead
		@ relocate got to current address
		.global	__lds_got_start
		.global	__lds_got_end
		MOV	R0, SL
		ldr_fix	R1, __lds_got_start
		ldr_fix	R2, __lds_got_end
		BL	__reloc_got
		#endif

		@ relocate dyn to current address
		.global	__lds_rel_start
		.global	__lds_rel_end
		MOV	R0, SL
		ldr_fix	R1, __lds_rel_start
		ldr_fix R2, __lds_rel_end
		BL	__reloc_dyn

		@ clear BSS area
		.extern	__lds_bss_start
		.extern	__lds_bss_end
		MOV	R0, #0
		@ do not use ldr_fix after __reloc_dyn()
		LDR	R1, =__lds_bss_start
		LDR	R2, =__lds_bss_end
1:
		STR	R0, [R1], #4
		CMP	R1, R2
		BLO	1b

		.extern	low_level_init
		BL	low_level_init

		@ call to main entry point
		.extern	main
		bl	main

_loop:
		B	_loop

		.global get_exec_base
get_exec_base:
		MOV	R0, SL
		MOV	PC, LR

		@ Add every element in [r1,r2) with r0
		.global	__reloc_got
__reloc_got:
		CMP	R1, R2
		MOVHS	PC, LR
1:
		LDR	R4, [R1]
		ADD	R4, R4, R0
		STR	R4, [R1], #4
		CMP	R1, R2
		BLO	1b
		MOV	PC, LR

		@ Add word pointed by odd element in [r1,r2) with r0
		.global	__reloc_dyn
__reloc_dyn:
		CMP	R1, R2
		MOVHS	PC, LR
1:
		LDM	R1!, {R3, R4}
		CMP	R4, #0x17
		BNE	2f
		ADD	R3, R3, R0
		LDR	R4, [R3]
		ADD	R4, R4, R0
		STR	R4, [R3]
2:
		CMP	R1, R2
		BLO	1b
		MOV	PC, LR

		@ copy memory from [r1, r2) to [R0, r0 + r2 - r1)
		.global __copy_mem
__copy_mem:
		CMP	R1, R2
		MOVHS	PC, LR
1:
		LDR	R3, [R1], #4
		STR	R3, [R0], #4
		CMP	R1, R2
		BLO	1b
		MOV	PC, LR


__link_addr:
		.long	__load_addr
		.extern	__lds_got_size
		.func	__lds_got_size
		.long	__lds_got_size
		.ltorg

		.section .stack, "w"
		.balign	0x10
stack_mem:
		.space	usr_stack_size
stack_usr:
		.space	isr_stack_size
stack_isr:
		.space	und_stack_size
stack_abt:
		.space	und_stack_size
stack_und:
		.end
