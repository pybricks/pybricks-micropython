/**
 * \file  interrupt.c
 *
 * \brief Contains the APIs for configuring AINTC.
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

#include "hw_aintc.h"
#include "interrupt.h"
#include "hw_types.h"
#include "soc_AM1808.h"
#include "cpu.h"

/******************************************************************************
**                INTERNAL MACRO DEFINITIONS
******************************************************************************/
#define SYSTEM_INTR_BYTE_SHIFT         (0x03)
#define SYSTEM_INTR_BIT_MASK           (0x07)

/**************** *************************************************************
**                 STATIC VARIABLE DEFINITIONS
******************************************************************************/
static void (*fnRAMVectors[NUM_INTERRUPTS])(void);

/******************************************************************************
**                STATIC FUNCTION DECLARATIONS
******************************************************************************/
static void IntDefaultHandler(void);

/******************************************************************************
**                     API FUNCTION DEFINITIONS
******************************************************************************/

/**
 *
 * The Default Interrupt Handler. 
 *
 * This is the default interrupt handler for all interrupts.  It simply loops
 * forever so that the system state is preserved for observation by a
 * debugger.  Since interrupts should be disabled before unregistering the
 * corresponding handler, this should never be called.
 *
 *
 **/
static void IntDefaultHandler(void)
{
    /* Go into an infinite loop.*/
    while(1)
    {
        ;
    }
}

/**
 * \brief    Registers an interrupt Handler in the interrupt vector table for
 *           system interrupts. 
 * 
 * \param    intrNum - Interrupt Number
 * \param    fnHandler - Function pointer to the ISR
 * 
 * Note: When the interrupt occurs for the sytem interrupt number indicated,
 * the control goes to the ISR given as the parameter.
 * 
 * \return      None.
 **/
void IntRegister(unsigned int intrNum, void (*fnHandler)(void))
{
    /* Assign ISR */
    fnRAMVectors[intrNum] = fnHandler;
}

/**
 * \brief   Unregisters an interrupt
 * 
 * \param   intrNum - Interrupt Number
 * 
 * Note: Once an interrupt is unregistered it will enter infinite loop once
 * an interrupt occurs
 * 
 * \return      None.
 **/
void IntUnRegister(unsigned int intrNum)
{
    /* Assign default ISR */
    fnRAMVectors[intrNum] = IntDefaultHandler;
}

/**
 * \brief  Set the channel number for a system interrupt. Channel numbers 0-1 
 *         are mapped to FIQ and Channel numbers 2-31 are mapped to IRQ of ARM.
 *         One or more system interrupt can be mapped to one channel. However, 
 *         one system interrupt can not be mapped to multiple channels.
 * 
 * \param   intrNum - Interrupt Number
 * \param   channel - Channel Number to be set
 *
 * \return None.
 **/
void IntChannelSet(unsigned int intrNum, unsigned char channel)
{
    volatile unsigned char *CMRByte = (unsigned char*)(SOC_AINTC_0_REGS + 
                                                       AINTC_CMR(0));

    /* 
    ** This function assumes that the CMRs are continuous in memory and are
    ** byte accessible. Also assumes that the architecture is little-endian
    */

    /* Point to the location for the given system interrupt */
    CMRByte += intrNum;

    /* Write the channel number */
    *CMRByte = channel;
}

/**
 * \brief    Get the channel number for a system interrupt
 * 
 * \param    intrNum - Interrupt Number
 * 
 * \return   Channel Number.
 **/
unsigned char IntChannelGet(unsigned int intrNum)
{
    volatile unsigned char *CMRByte = (unsigned char*)(SOC_AINTC_0_REGS + 
                                                       AINTC_CMR(0));

    /* 
    ** This function assumes that the CMRs are continuous in memory and are
    ** byte accessible. Also assumes that the architecture is little-endian
    */

    /* Point to the location for the given system interrupt */
    CMRByte +=  intrNum;

    return (*CMRByte);
}

