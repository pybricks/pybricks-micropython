        .global ExceptionHandler
        .global IRQHandler
        .global SWIHandler
        
        .set  MODE_USR, 0x10            
        .set  MODE_FIQ, 0x11
        .set  MODE_IRQ, 0x12
        .set  MODE_SVC, 0x13
        .set  MODE_ABT, 0x17
        .set  MODE_UND, 0x1B
        .set  MODE_SYS, 0x1F            
  

        .equ I_F_BIT, 0xC0 
        .equ MASK_SWI_NUM, 0xFF000000

        .text

        .code 32

@ Default entry for all vecotrs except Reset is "B ExceptionHandler" so each interrupt would cause a reset
@ Since I can only say that the IRQ Handler works, we leave the remaining ones as they are
ExceptionHandler:
        B Entry
        B ExceptionHandler
        B SWIHandler
        B ExceptionHandler
        B ExceptionHandler
        B ExceptionHandler
        B IRQHandler
        NOP @ Fall to FIQHandler
        
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
        BNE      SkipButtons                    @ If not we can return from IRQ-Handler
        
        MOV      lr, pc                         @ Otherwise check if any button was pressed
        LDR      pc, =check_EV3_buttons           
        LDR	 r0, =callevel                      @ Set callevel to TCL_ISR2 (2) and call user_1ms_isr_type2
        LDRB     r2, [r0]
        PUSH     {r2}                           @ Save the current callevel to restore it afterwards
	MOV	 r1, #2
	STRB	 r1, [r0]
        MOV      lr, pc
        LDR      pc, =user_1ms_isr_type2
        MOV      lr, pc
        LDR      pc, =SetDispatch
        LDR	 r0, =callevel                      @ Restore previous callevel
        POP      {r2}                           
        STRB     r2, [r0]
        
        LDR      r0, =addrShouldDispatch        @ Check if we need to dispatch tasks (double dererencation necessary since it does not work otherwise)
        LDR      r2, [r0]
        LDR      r3, [r2]
        CMP      r3, #0
        BEQ      SkipButtons
        MOV      r1, #0                         @ Reset the shouldDispatch-flag
        STR      r1, [r2]

        
        LDMFD    r13!, {r0}                     @restore spsr
        MSRNE    spsr_cxsf, r0
        LDMFD    r13!, {r2}                     @ pop lr from IRQ stack to r2
        MSR      cpsr_c, #MODE_SYS|I_F_BIT      @ Change to System MOde
        STMFD    r13!, {r2}                     @ push lr from r2 to User/System stack
        MSR      cpsr_c, #MODE_IRQ|I_F_BIT      @ change to IRQ mode
 
        
        LDMFD    r13!, {r0-r12}                 @ Restore registers from IRQ stack
        
        LDR      lr, =dispatch_irq
        MOVS     pc, lr                         @ MOVS = Return from exception (i.e. change back to system mode)
        
SkipButtons:
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
