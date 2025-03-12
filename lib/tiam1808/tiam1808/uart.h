/**
 *  \file       uart.h
 *
 *  \brief      This file contains the function prototypes for the device
 *              abstraction layer for UART. It also contains some
 *              related macro definitions and some files to be included.
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


#ifndef __UART_H__
#define __UART_H__

#include <stdio.h>
#include <stdlib.h>
#include "hw_uart.h"

#ifdef __cplusplus
extern "C" {
#endif
#define UART_TX_FIFO_LENGTH           16
#define UART_RX_FIFO_LENGTH           16
#define UART_MAX_TRIAL_COUNT          0x0FFF
#define BAUD_115200                   115200


//***********************************************************************
//  Values that can be used by the application for configuration via APIs
//***********************************************************************/

/***********************************************************************/
/*
** The macros below are used to set the reciever trigger levels.
** One of the macros  below is passed as 'rxLevel' to'UARTFIFOLevelSet', 
** as 'flags' to 'UARTDMAEnable' and to 'UARTDMADisable'.
*/ 

/* This is used to select the receiver trigger level as 1 byte.*/
#define UART_RX_TRIG_LEVEL_1       (UART_FCR_RXFIFTL_CHAR1                  \
                                                << UART_FCR_RXFIFTL_SHIFT)

/* This is used to select the receiver trigger level as 4 bytes.*/
#define UART_RX_TRIG_LEVEL_4       (UART_FCR_RXFIFTL_CHAR4                  \
                                                << UART_FCR_RXFIFTL_SHIFT)

/* This is used to select the receiver trigger level as 8 bytes.*/
#define UART_RX_TRIG_LEVEL_8       (UART_FCR_RXFIFTL_CHAR8                  \
                                                << UART_FCR_RXFIFTL_SHIFT)

/* This is used to select the receiver trigger level as 14 bytes.*/
#define UART_RX_TRIG_LEVEL_14      (UART_FCR_RXFIFTL_CHAR14                 \
                                                << UART_FCR_RXFIFTL_SHIFT)

/* This is used as a mask for the Receiver Trigger Level Selection bits.*/
#define UART_RX_TRIG_LEVEL         UART_FCR_RXFIFTL

/***************************************************************************/

/***************************************************************************/
/*
** These macros are used to control bits in FIFO Control register(FCR).
** These are used in the functions: 'UARTFIFOEnable', 'UARTFIFOLevelSet'
** 'UARTDMAEnable', 'UARTDMADisable'.
** An appropriate combination of the below has to be sent as 'rxLevel' to
** 'UARTFIFOLevelSet', as 'flags' to 'UARTDMAEnable' and 'UARTDMADisable'.
*/

/* This is used to enable/disable the DMA mode of operation.*/
#define UART_DMAMODE                  UART_FCR_DMAMODE1

/* This is used to clear the Transmitter FIFO and to reset the transmitter
 * FIFO counter.*/
#define UART_TX_CLEAR                 UART_FCR_TXCLR

/* This is used to clear the Receiver FIFO and to reset the receiver FIFO
 * counter.*/
#define UART_RX_CLEAR                 UART_FCR_RXCLR

/* This is used to enable/disable the FIFO mode of operation.*/
#define UART_FIFO_MODE                UART_FCR_FIFOEN

/***************************************************************************/

/************************************************************************/
/* 
** These macros are used to set the Parity settings. 
** They are used in the functions 'UARTConfigSetExpClk', 'UARTConfigGetExpClk',
** 'UARTParityModeSet', 'UARTParityModeGet'.
*/

/* This is used to set odd parity.*/
#define UART_PARITY_ODD                 UART_LCR_PEN

/* This is used to configure even parity.*/
#define UART_PARITY_EVEN               (UART_LCR_PEN | UART_LCR_EPS)

/* This is used to configure odd parity with Stick Parity Enable.*/
#define UART_PARITY_STICK_ODD          (UART_PARITY_ODD | UART_LCR_SP)

/* This is used to configure even parity with Stick Parity Enable.*/
#define UART_PARITY_STICK_EVEN         (UART_PARITY_EVEN | UART_LCR_SP)

/* This is used to configure no parity.*/
#define UART_PARITY_NONE                UART_LCR_EPS_ODD

/************************************************************************/

/***************************************************************************/
/*
** These macros are used to control bits in the Line Control Register(LCR).
** These are used in the functions: 'UARTConfigSetExpClk','UARTConfigGetExpClk'
** 'UARTParityModeSet', 'UARTParityModeGet'.
** An appropriate combination of the below has to be sent as 'config' parameter
** to 'UARTConfigSetExpClk',as 'parityMode' to 'UARTParityModeSet'.
*/

/* This is used to select Divisor Latch Access Bit(DLAB).*/
#define UART_DLAB                      UART_LCR_DLAB

/* This is used to enable/disable Break Condition.*/
#define UART_BREAK_CTRL                UART_LCR_BC

/* This is used to enable/disable  Stick Parity Feature.*/
#define UART_STICK_PARITY              UART_LCR_SP