/**
 * \brief    Enables interrupts in GER register of AINTC. FIQ and IRQ will be
 *           signaled for processing.
 * 
 * \param    None
 * 
 * \return   None
 **/
void IntGlobalEnable(void)
{
    /* Enable Interrupts in GER */
    HWREG(SOC_AINTC_0_REGS + AINTC_GER) = AINTC_GER_ENABLE;
}

/**
 * \brief    Disables interrupts in GER register of AINTC. No interrupts will
 *           be signaled by the AINTC to the ARM core.
 * 
 * \param    None
 * 
 * \return   None
 **/
void IntGlobalDisable(void)
{
    /* Enable Interrupts in GER */
    HWREG(SOC_AINTC_0_REGS + AINTC_GER) &= ~AINTC_GER_ENABLE;
}

/**
 * \brief    Enables the sytem interrupt in AINTC. The interrupt will be 
 *           processed only if it is enabled in AINTC
 * 
 * \param    intrNum - Interrupt number
 *
 * \return   None.
 **/
void IntSystemEnable(unsigned int intrNum)
{
    volatile unsigned char *ESRByte = (unsigned char*)(SOC_AINTC_0_REGS + 
                                                       AINTC_ESR(0));
    unsigned int bitNum;
    
    /* 
    **  This function assumes that the ESRn are continuous in memory and are
    **  byte accessible. Also assumes that the architecture is little-endian
    */
    
    /* Get the byte address corresponding to the system interrupt */
    ESRByte += (intrNum >> SYSTEM_INTR_BYTE_SHIFT);

    /* Get the Bit number of the system interrupt inside the byte*/
    bitNum = intrNum & SYSTEM_INTR_BIT_MASK;

    /* Store back to the ESR register */
    *(ESRByte) |= (0x01 << bitNum);
}

/**
 * \brief   Disables the sytem interrupt in the AINTC
 * 
 * \param   intrNum - Interrupt number
 * 
 * \return  None
 **/
void IntSystemDisable(unsigned int intrNum)
{
    volatile unsigned char *ECRByte = (unsigned char*)(SOC_AINTC_0_REGS + 
                                                       AINTC_ECR(0));
    unsigned int bitNum;

    /* 
    **  This function assumes that the ESRn are continuous in memory and are
    **  byte accessible. Also assumes that the architecture is little-endian
    */
    
    /* Get the byte address corresponding to the system interrupt */
    ECRByte += (intrNum >> SYSTEM_INTR_BYTE_SHIFT);

    /* Get the Bit number of the system interrupt inside the byte*/
    bitNum = intrNum & SYSTEM_INTR_BIT_MASK;

    /* Store back to the ECR register */
    *(ECRByte) = (0x01 << bitNum);
}

/**
 * \brief    Clears a sytem interrupt status in AINTC
 * 
 * \param    intrNum - Interrupt number
 *
 * \return   None
 *
 **/
void IntSystemStatusClear(unsigned int intrNum)
{
    volatile unsigned char *SECRByte = (unsigned char*)(SOC_AINTC_0_REGS + 
                                                        AINTC_SECR(0));
    unsigned int bitNum;
    
    /* Get the byte address corresponding to the system interrupt */
    SECRByte += (intrNum >> SYSTEM_INTR_BYTE_SHIFT);

    /* Get the Bit number of the system interrupt inside the byte*/
    bitNum = intrNum & SYSTEM_INTR_BIT_MASK;

    /* Store back to the SECR register */
    *(SECRByte) = (0x01 << bitNum);
}

/**
 * \brief    Get the raw status of a system interrupt. This will return 1
 *           if the status is set and return 0 if the status is clear.
 * 
 * \param    intrNum - Interrupt number
 * 
 * \return   Raw Interrupt Status 
 * 
 **/
