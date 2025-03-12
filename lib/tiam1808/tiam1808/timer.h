/**
 *  \file   timer.h
 *
 *  \brief  Timer APIs and macros.
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

#ifndef __TIMER_H__
#define __TIMER_H__

#include "hw_tmr.h"

#ifdef __cplusplus
extern "C" {
#endif
/*****************************************************************************/
/*
** Values that can be passed to Timer APIs as timer
*/
#define TMR_TIMER12                        (0x00003FFEu) /* Timer12 */
#define TMR_TIMER34                        (0x3FFE0000u) /* Timer34 */
#define TMR_TIMER_BOTH                     (0x3FFE3FFEu) /* Both Timers */
/*****************************************************************************/
/*
** Values that can be passed to TimerEnable API as enaMode to select the mode
*/
/* Enable the Timer for once */
#define TMR_ENABLE_ONCE                    (0x00400040u)

/* Enable the Timer for continuous operation without period reload */
#define TMR_ENABLE_CONT                    (0x00800080u)

/* Enable the Timer for continuous operation with period reload */
#define TMR_ENABLE_CONTRELOAD              (0x00C000C0u)

/*****************************************************************************/
/*
** Values that can be passed to TimerConfigure API as config
*/
/* 64 Bit mode with internal clock source */
#define TMR_CFG_64BIT_CLK_INT              (0x00000013u)

/* 64 Bit mode with external clock source */
#define TMR_CFG_64BIT_CLK_EXT              (0x01000113u)

/* 64 Bit Watchdog timer mode */
#define TMR_CFG_64BIT_WATCHDOG             (0x0000001Bu)

/* 32 Bit chained mode with internal clock source */
#define TMR_CFG_32BIT_CH_CLK_INT           (0x0000001Fu)

/* 32 Bit chained mode with external clock source */
#define TMR_CFG_32BIT_CH_CLK_EXT           (0x0100011Fu)

/* 32 Bit unchained mode with internal clock source for both timers */
#define TMR_CFG_32BIT_UNCH_CLK_BOTH_INT    (0x00000017u)

/* 32 Bit unchained mode; clock Internal for Timer12; external for Timer34 */
#define TMR_CFG_32BIT_UNCH_CLK_12INT_34EXT (0x01000017u)

/* 32 Bit unchained mode; clock external for Timer12; internal for Timer34 */
#define TMR_CFG_32BIT_UNCH_CLK_12EXT_34INT (0x00000117u)

/* 32 Bit unchained mode with external clock source for both timers */
#define TMR_CFG_32BIT_UNCH_CLK_BOTH_EXT    (0x01000117u)
/*****************************************************************************/
/*
** Values that can be passed to TimerIntEnable/IntDisable APIs as intFlag
** Any combination is also allowed.
** Example- (TMR_INTCTLSTAT_PRDINTEN12 | TMR_INTCTLSTAT_EVTINTEN34)
*/
/* To enable/disable the interrupt for Timer12 in Capture mode */
#define TMR_INT_TMR12_CAPT_MODE            (TMR_INTCTLSTAT_EVTINTEN12)

/* To enable/disable the interrupt for Timer12 in in 64 Bit/32 Bit/WDT mode */
#define TMR_INT_TMR12_NON_CAPT_MODE        (TMR_INTCTLSTAT_PRDINTEN12)

/* To enable/disable the interrupt for Timer34 in Capture mode */
#define TMR_INT_TMR34_CAPT_MODE            (TMR_INTCTLSTAT_EVTINTEN34)

/* To enable/disable the interrupt for Timer34 in 64 Bit/32 Bit/WDT mode */
#define TMR_INT_TMR34_NON_CAPT_MODE        (TMR_INTCTLSTAT_PRDINTEN34)

/*****************************************************************************/
/*
** Values that can be passed to TimerIntStatusGet/Clear as statFlag
** Any combination is also allowed.
*/
/* Interrupt Generated when Timer 12 in 64 Bit/32 Bit/WDT mode */
#define TMR_INTSTAT12_TIMER_NON_CAPT       (TMR_INTCTLSTAT_PRDINTSTAT12)

/* Interrupt Generated when Timer 12 in capture mode */
#define TMR_INTSTAT12_TIMER_CAPT           (TMR_INTCTLSTAT_EVTINTSTAT12)

/* Interrupt Generated when Timer 34 in 64 Bit/32 Bit/WDT mode */
#define TMR_INTSTAT34_TIMER_NON_CAPT       (TMR_INTCTLSTAT_PRDINTSTAT34)

/* Interrupt Generated when Timer 34 in capture mode */
#define TMR_INTSTAT34_TIMER_CAPT           (TMR_INTCTLSTAT_EVTINTSTAT12)

/*****************************************************************************/
/*
** Values that can be passed to TimerCaptureConfig as cfgCap
*/
/* Disable Capture Mode */
#define TMR_CAPT_DISABLE                   (0x00000000)