/* This is used to select Even Parity /Odd parity.*/
#define UART_SET_PARITY_TYPE           UART_LCR_EPS

/* This is used to enable/disable Parity feature.*/
#define UART_PARITY                    UART_LCR_PEN

/* This is used to select single/multiple stop bits.*/
#define UART_STOP_BIT                  UART_LCR_STB

/* This can be used as a mask for word length selection bits.*/ 
#define UART_WORDL                     UART_LCR_WLS

/* This is used to select character length as 5 bits per frame.*/
#define UART_WORDL_5BITS               UART_LCR_WLS_5BITS

/* This is used to select character length as 6 bits per frame.*/
#define UART_WORDL_6BITS               UART_LCR_WLS_6BITS

/* This is used to select character lenghth as 7 bits per frame.*/
#define UART_WORDL_7BITS               UART_LCR_WLS_7BITS

/* This is used to select character length as 8 bits per frame.*/
#define UART_WORDL_8BITS               UART_LCR_WLS_8BITS

/****************************************************************************/

/*****************************************************************************/
/*
** These macros are used to analyze bits in Line Status Register(LSR).
** These are used in the functions: 'UARTSpaceAvail','UARTCharsAvail',
** 'UARTCharGet','UARTCharGetNonBlocking'.'UARTCharPut','UARTCharPutNonBlocking'
** 'UARTRxErrorGet', 'UARTRxErrorGet'.
*/

/* This is used to identify if Receiver FIFO error has occured. */
#define UART_RXFIFO_ERROR               UART_LSR_RXFIFOE

/* This is used to identify if both the Transmitter Holding Register(THR) and
 * Transmitter Shift Register(TSR) are empty.*/
#define UART_THR_TSR_EMPTY              UART_LSR_TEMT

/* This is used to identify if the THR alone is empty.*/
#define UART_THR_EMPTY                  UART_LSR_THRE

/* This is used to identify if the Break Indicator bit is set.*/
#define UART_BREAK_IND                  UART_LSR_BI

/* This is used to identify if Framing Error has occured.*/
#define UART_FRAME_ERROR                UART_LSR_FE

/* This is used to identify if Parity Error has occured.*/
#define UART_PARITY_ERROR               UART_LSR_PE

/* This is used to identify if Overrun Error has occured.*/
#define UART_OVERRUN_ERROR              UART_LSR_OE

/* This is used to identify if atleast one full byte is present in the Receiver
 * FIFO(RBR register for non-FIFO mode) and ready to be read.*/
#define UART_DATA_READY                 UART_LSR_DR

/*******************************************************************************/


/*******************************************************************************/
/*
** These macros are used to enable/disable interrupts in the Interrupt Enable
** Register(IER).
** These are used in functions: 'UARTIntEnable', 'UARTIntDisable'.
** An approrpriate combination of the below has to be passed as 'intFlags'
** to 'UARTIntEnable'and also to 'UARTIntDisable'.
*/

/* This is used to enable/disable Modem Status Interrupt.*/
#define UART_INT_MODEM_STAT            UART_IER_EDSSI

/* This is used to enable/disable Line Status Interrupt.*/
#define UART_INT_LINE_STAT             UART_IER_ELSI

/* This is used to enable/disable Transmitter Empty Interrupt.*/
#define UART_INT_TX_EMPTY              UART_IER_ETBEI

/* This is used to enable/disable Receiver Data Available and Character Timeout 
   Interrupt*/
#define UART_INT_RXDATA_CTI            UART_IER_ERBI

/************************************************************************/

/***************************************************************************/
/*
** These macros are used to identify what events have generated interrupts.
** These are used in the function: 'UARTIntStatus'.
** One of the macro below is returned by 'UARTIntStatus'. Refer to 
** definition of 'UARTIntStatus' function for more information.
*/

/* This is used to identify whether FIFO mode is enabled or not.*/
#define UART_FIFOEN_STAT              UART_IIR_FIFOEN

/* This can be used as a mask for the Interrupt Identification(INTID) bits in 
 * the Interrupt Identification Register(IIR).*/
#define UART_INTID                    UART_IIR_INTID

/* This is used to identify whether Transmitter Empty event has generated an
**  interrupt. 
*/
#define UART_INTID_TX_EMPTY           UART_IIR_INTID_THRE                    

/* This is used to identify whether Receiver Data Available event has generated 
 * an interrupt.*/
#define UART_INTID_RX_DATA            UART_IIR_INTID_RDA                     

/* This is used to identify whether Receiver Line Status event has generated
 * an interrupt.*/
#define UART_INTID_RX_LINE_STAT       UART_IIR_INTID_RLS                     

/* This is used to identify whether Character Timeout event has generated an
 * interrupt.*/
#define UART_INTID_CTI                UART_IIR_INTID_CTI		     

/* This is used to identify whether the servicing of any interrupt is pending
 * or not. */
#define UART_INTID_IPEND              UART_IIR_IPEND

/***************************************************************************/

