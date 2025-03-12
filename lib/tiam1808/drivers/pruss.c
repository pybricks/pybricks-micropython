/**
 * \file      pruss.c
 *
 * \brief     This file contains device abstraction layer APIs for the
 *            PRU Subsystem. There are APIs here to enable/disable the
 *            PRU instance, Load program to PRU and Write to PRU memory.
 */

/*
* Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
*
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
*
*/

#include "hw/hw_types.h"
#include "pruss.h"
#include "armv7a/am335x/interrupt.h"

/*****************************************************************************/
/*                      INTERNAL MACRO DEFINITIONS                           */
/*****************************************************************************/
#define PRUSSDRV_PRU_RESET      0
#define PRUSSDRV_PRU_DISABLE    1
#define PRUSSDRV_PRU_ENABLE     2

#define PRUSSDRV_MAX_WAIT_EVENTS	7

/**************************************************************************/
/*                     API FUNCTION DEFINITIONS                           */
/**************************************************************************/
/**
 * \brief    Resets PRU: \n
 *
 * \param    pruNum          PRU instance number[0 or 1].
 *
 * \return   0 in case of successful reset, -1 otherwise.
 **/
int PRUSSDRVPruReset(unsigned int pruNum)
{
    if(pruNum == 0)
        HWREG( PRU0CONTROL_PHYS_BASE)  = PRUSSDRV_PRU_RESET;
    else if(pruNum == 1)
        HWREG( PRU1CONTROL_PHYS_BASE ) = PRUSSDRV_PRU_RESET;
    else
        return -1;
    return 0;

}

/**
 * \brief    Disables PRU: \n
 *
 * \param    pruNum          PRU instance number[0 or 1].
 *
 * \return   0 in case of successful disable, -1 otherwise.
 **/
int PRUSSDRVPruDisable(unsigned char pruNum)
{
    if(pruNum == 0)
        HWREG( PRU0CONTROL_PHYS_BASE)  = PRUSSDRV_PRU_DISABLE;
    else if(pruNum == 1)
        HWREG( PRU1CONTROL_PHYS_BASE ) = PRUSSDRV_PRU_DISABLE;
    else
        return -1;
    return 0;
}

/**
 * \brief    Enables PRU: \n
 *
 * \param    pruNum          PRU instance number[0 or 1].
 *
 * \return   0 in case of successful enable, -1 otherwise.
 **/
int PRUSSDRVPruEnable(unsigned char pruNum)
{
    if(pruNum == 0)
        HWREG( PRU0CONTROL_PHYS_BASE)  = PRUSSDRV_PRU_ENABLE;
    else if(pruNum == 1)
        HWREG( PRU1CONTROL_PHYS_BASE ) = PRUSSDRV_PRU_ENABLE;
    else
        return -1;
    return 0;
}


/**
 *
 *  \brief   This function writes the given data to PRU memory
 *
 * \param     pruMem         PRU Memory Macro [DATARAM0_PHYS_BASE
 * \param     wordoffset     Offset at which the write will happen.
 * \param     source_mem     Source memory[ Array of integers ]
 * \param     bytelength     Total number of bytes to be writen
 *
 * pruMem can have values
 *		PRUSS0_PRU0_DATARAM\n
 *		PRUSS0_PRU1_DATARAM\n
 *		PRUSS0_PRU0_IRAM\n
 *		PRUSS0_PRU1_IRAM
 * 		PRUSS0_SHARED_DATARAM (Incase of AM33XX)
 * \return                   0 in case of successful transition, -1 otherwise.
 *
 **/
int PRUSSDRVPruWriteMemory(unsigned int pruMem,
                           unsigned int wordoffset,
                           unsigned int *source_mem,
                           unsigned int bytelength)
{
	unsigned int i, wordlength;

	switch(pruMem)
	{
	case PRUSS0_PRU0_DATARAM:
		pruMem = DATARAM0_PHYS_BASE;
		break;
	case PRUSS0_PRU1_DATARAM:
		pruMem = DATARAM1_PHYS_BASE;
		break;
	case PRUSS0_PRU0_IRAM:
		pruMem = PRU0IRAM_PHYS_BASE;
		break;
	case PRUSS0_PRU1_IRAM:
		pruMem = PRU1IRAM_PHYS_BASE;
		break;
#ifdef	AM33XX
	case PRUSS0_SHARED_DATARAM:
		pruMem = PRUSS_SHAREDRAM_BASE;
#endif
	default:
		return -1;
	}

    //Adjust length as multiple of 4 bytes
    wordlength = (bytelength + 3) >> 2;

    for (i = 0; i < wordlength; i++)
    {
        HWREG( pruMem + (i << 2) + wordoffset ) = source_mem[i];
    }
    return wordlength;
}


/**
 * \brief  This function Generates an INTC event \n
 *
 * \param   eventnum   The INTC Event number.\n
 *
 * \return  0 in case of successful transition, -1 otherwise. \n
 **/
int PRUSSDRVPruSendEvent(unsigned int eventnum)
{
	if(eventnum < 32)
		HWREG(INTC_PHYS_BASE + PRU_INTC_SRSR1_REG) = 1 << eventnum ;
	else
		HWREG(INTC_PHYS_BASE + PRU_INTC_SRSR2_REG) = 1 << (eventnum - 32);
	return  0;
}

