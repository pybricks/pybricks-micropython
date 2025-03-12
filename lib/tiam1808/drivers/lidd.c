/**
 *  \file   lidd.c
 *
 *  \brief  LIDD device abstraction layer
 *
 *   This file contains the device abstraction layer APIs for LIDD mode of LCDC
 */

/*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
*/
/*
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

/* HW Macros */
#include "hw_types.h"

/* Driver APIs */
#include "lidd.h"


/*******************************************************************************
*                       DAL API DEFINITIONS
*******************************************************************************/
/**
* \brief  This function configures clkdiv to generate required frequency of
*         of device clock and selects the LIDD mode of operation \n
*
* \param  baseAddr   is the base address of LCD controller \n
* \param  freq       is the required device I/O clock frequency.\n
* \param  moduleFreq is the module i/p freq to LCD module from PLL.\n
*
* \return none.
*
* NOTE: This is the first API to be called
**/
void LIDDClkConfig(unsigned int baseAddr, unsigned int freq,
                   unsigned int moduleFreq)
{
    HWREG(baseAddr + LCDC_LCD_CTRL) = ((moduleFreq / freq) & 0xFF) \
                                        <<  LCDC_LCD_CTRL_CLKDIV_SHIFT;
}

/**
 * \brief  This function returns the status of the LCD controller
 *
 * \param  baseAddr   is the base address of LCD controller \n
 *
 * \return LCD status bit mask
**/
unsigned int LIDDStatusGet(unsigned int baseAddr)
{
    return (HWREG(baseAddr + LCDC_LCD_STAT));
}

/**
 * \brief  This function configures the mode of LIDD operation
 *
 * \param  baseAddr  is the base address of the LCD controller \n
 * \param  mode      is the mode in which the LIDD interface is configured \n
 *
 * type can take the following values
 *  LIDD_MODE_SYNC_MPU68\n
 *  LIDD_MODE_ASYNC_MPU68\n
 *  LIDD_MODE_SYNC_MPU80\n
 *  LIDD_MODE_ASYNC_MPU80\n
 *  LIDD_MODE_HITACHI\n
 *
 *  \return none
 *
 *  NOTE: LIDDModeSet, LIDDPolaritySet, LIDDDMAConfigSet act on same register.
 *  Hence to start with, first LIDDModeSet is to be called and then the others
 **/

void LIDDModeSet(unsigned int baseAddr, unsigned int mode)
{
    HWREG(baseAddr + LCDC_LIDD_CTRL) = mode & LCDC_LIDD_CTRL_LIDD_MODE_SEL;
}

/**
 * \brief This function configures the mode of polarity for CS/Strobe/Enable signals
 *
 * \param  baseAddr  is the base address of the LCD controller
 * \param  polarity  is the polarity for the CS/Strobe/Enable signals
 *
 * \return none
 *
 * NOTE:\n
 * (1) combined polarity should be provided using LIDD_CS_STROBE_POLARITY(...) \n
 * (2) LIDDModeSet, LIDDPolaritySet, LIDDDMAConfigSet act on same register.\n
 *     Hence to start with, first LIDDModeSet is to be called and then the others
 **/
void LIDDPolaritySet(unsigned int baseAddr, unsigned int polarity)
{
    HWREG(baseAddr + LCDC_LIDD_CTRL) |= polarity;
}

/**
 * \brief This function enables/disbles the DMA and DMA DONE interrupt
 *
 * \param baseAddr   is the base address of the LCD controller \n
 * \param dmaEnable  is a flag, when 0 disables the DMA, 1 enables the DMA
 * \param doneEnable is a flag, when 0 disables DONE and 1 enables DONE interrupt
 *
 * \return none
 *
 * NOTE: \n
 * (1) If DMA is disabled then DONE is also disabled and doneEnable is ignored
 * (2) LIDDModeSet, LIDDPolaritySet, LIDDDMAConfigSet act on same register.\n
 *     Hence to start with, first LIDDModeSet is to be called and then the others
 **/
