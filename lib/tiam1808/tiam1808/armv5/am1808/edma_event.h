/**
 *  \file   edma_event.h
 *
 *  \brief  EDMA event enumeration
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

#ifndef _EDMAEVENT_H
#define _EDMAEVENT_H

#include "hw_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/****************************************************************************
**                       MACRO DEFINITIONS
****************************************************************************/
/*
**EDMA Event number list
*/

/* McASP0 Receive Event */
#define EDMA3_CHA_MCASP0_RX             0
/* McASP0 Transmit Event*/
#define EDMA3_CHA_MCASP0_TX             1
/* McBSP0 Receive Event */
#define EDMA3_CHA_MCBSP0_RX             2
/* McBSP1 Transmit Event*/
#define EDMA3_CHA_MCBSP0_TX             3
/* McBSP1 Receive Event */
#define EDMA3_CHA_MCBSP1_RX             4
/* McBSP1 Transmit Event*/
#define EDMA3_CHA_MCBSP1_TX             5

/* GPIO Bank0 event*/
#define EDMA3_CHA_GPIO_BNKINT0          6
/* GPIO Bank1 event*/
#define EDMA3_CHA_GPIO_BNKINT1          7
/* GPIO Bank2 event*/
#define EDMA3_CHA_GPIO_BNKINT2          22
/* GPIO Bank3 event*/
#define EDMA3_CHA_GPIO_BNKINT3          23
/* GPIO Bank4 event*/
#define EDMA3_CHA_GPIO_BNKINT4          28
/* GPIO Bank5 event*/
#define EDMA3_CHA_GPIO_BNKINT5          29
/* GPIO Bank6 event(TPCC1)*/
#define EDMA3_CHA_GPIO_BNKINT6          16
/* GPIO Bank7 event(TPCC1)*/
#define EDMA3_CHA_GPIO_BNKINT7          17
/* GPIO Bank8 event(TPCC1)*/
#define EDMA3_CHA_GPIO_BNKINT8          18

/* UART0 Receive Event */
#define EDMA3_CHA_UART0_RX              8
/* UART0 Transmit Event */
#define EDMA3_CHA_UART0_TX              9
/* UART1 Receive Event */
#define EDMA3_CHA_UART1_RX              12
/* UART1 Transmit Event */
#define EDMA3_CHA_UART1_TX              13
/* UART2 Receive Event */
#define EDMA3_CHA_UART2_RX              30
/* UART2 Transmit Event */
#define EDMA3_CHA_UART2_TX              31

/* Timer 64P0 Event Out 12 */
#define EDMA3_CHA_TIMER64P0_EVT12       10
/* Timer 64P0 Event Out 34 */
#define EDMA3_CHA_TIMER64P0_EVT34       11

/* Timer 64P2 Event Out 12 (TPCC1) */
#define EDMA3_CHA_TIMER64P2_EVT12       24
/* Timer 64P2 Event Out 34 (TPCC1) */
#define EDMA3_CHA_TIMER64P2_EVT34       25

/* Timer 64P3 Event Out 12 (TPCC1) */
#define EDMA3_CHA_TIMER64P3_EVT12       26
/* Timer 64P3 Event Out 34 (TPCC1) */
#define EDMA3_CHA_TIMER64P3_EVT34       27

/* SPI0 Receive Event */
#define EDMA3_CHA_SPI0_RX               14
/* SPI0 Transmit Event */
#define EDMA3_CHA_SPI0_TX               15
/* SPI1 Receive Event */
#define EDMA3_CHA_SPI1_RX               18
/* SPI1 Transmit Event */
#define EDMA3_CHA_SPI1_TX               19

/* MMCSD0 Receive Event */
#define EDMA3_CHA_MMCSD0_RX             16
/* MMCSD0 Transmit Event */
#define EDMA3_CHA_MMCSD0_TX             17

/* MMCSD1 Receive Event    (TPCC1) */
#define EDMA3_CHA_MMCSD1_RX             28
/* MMCSD1 Transmit Event   (TPCC1) */
#define EDMA3_CHA_MMCSD1_TX             29

/* I2C0 Receive Event */
#define EDMA3_CHA_I2C0_RX               24
/* I2C0 Transmit Event */
#define EDMA3_CHA_I2C0_TX               25
/* I2C1 Receive Event */
#define EDMA3_CHA_I2C1_RX               26
/* I2C1 Transmit Event */
#define EDMA3_CHA_I2C1_TX               27

/* Timer 2 compare event0  (TPCC1) */
#define EDMA3_TIMER2_T12CMPEVT0         0
/* Timer 2 compare event1  (TPCC1) */
#define EDMA3_TIMER2_T12CMPEVT1         1
/* Timer 2 compare event2  (TPCC1) */
#define EDMA3_TIMER2_T12CMPEVT2         2
/* Timer 2 compare event3  (TPCC1) */
#define EDMA3_TIMER2_T12CMPEVT3         3
/* Timer 2 compare event4  (TPCC1) */
#define EDMA3_TIMER2_T12CMPEVT4         4
/* Timer 2 compare event5  (TPCC1) */
#define EDMA3_TIMER2_T12CMPEVT5         5
/* Timer 2 compare event6  (TPCC1) */
#define EDMA3_TIMER2_T12CMPEVT6         6
/* Timer 2 compare event7  (TPCC1) */
#define EDMA3_TIMER2_T12CMPEVT7         7


/* Timer 3 compare event0  (TPCC1) */
#define EDMA3_TIMER3_T12CMPEVT0         8
/* Timer 3 compare event1  (TPCC1) */
#define EDMA3_TIMER3_T12CMPEVT1         9
/* Timer 3 compare event2  (TPCC1) */
#define EDMA3_TIMER3_T12CMPEVT2         10
/* Timer 3 compare event3  (TPCC1) */
#define EDMA3_TIMER3_T12CMPEVT3         11
/* Timer 3 compare event4  (TPCC1) */
#define EDMA3_TIMER3_T12CMPEVT4         12
/* Timer 3 compare event5  (TPCC1) */
#define EDMA3_TIMER3_T12CMPEVT5         13
/* Timer 3 compare event6  (TPCC1) */
#define EDMA3_TIMER3_T12CMPEVT6         14
/* Timer 3 compare event7  (TPCC1) */
#define EDMA3_TIMER3_T12CMPEVT7         15

/* PRU Subsystem */
#define EDMA3_PRU_EVTOUT6		    20
#define EDMA3_PRU_EVTOUT7		    21

#ifdef __cplusplus
}
#endif

#endif
