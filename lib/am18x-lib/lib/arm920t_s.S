@ tary, 1:17 2011/8/3
#include "arm920t.h"

		.SECTION .text, "ax"
		.arm

		.GLOBAL	arm_intr_disable
arm_intr_disable:
		PUSH	{R0}
		MRS	R0, CPSR
		ORR	R0, R0, # XPSR_F_IRQ | XPSR_F_FIQ
		MSR	CPSR_c, R0
		POP	{R0}
		MOV	PC, LR

		.GLOBAL	arm_intr_enable
arm_intr_enable:
		PUSH	{R0}
		MRS	R0, CPSR
		BIC	R0, R0, # XPSR_F_IRQ | XPSR_F_FIQ
		MSR	CPSR_c, R0
		POP	{R0}
		MOV	PC, LR

		@ enable MMU
		.GLOBAL arm_mmu_enable
arm_mmu_enable:
		PUSH	{R1}
		MRC	p15, 0, R1, C1, C0, 0
		ORR	R1, #0x1
		MCR	p15, 0, R1, C1, C0, 0
		@ !!! make here one instruction between mcr and mov
		POP	{R1}
		MOV	PC, LR

		@ disable MMU
		.GLOBAL arm_mmu_disable
arm_mmu_disable:
		PUSH	{R1}
		MRC	p15, 0, R1, C1, C0, 0
		BIC	R1, R1, #0x5
		MCR	p15, 0, R1, C1, C0, 0
		@ !!! make here one instruction between mcr and mov
		POP	{R1}
		MOV	PC, LR

		.END
