/**
 *  \file   spi.h
 *
 *  \brief  SPI APIs and macros.
 *
 *   This file contains the driver API prototypes and macro definitions.
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


#ifndef _SPI_H_
#define _SPI_H_

#include "hw_spi.h"
#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/*
** Values that can be passed to  SPIModeConfigure API as flag  to select mode
** of operation of SPI
*/

  /* To Configure SPI to Master Mode */
#define SPI_MASTER_MODE               (SPI_SPIGCR1_MASTER | SPI_SPIGCR1_CLKMOD)

 /* To Configure SPI to Slave Mode */
#define SPI_SLAVE_MODE                0
/***************************************************************************/
/*
** It use to select different data format register avilable in SPI peripheral.
*/
#define SPI_DATA_FORMAT0              SPI_SPIDAT1_DFSEL_FORMAT0

#define SPI_DATA_FORMAT1              SPI_SPIDAT1_DFSEL_FORMAT1

#define SPI_DATA_FORMAT2              SPI_SPIDAT1_DFSEL_FORMAT2

#define SPI_DATA_FORMAT3              SPI_SPIDAT1_DFSEL_FORMAT3
/*************************************************************************/
/*
** Values that can be passed to SPIDat1Config API as flag to
** assert  the CS pin(line) and to disable or Enable counter
*/

 /* Assert CS pin(line)*/
#define SPI_CSHOLD                    SPI_SPIDAT1_CSHOLD
 
/* Enable Counter */ 
#define SPI_DELCOUNT_ENABLE           SPI_SPIDAT1_WDEL

/* Disable Counter */
#define SPI_DELCOUNT_DISABLE          0

/**********************************************************************/
/*
** Values that can be passed to SPIConfigClkFormat API as flag to select
** clock polarity
*/

 /* Active low clock */
#define SPI_CLK_POL_HIGH              SPI_SPIFMT_POLARITY

 /* Active high clock */
#define SPI_CLK_POL_LOW               0

 /* In Phase clock */ 
#define SPI_CLK_INPHASE               0

 /* Out of Phase clock */
#define SPI_CLK_OUTOFPHASE            SPI_SPIFMT_PHASE

/*********************************************************************/
/*
** Values that can be passed to  SPIParityEnable API as flag to select
** type of parity
*/
  /* Odd Parity */
#define SPI_ODD_PARITY                SPI_SPIFMT_PARPOL

 /* Even Parity */
#define SPI_EVEN_PARITY               0

/**********************************************************************/
/*
** Values that can be passed to  SPIIntLevelSet API as flag to map
** interrupts to interrupt line INT1
*/

  /* Data length error interrupt enable level */    
#define SPI_DATALEN_ERR_INTLVL        SPI_SPILVL_DLENERRLVL

 /* Time out interrupt level */ 
#define SPI_TIMEOUT_INTLVL            SPI_SPILVL_TIMEOUTLVL

 /* Parrity interrupt level */
#define SPI_PARITY_ERR_INTLVL         SPI_SPILVL_PARERRLVL

 /* Desynchronized salve interrupt level */
#define SPI_DESYNC_SLAVE_INTLVL       SPI_SPILVL_DESYNCLVL

 /* Bit error interrupt level */
#define SPI_BIT_ERR_INTLVL            SPI_SPILVL_BITERRLVL

 /* Receive overrun interrupt level */
#define SPI_RECV_OVERRUN_INTLVL       SPI_SPILVL_OVRNINTLVL

 /* Receive interrupt level */
#define SPI_RECV_INTLVL               SPI_SPILVL_RXINTLVL

 /* Transmit interrupt level */
#define SPI_TRANSMIT_INTLVL           SPI_SPILVL_TXINTLVL

/********************************************************************/
/*
** Values that can be passed to SPIIntEnable API as flag to
** enable or disable the interrupt
*/

  /* Data length error interrupt */
#define SPI_DATALEN_ERR_INT           SPI_SPIINT0_DLENERRENA

  /* Time out interrupt level */