void LIDDDMAConfigSet(unsigned int baseAddr, unsigned int dmaEnable,
                   unsigned int doneEnable)
{
    if (1 == dmaEnable)
    {
        HWREG(baseAddr + LCDC_LIDD_CTRL) |= LCDC_LIDD_CTRL_LIDD_DMA_EN;
        if (1 == doneEnable)
        {
            HWREG(baseAddr + LCDC_LIDD_CTRL) |= LCDC_LIDD_CTRL_DONE_INT_EN;
        }
    }
    else
    {
        /* If DMA is disabled then DONE is also disabled */
        HWREG(baseAddr + LCDC_LIDD_CTRL) &= ~(LCDC_LIDD_CTRL_DONE_INT_EN |\
                                                 LCDC_LIDD_CTRL_LIDD_DMA_EN);
    }
}

/**
 * \brief This function configures the CS to which the DMA should transfer
 *
 * \param baseAddr  is the base address of the LCD controller \n
 * \param cs        is the CS to which the DMA should transfer\n
 *
 * \return none
 *
 * NOTE:\n
 * (1) LIDDModeSet, LIDDPolaritySet, LIDDDMAConfigSet, LIDDDMACSSet act on same \n
 *     register. Hence to start with, first LIDDModeSet is to be called and \n
 *     then the others
 **/
void LIDDDMACSSet(unsigned int baseAddr, unsigned int cs)
{
    /* CS can only be 0 or 1 */
    cs &= 0x1;
    HWREG(baseAddr + LCDC_LIDD_CTRL) |= ((cs << LCDC_LIDD_CTRL_DMA_CS0_CS1_SHIFT) & \
                                          LCDC_LIDD_CTRL_DMA_CS0_CS1)  ;
}

/**
 * \brief This function configures the timing parameters for the strobe signals
 *        on the given chip select
 *
 * \param baseAddr  is the base address of the LCD controller\n
 * \param cs        is the chip select for which the timing is configured\n
 * \param conf      is the strobe timing configuration for cs
 *
 * \return none
 *
 * NOTE:\n
 * The strobe timing configuration includes, r/w set up times, r/w strobe times\n
 * and the turnaround times . These make up to 7 parameters which are combined into\n
 * one register value by LIDD_CS_CONF(...). LIDD_CS_CONF(...) should be used to\n
 * specify the combined timing configuration for CS
 **/
void LIDDCSTimingConfig(unsigned int baseAddr, unsigned int cs, unsigned int conf)
{
    /* CS can only be 0 or 1 */
    HWREG(baseAddr + LCDC_LIDD_CS_CONF(cs & 0x1)) = conf;
}

/**
 * \brief This function sets the address index for a chip select
 *
 * \param baseAddr is the base address of the LCD controller \n
 * \param cs       is the chip select which is to be affected \n
 * \param index    is the address index to be set
 *
 * \return none
 **/
void LIDDAddrIndexSet(unsigned int baseAddr, unsigned int cs, unsigned int index)
{
    HWREG(baseAddr + LCDC_LIDD_CS_ADDR(cs & 0x1)) = index & LCDC_LIDD_CS0_ADDR_ADR_INDX;
}

/**
 * \brief This function sets the data for a chip select
 *
 * \param baseAddr is the base address of the LCD controller \n
 * \param cs       is the chip select which is to be affected \n
 * \param data     is the data to be set
 *
 * \return none
 **/
void LIDDDataWrite(unsigned int baseAddr, unsigned int cs, unsigned int data)
{
    HWREG(baseAddr + LCDC_LIDD_CS_DATA(cs & 0x1)) = data & LCDC_LIDD_CS0_DATA_DATA;
}

/**
 * \brief This function sets a data of a given length
 *
 * \param baseAddr is the base address of the LCD controller \n
 * \param cs       is the chip select which is to be affected \n
 * \param start    is the starting address index \n
 * \param data     the pointer to the data buffer \n
 * \param len      the length of the data
 *
 * \return none
 **/
void LIDDStringWrite(unsigned int baseAddr, unsigned int cs, unsigned int start,
                     char *data, unsigned int len)
{
    unsigned int i;
    for (i =0; i < len; i++)
    {
        HWREG(baseAddr + LCDC_LIDD_CS_ADDR(cs & 0x1)) = \
                                    ((start + i) & LCDC_LIDD_CS0_ADDR_ADR_INDX);
        HWREG(baseAddr + LCDC_LIDD_CS_DATA(cs & 0x1)) = \
                                    (*(data + i) & LCDC_LIDD_CS0_DATA_DATA);
    }
}