/****************************************************************************/
/*
** These macros are used to control bits in the Modem Control Register(MCR).
** These are used by the functions: 'UARTModemControlSet','UARTModemControlClear'
** 'UARTModemControlGet'.
** An appropriate combination of the below is passed as 'ctrlFlags' to
** 'UARTModemControlSet' and also to 'UARTModemControlClear'.
*/

/* This is used to select Autoflow control feature.*/
#define UART_AUTOFLOW                 UART_MCR_AFE

/* This is used to select Loopback mode feature.*/
#define UART_LOOPBACK                 UART_MCR_LOOP

/* This is used to select OUT2 control bit.*/
#define UART_OUT2_CTRL                UART_MCR_OUT2

/* This is used to select OUT1 control bit.*/
#define UART_OUT1_CTRL                UART_MCR_OUT1

/* This is used to select Request To Send(RTS) bit.*/
#define UART_RTS                      UART_MCR_RTS

/****************************************************************************/

/****************************************************************************/
/* These macros are used to analyze the bits in Modem Status Register(MSR).
** An appropriate combination of the macros below is used in the function:
** 'UARTModemStatusGet'.
*/
#define UART_CD                       UART_MSR_CD
#define UART_RI                       UART_MSR_RI
#define UART_DSR                      UART_MSR_DSR
#define UART_CTS                      UART_MSR_CTS

#define UART_DCD                      UART_MSR_DCD
#define UART_TERI                     UART_MSR_TERI
#define UART_DDSR                     UART_MSR_DDSR
#define UART_DCTS                     UART_MSR_DCTS

/****************************************************************************/
/*
** These macros are used to control bits in Power and Emulation Management
** Register(PWREMU_MGMT).
** These are used in the functions: 'UARTEnable', 'UARTDisable'.
*/

/* This is used to reset and enable/disable the Transmitter.*/
#define UART_TX_RST_ENABLE            UART_PWREMU_MGMT_UTRST

/* This is used to reset and enable/disable the Receiver.*/
#define UART_RX_RST_ENABLE            UART_PWREMU_MGMT_URRST

/* This is used to enable/disable Free Running Mode of operation. */
#define UART_FREE_MODE                UART_PWREMU_MGMT_FREE

/****************************************************************************/

/****************************************************************************/
/*
** These macros are used to control bits in Mode Definition Register(MDR).
** These are used in the functions: 'UARTConfigSetExpClk',
** 'UARTConfigGetExpClk'
*/

/* This is used to identify the Over-sampling rate being set.*/
#define UART_OVER_SAMP_RATE             UART_MDR_OSM_SEL                            

/* This is used to select Over-sampling rate as 16. */
#define UART_OVER_SAMP_RATE_16          UART_MDR_OSM_SEL_SHIFT

/* This is used to select Over-sampling rate as 13.*/
#define UART_OVER_SAMP_RATE_13          UART_MDR_OSM_SEL

/*******************************************************************************/


//**********************************************************************
//  API FUNCTION PROTOTYPES
//**********************************************************************/

void UARTConfigSetExpClk (unsigned int baseAdd, unsigned int uartClk,
                          unsigned int baudrate, unsigned int config, 
                          unsigned int overSampRate); 
void UARTConfigGetExpClk (unsigned int baseAdd, unsigned int uartClk,
                          unsigned int *pBaud, unsigned int *pConfig);
void UARTFIFOLevelSet (unsigned int baseAdd, unsigned int rxLevel);
void UARTParityModeSet(unsigned int baseAdd, unsigned int parityMode);
unsigned int UARTParityModeGet(unsigned int baseAdd);                                     
void UARTEnable (unsigned int baseAdd);
void UARTDisable (unsigned int baseAdd);
void UARTFIFOEnable(unsigned int baseAdd);
void UARTFIFODisable(unsigned int baseAdd);
unsigned int UARTCharsAvail(unsigned int baseAdd);                                           

unsigned int UARTSpaceAvail(unsigned int baseAdd);                                           
unsigned int UARTCharPutNonBlocking(unsigned int baseAdd, 
                                    unsigned char byteWrite);

int UARTCharGetNonBlocking(unsigned int baseAdd);
void UARTIntEnable( unsigned int baseAdd, unsigned int intFlags);
void UARTIntDisable(unsigned int baseAdd, unsigned int intFlags);
unsigned int UARTIntStatus(unsigned int baseAdd);
int UARTCharGet(unsigned int baseAdd);
void UARTCharPut(unsigned int baseAdd, unsigned char byteTx);
void UARTBreakCtl(unsigned int baseAdd, unsigned int breakState);
void UARTModemControlSet(unsigned int baseAdd, unsigned int ctrlFlags);

void UARTModemControlClear(unsigned int baseAdd, unsigned int ctrlFlags);
unsigned int UARTModemControlGet(unsigned int baseAdd);
unsigned int UARTModemStatusGet(unsigned int baseAdd);
unsigned int UARTRxErrorGet(unsigned int baseAdd);
void UARTDMAEnable (unsigned int baseAdd, unsigned int flags);
void UARTDMADisable (unsigned int baseAdd, unsigned int flags);


#ifdef __cplusplus
}
#endif
#endif

