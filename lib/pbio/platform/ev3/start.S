@ Code kombiniert aus init.S (TI/system_config/armv5/gcc) und ExceptionHandler.S (TI/system_config/armv5/am1808/gcc)
@ SPDX-License-Identifier: MPL-1.0
@ Copyright (c) 2016 Tobias Schießl

#include "hw_aintc.h"
#include "soc_AM1808.h"

        .global Entry
        .global pbdrv_stack_end
        .global _bss_start
        .global _bss_end

@************************ Internal Definitions ********************************
@
@ Define the stack sizes for different modes. The user/system mode will use
@ the rest of the total stack size
@
        .set  UND_STACK_SIZE, 0x50000
        .set  ABT_STACK_SIZE, 0x50000
        .set  FIQ_STACK_SIZE, 0xF0000
        .set  IRQ_STACK_SIZE, 0xF0000
        .set  SVC_STACK_SIZE, 0x50000

@
@ to set the mode bits in CPSR for different modes
@
        .set  MODE_USR, 0x10
        .set  MODE_FIQ, 0x11
        .set  MODE_IRQ, 0x12
        .set  MODE_SVC, 0x13
        .set  MODE_ABT, 0x17
        .set  MODE_UND, 0x1B
        .set  MODE_SYS, 0x1F

        .equ  I_F_BIT, 0xC0

@**************************** Code Seection ***********************************
        .text

@
@ This code is assembled for ARM instructions
@
        .code 32

@******************************************************************************
@
@******************************************************************************
@
@ The reset handler sets up the stack pointers for all the modes. The FIQ and
@ IRQ shall be disabled during this. Then, clearthe BSS sections, switch to the
@ main() function.
@
Entry:
         @ HACK: The U-Boot Linux loader was doing this, but the ELF loader
         @ does not. Adding this here until we sort out the proper way to handle
         @ caching. But right now, it breaks the display driver and other things
         @ if we don't do this.
         mrc p15, #0, r0, c1, c0,#0            @ Read System Control Register
         bic r0, r0, #0x1000                   @ Clear bit 12 to disable ICache
         bic r0, r0, #0x0004                   @ Clear bit 2 to disable DCache
         mcr p15, #0, r0, c1, c0, #0           @ Write back to System Control Register
         mov r0, #0                            @ Set r0 to 0
         mcr p15, #0, r0, c7, c7, #0           @ Invalidate all caches
         @ END HACK

         MRC p15, 0, r0, c1, c0, 0 @ Load Coprocessor Register C1 to ARM Register r0
         ORR r0, r0, #0x00002000 @ Logical OR --> Set Bit 13
         MCR p15, 0, r0, c1, c0, 0 @ Restore Coprocessor Register C1 from ARM Register r0
@
@ Set up the Stack for Undefined mode
@
         LDR   r0, =pbdrv_stack_end            @ Read the stack address
         MSR   cpsr_c, #MODE_UND|I_F_BIT       @ switch to undef  mode
         MOV   sp,r0                           @ write the stack pointer
         SUB   r0, r0, #UND_STACK_SIZE         @ give stack space
@
@ Set up the Stack for abort mode
@
         MSR   cpsr_c, #MODE_ABT|I_F_BIT       @ Change to abort mode
         MOV   sp, r0                          @ write the stack pointer
         SUB   r0,r0, #ABT_STACK_SIZE          @ give stack space
@
@ Set up the Stack for FIQ mode
@
         MSR   cpsr_c, #MODE_FIQ|I_F_BIT       @ change to FIQ mode
         MOV   sp,r0                           @ write the stack pointer
         SUB   r0,r0, #FIQ_STACK_SIZE          @ give stack space
@
@ Set up the Stack for IRQ mode
@
         MSR   cpsr_c, #MODE_IRQ|I_F_BIT       @ change to IRQ mode
         MOV   sp,r0                           @ write the stack pointer
         SUB   r0,r0, #IRQ_STACK_SIZE          @ give stack space
@
@ Set up the Stack for SVC mode
@
         MSR   cpsr_c, #MODE_SVC|I_F_BIT       @ change to SVC mode
         MOV   sp,r0                           @ write the stack pointer
         SUB   r0,r0, #SVC_STACK_SIZE          @ give stack space
@
@ Set up the Stack for USer/System mode
@
         MSR   cpsr_c, #MODE_SYS|I_F_BIT       @ change to system mode
         MOV   sp,r0                           @ write the stack pointer

@
@ Clear the BSS section here
@
Clear_Bss_Section:

         LDR   r0, =_bss_start                 @ Start address of BSS
         LDR   r1, =(_bss_end - 0x04)          @ End address of BSS
         MOV   r2, #0
Loop:
         STR   r2, [r0], #4                    @ Clear one word in BSS
         CMP   r0, r1
         BLE   Loop                            @ Clear till BSS end


@
@ Enter the start_boot function. The execution still happens in system mode
@
Enter_main:
         LDR   r10, =SystemInit                @ Get the address of SystemInit
         MOV   lr, pc                          @ Dummy return
         BLX   r10                             @ Branch to SystemInit

         LDR   r10, =main                      @ Get the address of main
         MOV   r0, #0                          @ Set the first argument to 0
         MOV   r1, #0                          @ Set the second argument to NULL (0)
         MOV   lr, pc                          @ Dummy return
         BLX   r10                             @ Branch to main

         SUB   pc, pc, #0x08                   @ looping

@*****************************************************************************
@
@ End of the file
@
    .end
