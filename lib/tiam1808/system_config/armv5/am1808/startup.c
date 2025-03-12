/**
 * \file  startup.c
 *
 * \brief Configures the PLL registers to achieve the required Operating
 *        frequency. Power and sleep controller is activated for UART and
 *        Interuppt controller. Interrupt vector is copied to the shared Ram.
 *        After doing all the above, controller is given to the application.
 */

/*
* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "hw_syscfg0_AM1808.h"
#include "hw_syscfg1_AM1808.h"
#include "hw_pllc_AM1808.h"
#include "hw_ddr2_mddr.h"
#include "soc_AM1808.h"
#include "evmAM1808.h"
#include "hw_types.h"
#include "psc.h"

#define E_PASS    0
#define E_FAIL    -1

/***********************************************************************
**                     EXTERNAL FUNCTION PROTOTYPES
***********************************************************************/
extern void Entry(void);
extern void UndefInstHandler(void);
extern void SWIHandler(void);
extern void AbortHandler(void);
extern void IRQHandler(void);
extern void FIQHandler(void);

/**********************************************************************
*                   INTERNAL FUNCTION PROTOTYPES
**********************************************************************/

unsigned int PLL0Init(unsigned char clk_src, unsigned char pllm,
                      unsigned char prediv, unsigned char postdiv,
                      unsigned char div1, unsigned char div3,
                      unsigned char div7);
unsigned int PLL1Init(unsigned char pllm, unsigned char postdiv,
                      unsigned char div1, unsigned char div2,
                      unsigned char div3);
unsigned int DDR_Init ( unsigned int freq );
static void CopyVectorTable(void);
void BootAbort(void);
int main(void);

/******************************************************************************
**                      INTERNAL VARIABLE DEFINITIONS
*******************************************************************************/
static unsigned int const vecTbl[14]=
{
    0xE59FF018,
    0xE59FF018,
    0xE59FF018,
    0xE59FF018,
    0xE59FF014,
    0xE24FF008,
    0xE59FF010,
    0xE59FF010,
    (unsigned int)Entry,
    (unsigned int)UndefInstHandler,
    (unsigned int)SWIHandler,
    (unsigned int)AbortHandler,
    (unsigned int)IRQHandler,
    (unsigned int)FIQHandler
};


/******************************************************************************
**                          FUNCTION DEFINITIONS
*******************************************************************************/

/**
 * \brief   Boot strap function which enables the PLL(s) and PSC(s) for basic
 *          module(s)
 *
 * \param   none
 *
 * \return  None.
 * 
 * This function is the first function that needs to be called in a system.
 * This should be set as the entry point in the linker script if loading the
 * elf binary via a debugger, on the target. This function never returns, but
 * gives control to the application entry point
 **/
unsigned int start_boot(void) 
{

    /* Enable write-protection for registers of SYSCFG module. */
    SysCfgRegistersLock();

    /* Disable write-protection for registers of SYSCFG module. */
    SysCfgRegistersUnlock();

    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_UART2, 0, PSC_MDCTL_NEXT_ENABLE);

    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_AINTC, 0, PSC_MDCTL_NEXT_ENABLE);
    /* Initialize the vector table with opcodes */
    CopyVectorTable();

   /* Calling the main */
    main();

    while(1);
}


static void CopyVectorTable(void)
{
    unsigned int *dest = (unsigned int *)0xFFFF0000;
    unsigned int *src =  (unsigned int *)vecTbl;
    unsigned int count;

    for(count = 0; count < sizeof(vecTbl)/sizeof(vecTbl[0]); count++)
    {
        dest[count] = src[count];
    }
}


void BootAbort(void)
{
    while (1);
}

/***************************** End Of File ***********************************/
