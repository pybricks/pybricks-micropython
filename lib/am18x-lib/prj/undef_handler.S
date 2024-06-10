// tary, 18:53 2011/8/6
#include "arm920t.h"

	.text
	.arm

.global HandleUndef
	@	LR_und	= PC + 4 (arm)
	@	LR_und	= PC + 2 (thumb)
	@		PC is address of undefined instruction fetch
	@	return instruction MOVS PC, R14_und
	@
	@	now in Undefined Instruction Mode
	@	R13 = SP = SP_und
	@	R14 = LR = LR_und
	@	SPSR = SPSR_und = CPSR(saved)
	.func	HandleUndef
HandleUndef:
	PUSH	{R0, R1, V1-V3, LR}

	MRS	V1, SPSR
	ANDS	V3, V1, #XPSR_F_Thumb
	LDREQ	R1, =__arm_mode_fmt
	SUBEQ	V2, LR, #4
	LDRNE	R1, =__thumb_mode_fmt
	SUBNE	V2, LR, #2
	ADR	R0, __undef_fmt
	LDR	V1, =printk
	MOV	LR, PC
	BX	V1

	ADR	R0, __undef_pc_fmt
	MOV	R1, V2
	MOV	LR, PC
	BX	V1

	MOVS	V3, V3
	BNE	1f
0:
	ADR	R0, __undef_arm_instruction
	LDR	R1, [V2, #0]
	B	2f
1:
	ADR	R0, __undef_thumb_instruction
	LDRH	R1, [V2, #0]
	B	2f
2:
	MOV	LR, PC
	BX	V1

	POP	{R0, R1, V1-V3, LR}
	MOVS	PC, LR
__undef_loop:
	B	__undef_loop

	.align	4
	.ltorg

__undef_fmt:
	.string "\n****** Undefined Instruction --- %s mode ******\n"
__arm_mode_fmt:
	.string	"arm"
__thumb_mode_fmt:
	.string "thumb"
__undef_pc_fmt:
	.string "\tPC = 0x%.8X "
__undef_arm_instruction:
	.string "\t[PC] = 0x%.8X\n"
__undef_thumb_instruction:
	.string "\t[PC] = 0x%.4X\n"

	.balign	4
	.endfunc

	.end
