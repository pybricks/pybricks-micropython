/* Copyright (C) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

.code 32
.section .samba_init, "ax"
.align 0

#include "asm_decls.h"

#define SAMBA_END 0x202000

        .global nx__samba_start
        .extern __kernel_ram_load_size__

/*
 * Execution starts here after a SAM-BA boot.
 */
nx__samba_start:
        /*
         * Disable interrupts. We're going to kill SAM-BA, we don't
         * want pesky IRQs getting in the way.
         */
        msr cpsr_c, #(MODE_SVC | IRQ_FIQ_MASK)

        /*
         * Copy the entire kernel binary down in RAM, squashing
         * SAM-BA.
         */
        ldr r0, =SAMBA_END
        ldr r1, =__kernel_ram_load_size__
        add r1, r1, r0
        mov r2, #RAM_START

copy_loop:
        cmp r0, r1
        ldrlo r3, [r0], #4
        strlo r3, [r2], #4
        blo copy_loop

        /* Branch into the common kernel bootstrap. */
        ldr pc, =RAM_START
