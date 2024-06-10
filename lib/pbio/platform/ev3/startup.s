// To keep this in the first portion of the binary.
.section .text.boot
 
// Make _start global.
.globl _start
 
_start:
    // Setup the stack.
    ldr sp, =__bss_end
 
    // Clear out bss.
    ldr r4, =__bss_start
    ldr r9, =__bss_end
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    b       2f
 
1:
    // store multiple at r4.
    stmia r4!, {r5-r8}
 
    // If we are still below bss_end, loop.
2:
    cmp r4, r9
    blo 1b

    // Call SystemInit
    ldr r3, =SystemInit
    blx r3

    // Call main
    ldr r3, =main
    blx r3

halt:
    wfe
    b halt
