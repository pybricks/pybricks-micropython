@ tary, 12:58 2013/3/3
#include "am18x_map_s.h"

#define UART2_PSC_NR		0x0D

		.section .text
		.arm
		.global output_a_char
output_a_char:
		MOV	R12, R0

		@ kick off
		LDR	R0, =SYSCFG0_BASE
		LDR	R1, =KICK0R_UNLOCK
		LDR	R2, =KICK1R_UNLOCK
		STR	R1, [R0, #0x38]
		STR	R2, [R0, #0x3C]

		@ uart module state transition
		LDR	R0, =PSC1_BASE
		LDR	R1, =MDCTLx_STATE_Enable

1:		// Wait until PTSTAT == 0
		LDR	R3, [R0, #0x128]
		TST	R3, #1
		BNE	1b

		// if MDSTATx[UART2] != MDCTLx_STATE_Enable
		MOV	R3, #UART2_PSC_NR
		ADD	R3, R3, #0x200
		LDR	R2, [R0, R3, LSL #2]
		AND	R2, R2, #0x1F
		SUBS	R2, R2, R1
		BEQ	__skip_psc

		// R2 = MDCTLx[UART2]
		MOV	R3, #UART2_PSC_NR
		ADD	R3, R3, #0x280
		LDR	R2, [R0, R3, LSL #2]

		// MDCTLx[UART2] |= R1
		BIC	R2, R2, #0x7
		ORR	R2, R2, R1
		STR	R2, [R0, R3, LSL #2]

		// PTCMD = 1
		LDR	R3, [R0, #0x120]
		ORR	R3, R3, #1
		STR	R3, [R0, #0x120]

2:		// Wait until PTSTAT == 0
		LDR	R3, [R0, #0x128]
		TST	R3, #1
		BNE	2b

3:		// Wait until MDSTATx[UART2] == MDCTLx_STATE_Enable
		MOV	R3, #UART2_PSC_NR
		ADD	R3, R3, #0x200
		LDR	R2, [R0, R3, LSL #2]
		AND	R2, R2, #0x1F
		SUBS	R2, R2, R1
		BNE	3b
__skip_psc:

		@ AM1808 UART2 pinmux setting
		// R2 = PINMUXx[4]
		LDR	R0, =SYSCFG0_BASE
		LDR	R3, =4
		ADD	R3, R3, #0x48
		LDR	R2, [R0, R3, LSL #2]

		// PINMUXx[4] = XX22XXXX
		BIC	R2, R2, #0x00FF0000
		ORR	R2, R2, #0x00220000
		STR	R2, [R0, R3, LSL #2]

		@ AM1808 UART setting
		LDR	R0, =UART2_BASE

		LDR	R1, =0x6003
		STR	R1, [R0, #0x30]

		LDR	R1, =0x03
		STR	R1, [R0, #0x0C]

		// DLH:DLL = 0x0008
		// oscin = 24000000 Hz
		// UART working clock (PLLx_SYSCLK2) = oscin / 2 = 12000000 Hz
		// UART bit clock =  PLLx_SYSCLK2 / 13 Hz
		// DLH:DLL = UART bit clock / ( 115200 Hz )
		LDR	R1, =0x08
		STR	R1, [R0, #0x20]
		MOV	R1, #0x00
		STR	R1, [R0, #0x24]

		LDR	R1, =0x01
		STR	R1, [R0, #0x34]

		@ wait buffer empty
		LDR	R0, =UART2_BASE
4:
		LDR	R1, [R0, #0x14]
		TST	R1, #(0x1 << 5)
		BEQ	4b

		@ output a char to uart
		@LDR	R1, ='X'
		MOV	R1, R12
		STR	R1, [R0, #0x00]

		MOV	PC, LR
		.ltorg
		.end