#define SPI_TIMEOUT_INT               SPI_SPIINT0_TIMEOUTENA

  /* Parrity error interrupt */
#define SPI_PARITY_ERR_INT            SPI_SPIINT0_PARERRENA

  /* Desyncronized slave interrupt */
#define SPI_DESYNC_SLAVE_INT          SPI_SPIINT0_DESYNCENA

  /* Bit error interrupt */
#define SPI_BIT_ERR_INT               SPI_SPIINT0_BITERRENA

  /* Receive overrun interrupt */
#define SPI_RECV_OVERRUN_INT          SPI_SPIINT0_OVRNINTENA

  /* Receive interrupt */
#define SPI_RECV_INT                  SPI_SPIINT0_RXINTENA

  /* Transmit interrupt */
#define SPI_TRANSMIT_INT              SPI_SPIINT0_TXINTENA

  /* DMA request interrupt */
#define SPI_DMA_REQUEST_ENA_INT       SPI_SPIINT0_DMAREQEN

/*****************************************************************/
  /* Transmit buffer is empty  interrupt */
#define SPI_TX_BUF_EMPTY              0x14

  /* Receive buffer is full interrupt */
#define SPI_RECV_FULL                 0x12

  /* Error interrupt */
#define SPI_ERR                       0x11

/****************************************************************/
/*
** Prototypes of SPI driver API.
**
*/
void SPIClkConfigure(unsigned int baseAdd, unsigned int moduleClk,
                     unsigned int spiClk, unsigned int dataFormat);

void SPIEnable(unsigned int baseAdd);

void SPIReset(unsigned int baseAdd);

void SPIOutOfReset(unsigned int baseAdd);

void SPIModeConfigure(unsigned int baseAdd, unsigned int flag);

void SPIPinControl(unsigned int baseAdd, unsigned int idx,
                   unsigned int flag, unsigned int *val);

void SPIDelayConfigure(unsigned int baseAdd, unsigned int c2edelay,
                       unsigned int t2edelay, unsigned int t2cdelay,
                       unsigned int c2tdelay);

void SPIDefaultCSSet(unsigned int baseAdd, unsigned char dcsval);

void SPIDat1Config(unsigned int baseAdd, unsigned int flag, unsigned char cs);

void SPIConfigClkFormat(unsigned int baseAdd, unsigned int flag,
                        unsigned int dataFormat);

void SPITransmitData1(unsigned int baseAdd, unsigned int data);

void SPICSTimerEnable(unsigned int baseAdd, unsigned int dataFormat);

void SPICSTimerDisable(unsigned int baseAdd, unsigned int dataFormat);

void SPICharLengthSet(unsigned int baseAdd, unsigned int flag,
                      unsigned int dataFormat);

void SPIShiftMsbFirst(unsigned int baseAdd, unsigned int dataFormat);

void SPIShiftLsbFirst(unsigned int baseAdd, unsigned int dataFormat);

void SPIParityEnable(unsigned int baseAdd, unsigned int flag,
                     unsigned int dataFormat);

void SPIParityDisable(unsigned int baseAdd, unsigned int dataFormat);

void SPIWdelaySet(unsigned int baseAdd, unsigned int flag,
                  unsigned int dataFormat);

void SPIWaitEnable(unsigned int baseAdd, unsigned int dataFormat);

void SPIWaitDisable(unsigned int baseAdd, unsigned int dataFormat);

void SPIIntLevelSet(unsigned int baseAdd, unsigned int flag);

void SPIIntEnable(unsigned int baseAdd, unsigned int flag);

void SPIIntDisable(unsigned int baseAdd, unsigned int flag);

void SPIIntStatusClear(unsigned int baseAdd, unsigned int flag);

unsigned int SPIIntStatus(unsigned int baseAdd, unsigned int flag);

unsigned int SPIDataReceive(unsigned int baseAdd);

unsigned int  SPIInterruptVectorGet(unsigned int baseAdd);

#ifdef __cplusplus
}
#endif
#endif