/* Enable Capture Mode. Event Occurs in rising edge */
#define TMR_CAPT_ENABLE_RIS_EDGE           (0x08000800)

/* Enable Capture Mode. Event Occurs in falling edge */
#define TMR_CAPT_ENABLE_FALL_EDGE          (0x18001800)

/* Enable Capture Mode. Event Occurs in both edges */
#define TMR_CAPT_ENABLE_BOTH_EDGE          (0x28002800)

/*****************************************************************************/
/*
** Values that can be passed to TimerPulseWidthSet as pulseWidth
*/
#define TMR_PULSE_WIDTH_1_CLK              (0x00000000) /* 1 clock cycle */
#define TMR_PULSE_WIDTH_2_CLK              (0x00100010) /* 2 clock cycles */
#define TMR_PULSE_WIDTH_3_CLK              (0x00200020) /* 3 clock cycles */
#define TMR_PULSE_WIDTH_4_CLK              (0x00300030) /* 4 clock cycles */

/*****************************************************************************/
/*
** Values returned by TimerOUTStatusGet API
*/
#define TMR_OUT12_ASSERTED                 (TMR_TCR_TSTAT12) /* TMR64P_OUT12 */
#define TMR_OUT34_ASSERTED                 (TMR_TCR_TSTAT34) /* TMR64P_OUT34 */

/*****************************************************************************/
/*
** Prototypes for the APIs
*/
extern unsigned int TimerCompareGet(unsigned int baseAddr, 
                                     unsigned int regIndex);
extern unsigned int TimerOUTStatusGet(unsigned int baseAddr, 
                                       unsigned int timer);
extern unsigned int TimerIntStatusGet(unsigned int baseAddr, 
                                    unsigned int statFlag);
extern unsigned int TimerIntStatusClear(unsigned int baseAddr, 
                                    unsigned int statFlag);
extern unsigned int TimerCaptureGet(unsigned int baseAddr,unsigned int timer);
extern unsigned int TimerCounterGet(unsigned int baseAddr, unsigned int timer);
extern unsigned int TimerPeriodGet(unsigned int baseAddr, unsigned int timer);
extern unsigned int TimerReloadGet(unsigned int baseAddr, unsigned int timer);
extern unsigned int TimerPreScalarCount34Get(unsigned int baseAddr);
extern unsigned int TimerDivDwnRatio34Get(unsigned int baseAddr);

extern void TimerCounterSet(unsigned int baseAddr, unsigned int timer, 
                            unsigned int counter);
extern void TimerPeriodSet(unsigned int baseAddr, unsigned int timer, 
                           unsigned int period);
extern void TimerReloadSet(unsigned int baseAddr, unsigned int timer, 
                           unsigned int reload);
extern void TimerEnable(unsigned int baseAddr, unsigned int timer, 
                                                unsigned int enaMode);
extern void TimerPreScalarCount34Set(unsigned int baseAddr, 
                                     unsigned int psc34);
extern void TimerDivDwnRatio34Set(unsigned int baseAddr, unsigned int tddr34);
extern void TimerInvertOUTDisable(unsigned int baseAddr, unsigned int timer);
extern void TimerReadResetDisable(unsigned int baseAddr, unsigned int timer);
extern void TimerInputGateDisable(unsigned int baseAddr, unsigned int timer);
extern void TimerReadResetEnable(unsigned int baseAddr, unsigned int timer);
extern void TimerInvertINDisable(unsigned int baseAddr, unsigned int timer);
extern void TimerInvertOUTEnable(unsigned int baseAddr, unsigned int timer);
extern void TimerInputGateEnable(unsigned int baseAddr, unsigned int timer);
extern void TimerInvertINEnable(unsigned int baseAddr, unsigned int timer);
extern void TimerIntDisable(unsigned int baseAddr, unsigned int intFlags);
extern void TimerClockModeSet(unsigned int baseAddr, unsigned int timer);
extern void TimerPulseModeSet(unsigned int baseAddr, unsigned int timer);
extern void TimerIntEnable(unsigned int baseAddr, unsigned int intFlags);
extern void TimerConfigure(unsigned int baseAddr, unsigned int config);
extern void TimerDisable(unsigned int baseAddr, unsigned int timer);
extern void TimerWatchdogReactivate(unsigned int baseAddr);
extern void TimerWatchdogActivate(unsigned int baseAddr);
extern void TimerCaptureConfigure(unsigned int baseAddr, unsigned int timer,
                                  unsigned int cfgCap);
extern void TimerCompareSet(unsigned int baseAddr, unsigned int regIndex,
                            unsigned int compare);
extern void TimerPulseWidthSet(unsigned int baseAddr, unsigned int timer,
                               unsigned int pulseWidth);

#ifdef __cplusplus
}
#endif
#endif /* __TIMER_H__ */