unsigned int IntSystemStatusRawGet(unsigned int intrNum)
{
    volatile unsigned char *SRSRByte = (unsigned char*)(SOC_AINTC_0_REGS + 
                                                        AINTC_SRSR(0));

    unsigned int bitNum;
    
    /*
    **  This function assumes that the SRSRn are continuous in memory
    **  are byte accessible; also that the architecture is little-endian
    */
 
    /* Get the byte address corresponding to the system interrupt */
    SRSRByte += (intrNum >> SYSTEM_INTR_BYTE_SHIFT);

    /* Get the Bit number of the system interrupt inside the byte*/
    bitNum = intrNum & SYSTEM_INTR_BIT_MASK;

    /* return the status of the bit */
    return ((*(SRSRByte) >> bitNum)& 0x01);
}

/**
 * \brief    Get the enabled status of a system interrupt. This will return
 *           1 if the status is set, and return 0 if the status is clear.
 * 
 * \param    intrNum - Interrupt Number
 *  
 * \return   Enabled Interrupt Status.
 * 
 **/
unsigned int IntSystemStatusEnabledGet(unsigned int intrNum)
{
    volatile unsigned char *SECRByte = (unsigned char*)(SOC_AINTC_0_REGS + 
                                                        AINTC_SECR(0));

    unsigned int bitNum;
   
    /*
    **  This function assumes that the SECRn are continuous in memory 
    **  are byte accessible; also that the architecture is little-endian
    */

    /* Get the byte address corresponding to the system interrupt */
    SECRByte += (intrNum >> SYSTEM_INTR_BYTE_SHIFT);

    /* Get the Bit number of the system interrupt inside the byte*/
    bitNum = intrNum & SYSTEM_INTR_BIT_MASK;

    /* return the status of the bit */
    return ((*(SECRByte) >> bitNum)& 0x01);
}

/**
 * \brief    Enables IRQ in HIER register of AINTC
 * 
 * \param    None
 * 
 * \return   None.
 **/
void IntIRQEnable(void)
{
    /* Enable IRQ Interrupts */
    HWREG(SOC_AINTC_0_REGS + AINTC_HIER) |= AINTC_HIER_IRQ;
}

/**
 * \brief    Disables IRQ in HIER register of AINTC
 * 
 * \param    None
 * 
 * \return   None.
 **/
void IntIRQDisable(void)
{
    /* Disable IRQ Interrupts */
    HWREG(SOC_AINTC_0_REGS + AINTC_HIER) &= ~(AINTC_HIER_IRQ);
}

/**
 * \brief    Enables FIQ in AINTC
 * 
 * \param    None
 * 
 * \return   None.
 **/
void IntFIQEnable(void)
{
    /* Enable FIQ Interrupts */
    HWREG(SOC_AINTC_0_REGS + AINTC_HIER) |= AINTC_HIER_FIQ;
}

/**
 * \brief     Disables FIQ host interrupts in  AINTC
 * 
 * \param     None
 * 
 * \return    None
 **/
void IntFIQDisable(void)
{
    /* Disable FIQ Interrupts */
    HWREG(SOC_AINTC_0_REGS + AINTC_HIER) &= ~(AINTC_HIER_FIQ);
}

/**
 * \brief   This API is used to setup the AINTC. This API shall be called 
 *          before using the AINTC. All the host interruptsi will be disabled
 *          after calling this API. The user shall enable all the interrupts
 *          required for processing. 
 *
 * \param   None
 * 
 * \return  None.
 *
 **/
