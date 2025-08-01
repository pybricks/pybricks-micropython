@ SPDX-License-Identifier: MPL-1.0
@ Copyright (c) 2016 Tobias Schießl
#include "exceptionhandler.h"

        .set  MODE_USR, 0x10            
        .set  MODE_FIQ, 0x11
        .set  MODE_IRQ, 0x12
        .set  MODE_SVC, 0x13
        .set  MODE_ABT, 0x17
        .set  MODE_UND, 0x1B
        .set  MODE_SYS, 0x1F            
  

        .equ I_F_BIT, 0xC0 
        .equ MASK_SWI_NUM, 0xFF000000
        .equ MASK_SR_MODE, 0x1F

        @ This will be placed at 0xffff0000 by the linker script
        .section .vector,"ax",%progbits

        .code 32

ResetToEntry:
        B Entry
        B UndefHandlder
        B SWIHandler
        B PrefetchAbortHandler
        B DataAbortHandler
        B UnknownExceptionHandler
        B IRQHandler
        B FIQHandler

UndefHandlder:
        @ Save r0-r12, lr
        stmfd   sp!, {r0-r12, lr}
        mov     r7, #EV3_PANIC_UNDEFINED_INSTR
        b       CommonPanicHandler

PrefetchAbortHandler:
        @ Save r0-r12, lr
        stmfd   sp!, {r0-r12, lr}
        mov     r7, #EV3_PANIC_PREFETCH_ABORT
        b       CommonPanicHandler

DataAbortHandler:
        @ Save r0-r12, lr
        stmfd   sp!, {r0-r12, lr}
        mov     r7, #EV3_PANIC_DATA_ABORT
        b       CommonPanicHandler

UnknownExceptionHandler:
        @ Save r0-r12, lr
        stmfd   sp!, {r0-r12, lr}
        mov     r7, #EV3_PANIC_UNKNOWN
        @ b       CommonPanicHandler
        @ fall through

CommonPanicHandler:
        @ Save SPSR
        mrs     r0, spsr
        stmfd   sp!, {r0}
        @ Switch to previous mode to read its sp/lr
        @ NOTE: Assumes we never use User mode
        and     r0, #MASK_SR_MODE
        mrs     r1, cpsr
        and     r2, r1, #MASK_SR_MODE
        orr     r2, r0
        msr     cpsr, r2
        mov     r3, r13
        mov     r4, r14
        msr     cpsr, r1
        stmfd   sp!, {r3, r4}
        @ Call panic handler
        mov     r0, r7
        mov     r1, sp
        bl      ev3_panic_handler_
        @ Reset
        b       ResetToEntry

ev3_panic_handler_:
        ldr pc, =ev3_panic_handler

FIQHandler:
        STMFD    r13!, {r0-r7, lr}
        LDR      r0, =0xFFFEF600
        LDR      r0, [r0]
        MOV      lr, pc
        LDR      pc, [r0]
        LDMFD    r13!, {r0-r7, lr}
        SUBS     pc, lr, #0x4 
        
@******************************************************************************
@*                  Function Definition of SWI Handler
@******************************************************************************    
@
@ The SWI Handler switches to system mode if the SWI number is 458752. If the
@ SWI number is different, no mode switching will be done. No other SWI are 
@ handled here
@
SWIHandler:
        STMFD    r13!, {r0-r1, r14}       @ Save context in SVC stack
        LDR      r0, [r14, #-4]           @ R0 points to SWI instruction
        BIC      r0, r0, #MASK_SWI_NUM    @ Get the SWI number
        CMP      r0, #458752
        MRSEQ    r1, spsr                 @ Copy SPSR  
        ORREQ    r1, r1, #0x1F            @ Change the mode to System
        MSREQ    spsr_cf, r1              @ Restore SPSR
        LDMFD    r13!, {r0-r1, pc}^       @ Restore registers from IRQ stack

@******************************************************************************
@*                  Function Definition of IRQ Handler
@******************************************************************************    
@
@ The IRQ handler jumps to the ISR of highest priority pending IRQ. The address
@ is taken from the HIPVR2 register, which contains the ISR address of highest
@ pending IRQ. This handler doesnot support nesting.
@
IRQHandler:
        SUB      lr, lr, #0x4                   @ remove offset caused by Interrupt from return adress.
        STMFD    r13!, {r0-r12}                 @ Save context in IRQ stack
        STMFD    r13!, {lr}                     @Save lr in IRQ stack
        MRS      r0, spsr                       @Save spsr
        STMFD    r13!, {r0}

        LDR      r0, =0xFFFEF604                @ AINTC HIPVR2 Register
        LDR      r0, [r0]                       @ Get Entry of ISR-Vector-Table
        PUSH     {r0}                           @ Save r0 (required after ISR)
        MOV      lr, pc                         @ Save return address in LR 
        LDR      pc, [r0]                       @ Jump to ISR
        
        POP      {r0}                           @ Check if ISR was systick_isr_C
        LDR      r1, [r0]
        LDR      r2, =systick_isr_C
        CMP      r1, r2
        BNE      LeaveIRQ                       @ If not we can return from IRQ-Handler
        
        MOV      lr, pc                         @ Otherwise check if any button was pressed
        LDR      pc, =lazy_poweroff_hook           
        
LeaveIRQ:
        LDMFD    r13!, {r0}                     @restore spsr
        MSRNE    spsr_cxsf, r0
        LDMFD    r13!, {lr}                     @ Restore lr from IRQ stack
        LDMFD    r13!, {r0-r12}                 @ Restore registers from IRQ stack
        
        MOVS     pc, lr                         @ Return to program before IRQ

loop:
        B loop
        
@*****************************************************************************
@
@ End of the file
@
    .end
