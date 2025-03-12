/**
 *  \file   interrupt.h
 *
 *  \brief  Interrupt related API declarations.
 *
 *   This file contains the API prototypes for configuring AINTC
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

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "hw_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/****************************************************************************
**                       MACRO DEFINITIONS
****************************************************************************/
/*
**Interrupt number list
*/

#define SYS_INT_COMMTX			0
#define SYS_INT_COMMRX			1
#define SYS_INT_NINT			2
#define SYS_INT_EVTOUT0			3
#define SYS_INT_EVTOUT1			4
#define SYS_INT_EVTOUT2			5
#define SYS_INT_EVTOUT3			6
#define SYS_INT_EVTOUT4			7
#define SYS_INT_EVTOUT5			8
#define SYS_INT_EVTOUT6			9
#define SYS_INT_EVTOUT7			10
#define SYS_INT_CCINT0			11
#define SYS_INT_CCERRINT		12
#define SYS_INT_TCERRINT0		13
#define SYS_INT_AEMIFINT		14
#define SYS_INT_I2CINT0			15
#define SYS_INT_MMCSDINT0		16
#define SYS_INT_MMCSDINT1		17
#define SYS_INT_PSCINT0			18
#define SYS_INT_RTC			19
#define SYS_INT_SPINT0			20
#define SYS_INT_TINT12_0		21
#define SYS_INT_TINT34_0		22
#define SYS_INT_TINT12_1		23
#define SYS_INT_TINT34_1		24
#define SYS_INT_UARTINT0		25
#define SYS_INT_PROTERR  		26
#define SYS_INT_SYSCFG_CHIPINT0	28
#define SYS_INT_SYSCFG_CHIPINT1	29
#define SYS_INT_SYSCFG_CHIPINT2	30
#define SYS_INT_SYSCFG_CHIPINT3	31
#define SYS_INT_TCERRINT1		32
#define SYS_INT_C0_RXTHRESH		33
#define SYS_INT_C0_RX			34
#define SYS_INT_C0_TX			35
#define SYS_INT_C0_MISC			36
#define SYS_INT_C1_RXTHRESH		37
#define SYS_INT_C1_RX			38
#define SYS_INT_C1_TX			39
#define SYS_INT_C1_MISC			40
#define SYS_INT_DDR2MEMERR		41
#define SYS_INT_GPIOB0			42
#define SYS_INT_GPIOB1			43
#define SYS_INT_GPIOB2			44
#define SYS_INT_GPIOB3			45
#define SYS_INT_GPIOB4			46
#define SYS_INT_GPIOB5			47
#define SYS_INT_GPIOB6			48
#define SYS_INT_GPIOB7			49
#define SYS_INT_GPIOB8			50
#define SYS_INT_I2CINT1			51
#define SYS_INT_LCDINT			52
#define SYS_INT_UARTINT1		53
#define SYS_INT_MCASPINT		54
#define SYS_INT_PSCINT1			55
#define SYS_INT_SPINT1			56
#define SYS_INT_UHPI_INT1		57
#define SYS_INT_USB0 			58
#define SYS_INT_USB1HC			59
#define SYS_INT_USB1RWAKE		60
#define SYS_INT_UARTINT2		61
#define SYS_INT_EHRPWM0			63
#define SYS_INT_EHRPWM0TZ		64
#define SYS_INT_EHRPWM1			65
#define SYS_INT_EHRPWM1TZ		66
#define SYS_INT_SATA			67
#define SYS_INT_TIMR2_ALL		68
#define SYS_INT_ECAP0			69
#define SYS_INT_ECAP1			70
#define SYS_INT_ECAP2			71
#define SYS_INT_MMCSD			72
#define SYS_INT_SDIO			73
#define SYS_INT_TMR1_CMPINT0		74
#define SYS_INT_TMR1_CMPINT1		75
#define SYS_INT_TMR1_CMPINT2		76
#define SYS_INT_TMR1_CMPINT3		77
#define SYS_INT_TMR1_CMPINT4		78
#define SYS_INT_TMR1_CMPINT5		79
#define SYS_INT_TMR1_CMPINT6		80
#define SYS_INT_TMR1_CMPINT7		81
#define SYS_INT_TMR2_CMPINT0		82
#define SYS_INT_TMR2_CMPINT1		83
#define SYS_INT_TMR2_CMPINT2		84
#define SYS_INT_TMR2_CMPINT3		85
#define SYS_INT_TMR2_CMPINT4		86
#define SYS_INT_TMR2_CMPINT5		87
#define SYS_INT_TMR2_CMPINT6		88
#define SYS_INT_TMR2_CMPINT7		89
#define SYS_INT_ARMCLKSTOPREQ		90
#define SYS_INT_VPIF				92


/*
** The maximum number of interrupts supported by AM18XX
*/
#define NUM_INTERRUPTS           101


/*****************************************************************************
**                     API FUNCTION PROTOTYPES
*****************************************************************************/
extern void IntIRQEnable(void);
extern void IntIRQDisable(void);
extern void IntFIQEnable(void);
extern void IntFIQDisable(void);
extern void IntAINTCInit (void);
extern void IntGlobalEnable(void);
extern void IntGlobalDisable(void);
extern void IntMasterIRQEnable(void);
extern void IntMasterIRQDisable(void);
extern void IntMasterFIQEnable(void);
extern void IntMasterFIQDisable(void);
extern void IntSystemEnable(unsigned int intrNum);
extern void IntSystemDisable(unsigned int intrNum);
extern void IntSystemStatusClear(unsigned int intrNum);
extern unsigned char IntChannelGet(unsigned int intrNum);
extern unsigned int IntSystemStatusRawGet(unsigned int intrNum);
extern unsigned int IntSystemStatusEnabledGet(unsigned int intrNum);
extern unsigned int IntMasterStatusGet(void);
extern void IntUnRegister(unsigned int intrNum);
extern void IntChannelSet(unsigned int intrNum, unsigned char channel);
extern void IntRegister(unsigned int intrNum, void (*pfnHandler)(void));
extern unsigned char IntDisable(void);
extern void IntEnable(unsigned char  status);

#ifdef __cplusplus
}
#endif
#endif

