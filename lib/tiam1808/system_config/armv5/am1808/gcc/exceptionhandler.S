@******************************************************************************
@
@ exceptionhandler.S - Definitions of exception handlers
@
@ Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/  
@ All rights reserved.
@
@******************************************************************************
@
@ Share header file with assembly source code
@
#include "hw_aintc.h"
#include "soc_AM1808.h"

@************************** Global symbols ************************************
        .global IRQHandler
        .global FIQHandler
        .global AbortHandler
        .global SWIHandler
        .global UndefInstHandler
        .global CPUAbortHandler
        
        .equ ADDR_HIPVR1, SOC_AINTC_0_REGS + AINTC_HIPVR(0)
        .equ ADDR_HIPVR2, SOC_AINTC_0_REGS + AINTC_HIPVR(1)
        .equ MASK_SWI_NUM, 0xFF000000
  
@**************************** Text Section ************************************
        .text

@ This source file is assembled for ARM instructions
        .code 32
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
        STMFD    r13!, {r0-r3, r12, r14}  @ Save context in IRQ stack
        LDR      r0, =ADDR_HIPVR2         @ R0 points to address of HIPVR2
        LDR      r1, [r0]                 @ R1 contains address of ISR
        ADD      r14, pc, #0              @ Save return address in LR 
        LDR      pc, [r1]                 @ Go to ISR (still in IRQ mode)
        LDMFD    r13!, {r0-r3, r12, r14}  @ Restore registers from IRQ stack
        SUBS     pc, r14, #0x4            @ Return to program before IRQ
        
@******************************************************************************
@*                  Function Definition of FIQ Handler
@******************************************************************************    
@
@ The FIQ Handler jumps to the ISR of the highest priority pending FIQ. The
@ address is taken from HIPVR1, which contains the ISR address of the highest
@ pending FIQ. This handler doesnot support nesting
@
FIQHandler:
@
@ Save the required context in FIQ stack. 
@
        STMFD    r13!, {r0-r3, r12, r14}  @ Save context in FIQ stack
        LDR      r0, =ADDR_HIPVR1         @ R0 points to address of HIPVR1
        LDR      r1, [r0]                 @ R1 contains address of ISR
        ADD      r14, pc, #0              @ Save return address in LR 
        LDR      pc, [r1]                 @ Go to ISR (still in FIQ mode)
        LDMFD    r13!, {r0-r3, r12, r14}  @ Restore registers from FIQ stack
        SUBS     pc, r14, #0x4            @ Return to program state before FIQ 
        
@******************************************************************************
@*             Function Definition of Abort/Undef Handler
@******************************************************************************    
@
@ The Abort handler goes to the C handler of abort mode. Note that the undefined
@ instruction is not handled separately.
@ if nothing is done in the abort mode, the execution enters infinite loop.
@
AbortHandler:
UndefInstHandler:
@
@ Disable all the interrupts
@
        MRS     r0, cpsr                  @ Read from CPSR
        ORR     r0, r0, #0xC0             @ Clear the IRQ and FIQ bits    
        MSR     cpsr, r0                  @ Write to CPSR
        ADD     r14, pc, #0               @ Store the return address
        LDR     pc, =CPUAbortHandler      @ Go to C handler
@
@ Go to infinite loop if returned from C handler
@
loop0:
        B       loop0                      

@*****************************************************************************
@
@ End of the file
@
    .end
    
    
    
