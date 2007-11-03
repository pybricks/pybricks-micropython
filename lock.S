.code 32
.text
.align 0

	.global atomic_cas32
atomic_cas32:
	mov r2, r0
	swp r0, r1, [r2]
	bx lr


	.global atomic_cas8
atomic_cas8:
	mov r2, r0
	swpb r0, r1, [r2]
	and r0, r0, #0xFF
	bx lr


	.global spinlock_acquire_from_ref
nx_spinlock_acquire_from_ref:
	mov r1, #1
spinlock_loop:
	swpb r1, r1, [r0]
	cmp r1, #0
	bxeq lr
	b spinlock_loop


	.global spinlock_try_acquire_from_ref
nx_spinlock_try_acquire_from_ref:
	mov r1, #1
	swpb r1, r1, [r0]
	cmp r1, #0
	mvneq r0, #0
	movne r0, #0
	bx lr
