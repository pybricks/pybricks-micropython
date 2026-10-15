/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2006 David Anderson <david.anderson@calixo.net> */

/**
 * NXT bootstrap interface; NXT onboard flashing driver bootstrap.
 */

.text
.align 4
.globl _start

_start:
	/* Initialize the stack */
	mov sp, #0x210000

	/* Preserve old link register */
	stmfd sp!, {lr}

	/* Call main */
	bl do_flash_write

	/* Return */
	ldmfd sp!, {pc}
