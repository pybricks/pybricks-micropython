/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

.code 32
.text
.align 0

	.global nx_atomic_cas32
nx_atomic_cas32:
	mov r2, r0
	swp r0, r1, [r2]
	bx lr


	.global nx_atomic_cas8
nx_atomic_cas8:
	mov r2, r0
	swpb r0, r1, [r2]
	and r0, r0, #0xFF
	bx lr


	.global nx_spinlock_acquire_from_ref
nx_spinlock_acquire_from_ref:
	mov r1, #1
spinlock_loop:
	swpb r1, r1, [r0]
	cmp r1, #0
	bxeq lr
	b spinlock_loop


	.global nx_spinlock_try_acquire_from_ref
nx_spinlock_try_acquire_from_ref:
	mov r1, #1
	swpb r1, r1, [r0]
	cmp r1, #0
	mvneq r0, #1
	movne r0, #0
	bx lr