void IntAINTCInit(void)
{
    unsigned int cnt;
  
    /* Reset AINTC */
    for(cnt = 0; cnt < 3; cnt++)
    {
        HWREG(SOC_AINTC_0_REGS + AINTC_ECR(cnt)) = AINTC_ECR_DISABLE;
        HWREG(SOC_AINTC_0_REGS + AINTC_SECR(cnt)) = AINTC_SECR_ENBL_STATUS;
    }

    /* Disable FIQ and IRQ */
    HWREG(SOC_AINTC_0_REGS + AINTC_HIER) &= ~(AINTC_HIER_IRQ | AINTC_HIER_FIQ);

    /* Reset Global Interrupt register to disable global interrupt */
    HWREG(SOC_AINTC_0_REGS + AINTC_GER) = 0x0;

    /* Assign the ISR Table Address to VBR */
    HWREG(SOC_AINTC_0_REGS + AINTC_VBR) = (unsigned int)fnRAMVectors;

    /* Declare ISR Size (Function Pointer = 4 bytes) */
    HWREG(SOC_AINTC_0_REGS + AINTC_VSR) = 0x0;
}

/**
 * \brief  Enables the processor IRQ only in CPSR. Makes the processor to 
 *         respond to IRQs.  This does not affect the set of interrupts 
 *         enabled/disabled in the AINTC.
 *
 * \param    None
 *
 * \return   None
 *
 *  Note: This function call shall be done only in previleged mode of ARM
 **/
void IntMasterIRQEnable(void)
{
    /* Enable IRQ in CPSR.*/
    CPUirqe();

}

/**
 * \brief  Disables the processor IRQ only in CPSR.Prevents the processor to 
 *         respond to IRQs.  This does not affect the set of interrupts 
 *         enabled/disabled in the AINTC.
 *
 * \param    None
 *
 * \return   None
 *
 *  Note: This function call shall be done only in previleged mode of ARM
 **/
void IntMasterIRQDisable(void)
{
    /* Disable IRQ in CPSR.*/
    CPUirqd();
}

/**
 * \brief  Enables the processor FIQ only in CPSR. Makes the processor to 
 *         respond to FIQs.  This does not affect the set of interrupts 
 *         enabled/disabled in the AINTC.
 *
 * \param    None
 *
 * \return   None
 *
 *  Note: This function call shall be done only in previleged mode of ARM
 **/
void IntMasterFIQEnable(void)
{
    /* Enable FIQ in CPSR.*/
    CPUfiqe();
}

/**
 * \brief  Disables the processor FIQ only in CPSR.Prevents the processor to 
 *         respond to FIQs.  This does not affect the set of interrupts 
 *         enabled/disabled in the AINTC.
 *
 * \param    None
 *
 * \return   None
 *
 *  Note: This function call shall be done only in previleged mode of ARM
 **/
void IntMasterFIQDisable(void)
{
    /* Disable FIQ in CPSR.*/
    CPUfiqd();
}

/**
 * \brief   Returns the status of the interrupts FIQ and IRQ.
 *
 * \param    None
 *
 * \return   Status of interrupt as in CPSR.
 *
 *  Note: This function call shall be done only in previleged mode of ARM
 **/
unsigned int IntMasterStatusGet(void)
{
    return CPUIntStatus();
}




/**
 * \brief Read and save the stasus and Disables the processor IRQ .
 *         Prevents the processor to respond to IRQs.  
 *
 * \param    None
 *
 * \return   Current status of IRQ
 *
 *  Note: This function call shall be done only in previleged mode of ARM
 **/


unsigned char IntDisable(void)
{
	unsigned char status;

	/* Reads the current status.*/
	status = (IntMasterStatusGet() & 0xFF);

	/* Disable the Interrupts.*/
	IntMasterIRQDisable();

	return status;
}

/**
 * \brief  Restore the processor IRQ only status. This does not affect 
 *          the set of interrupts enabled/disabled in the AINTC.
 *
 * \param    The status returned by the IntDisable fundtion.
 *
 * \return   None
 *
 *  Note: This function call shall be done only in previleged mode of ARM
 **/


void IntEnable(unsigned char  status)
{
  
	if((status & 0x80) == 0) 
	{
		IntMasterIRQEnable();
	} 
}



/********************************** End Of File ******************************/