/**
 * \brief  This function clears an INTC event \n
 *
 * \param   eventnum   The INTC Event number.\n
 *
 * \return  0 in case of successful transition, -1 otherwise. \n
 **/
int PRUSSDRVPruClearEvent(unsigned int eventnum)
{
	if(eventnum < 32)
		HWREG(INTC_PHYS_BASE + PRU_INTC_SECR1_REG) = 1 << eventnum ;
	else
		HWREG(INTC_PHYS_BASE + PRU_INTC_SECR2_REG) = 1 << (eventnum - 32);
    return 0;
}


/**
 * \brief  This function returns the L3 Memeory Start Address.
 *
 * \param  address  Memory to which address to be copied. \n
 *
 * \return 0 in case of success, -1 otherwise. \n
 **/
int PRUSSDRVMapL3Mem(void **address)
{
    *address = (void*)0x0; // TODOK - Replace with proper Macro
    return 0;
}

/**
 * \brief  This function returns the External memory Start Address.
 *
 * \param  address  Memory to which address to be copied. \n
 *
 * \return 0 in case of success, -1 otherwise. \n
 **/
int PRUSSDRVMapExtMem(void **address)
{
    *address = (void*)0x0; // TODOK - Replace with proper Macro
    return 0;

}
/**
 * \brief  This function returns the address of PRU components.
 *
 * \param   per_id   PRU components' Id.
 *
 * \param  address  Memory to which address to be copied. \n
 *
 * \return 0 in case of success, -1 otherwise. \n
 **/
int PRUSSDRVMapPruMem(unsigned int pru_ram_id, void **address)
{
    switch (pru_ram_id) {
    case PRUSS0_PRU0_DATARAM:
        *address = (void*)DATARAM0_PHYS_BASE;
        break;
    case PRUSS0_PRU1_DATARAM:
        *address = (void*)DATARAM1_PHYS_BASE;
        break;
#ifdef	AM33XX
    case PRUSS0_SHARED_DATARAM:
        *address = (void*) PRUSS_SHAREDRAM_BASE;
        break;
#endif
    default:
        *address = 0;
        return -1;
    }
    return 0;
}

#ifdef	AM33XX

/**
 * \brief  This function returns the base address of peripheral IO modules.
 *
 * \param   per_id   Peripheral Module's Id.
 *
 * \param   address  Memory to which address to be copied. \n
 *
 * \return 0 in case of success, -1 otherwise. \n
 **/
int PRUSSDRVMapPeripheralIO(unsigned int per_id, void **address)
{
    switch (per_id)
	{
    case PRUSS0_CFG:
        *address = (void*)PRUSS_CFG_BASE;
        break;
    case PRUSS0_UART:
        *address = (void*)PRUSS_UART_BASE;
        break;
    case PRUSS0_IEP:
        *address = (void*)PRUSS_IEP_BASE;
        break;
    case PRUSS0_ECAP:
        *address = (void*)PRUSS_ECAP_BASE;
        break;
    case PRUSS0_MII_RT:
        *address = (void*)PRUSS_MIIRT_BASE;
        break;
    case PRUSS0_MDIO:
        *address = (void*)PRUSS_MDIO_BASE;
        break;
    default:
        *address = 0;
        return -1;
    }
    return 0;
}
#endif


/**
 * \brief   Sets System-Channel Map registers. \n
 *
 * \param   sysevt          Channel number.
 * \param   channel	        Host number.
 *
 * \return   None.
 **/
void PRUSSDRVIntcSetCmr(unsigned short sysevt,
                        unsigned short channel)
{
    HWREG(INTC_PHYS_BASE + (PRU_INTC_CMR1_REG + (sysevt & ~(0x3))))
                     |= ((channel & 0xF) << ((sysevt & 0x3) << 3));
}

/**
 * \brief    Sets Channel-Host Map registers. \n
 *
 * \param    channel       Channel number.
 * \param    host	       Host number.
 *
 * \return   None.
 **/
void PRUSSDRVIntcSetHmr(unsigned short channel,
                        unsigned short host)
{
    HWREG(INTC_PHYS_BASE + (PRU_INTC_HMR1_REG + (channel & ~(0x3))) )
          = HWREG(INTC_PHYS_BASE + (PRU_INTC_HMR1_REG + (channel & ~(0x3))))
            | (((host) & 0xF) << (((channel) & 0x3) << 3));
}

void ICSS_Init(void)
{
	/*	ICSS PRCM Enable	*/
	HWREG( 0x44E00C00)      |= 0x2;
	HWREG( 0x44E00C00)      &= 0xFFFFFFFD;

	HWREG(0x44E000E8)      |= 0x2;
	while( (HWREG(0x44E000E8) & (0x00030000)) != 0x0 );

	/*	ICSS PRCM Reset	*/
	HWREG( 0x44E00C00)      |= 0x2;
	HWREG( 0x44E00C00)      &= 0xFFFFFFFD;
}

/*****************************END OF FILE************************************/
