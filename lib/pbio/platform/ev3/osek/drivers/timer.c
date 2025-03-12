/**
 *  \file   timer.c
 *
 *  \brief  TIMER APIs.
 *
 *   This file contains the device abstraction layer APIs for Timer Plus.
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

/* HW Macros and Peripheral Defines */
#include "hw_types.h"
#include "hw_tmr.h"

/* Driver APIs */
#include "timer.h"

/*******************************************************************************
*                       INTERNAL MACRO DEFINITIONS
*******************************************************************************/
#define WDT_KEY_PRE_ACTIVE                    (0xA5C6u)
#define WDT_KEY_ACTIVE                        (0xDA7Eu)
#define CMP_IDX_MASK                          (0x07)
#define PRESCALE_MASK                         (0x0F)
#define TDDR_MASK                             (0x0F)

/*******************************************************************************
*                        API FUNCTION DEFINITIONS
*******************************************************************************/

/**
 * \brief   Enables the timer in the specified mode. The timer must be 
 *          configured before it is enabled. The timer starts running when this
 *          API is called
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer to be enabled.
 * \param   enaMode       Mode of enabling the timer.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer 12 only \n
 *     TMR_TIMER_BOTH - Both timers \n
 *
 * enaMode can take the values \n
 *     TMR_ENABLE_ONCE - Enable the timer to run once \n
 *     TMR_ENABLR_CONT - Enable to run continuous \n
 *     TMR_ENABLE_CONTRELOAD - Enable to run continuous with period reload
 *
 * \return  None.
 *
 **/
void TimerEnable(unsigned int baseAddr, unsigned int timer, 
                 unsigned int enaMode)
{
    /* Clear the enable bits of both timers; the timers stops couting */
    HWREG(baseAddr + TMR_TCR) &= ~((timer & (TMR_TCR_ENAMODE12 |
                                             TMR_TCR_ENAMODE34)));

    /* Enable the timer */
    HWREG(baseAddr + TMR_TCR) |= (enaMode & timer);
}

/**
 * \brief   Disables the timer. The timer stops running when this API is called
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         Timer to be disabled.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers \n
 *
 * \return  None.
 *
 **/
void TimerDisable(unsigned int baseAddr, unsigned int timer)
{
    /* Disable the timer; the Timer stops couting */
    HWREG(baseAddr + TMR_TCR) &= ~((timer & (TMR_TCR_ENAMODE12 |
                                             TMR_TCR_ENAMODE34)));
}

/**
 * \brief   Configures the timer. The timer can be configured in 64 bit mode
 *          32 bit chained/unchained mode, or as a watchdog timer. The timer
 *          can be given external clock input or internal clock input. When
 *          this API is called,\n  
 *          > The Timer counters are cleared \n
 *          > Both the timers are disabled from Reset. Hence, both the timers
 *            will start counting when enabled. \n
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   config        Configuration of the Timer Module.
 *
 * config can take the values \n 
 *     TMR_CFG_64BIT_CLK_INT - 64 bit mode with internal clock \n
 *     TMR_CFG_64BIT_CLK_EXT - 64 bit mode with external clock \n
 *     TMR_CFG_64BIT_WATCHDOG - 64 bit watchdog timer mode \n
 *     TMR_CFG_32BIT_CH_CLK_INT - 32 bit chained mode with internal clock \n
 *     TMR_CFG_32BIT_CH_CLK_EXT - 32 bit chained mode with external clock \n
 *     TMR_CFG_32BIT_UNCH_CLK_BOTH_INT - 32 bit unchained mode; Both timers 
 *                                       clock sources are internal \n
 *     TMR_CFG_32BIT_UNCH_CLK_12INT_34EXT - 32 bit unchained mode; Clock source
 *                       for Timer12 is internal and for Timer34 is external \n 
 *     TMR_CFG_32BIT_UNCH_CLK_12EXT_34INT - 32 bit unchained mode; Clock source
 *                       for Timer12 is external and for Timer34 is internal \n
 *     TMR_CFG_32BIT_UNCH_CLK_BOTH_EXT - 32 bit unchained  mode; Both timers 
 *                                       clock sources are external
 *
 * \return  None.
 *
 **/
void TimerConfigure(unsigned int baseAddr, unsigned int config)
{

    /*
    ** Set the timer control register. This will only affect the clock
    ** selection bits. All other fields will be reset and the timer counting
    ** will be disabled.
    */
    HWREG(baseAddr + TMR_TCR) = (config & (TMR_TCR_CLKSRC12 | TMR_TCR_CLKSRC34));

    /* Clear the Timer Counters */
    HWREG(baseAddr + TMR_TIM12) = 0x0;
    HWREG(baseAddr + TMR_TIM34) = 0x0;

    /* Clear the TIMMODE bits and Reset bits */
    HWREG(baseAddr + TMR_TGCR) &= ~( TMR_TGCR_TIMMODE | TMR_TGCR_TIM34RS |
                                     TMR_TGCR_TIM12RS);

    /*
    ** Select the timer mode and disable the timer module from Reset
    ** Timer Plus features are enabled.
    */
    HWREG(baseAddr + TMR_TGCR) |= (config & 
                                   (TMR_TGCR_TIMMODE | TMR_TGCR_TIM34RS |
                                    TMR_TGCR_TIM12RS | TMR_TGCR_PLUSEN));
}

/**
 * \brief   Activate the Watchdog timer. The timer shall be configured as
 *          watchdog timer before this API is called.  This API writes two keys
 *          into the WDTCR in the order to activate the WDT. The user shall call
 *          TimerWatchdogReactivate API before the WDT expires, to avoid a reset.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 *
 * \return  None.
 *
 **/
void TimerWatchdogActivate(unsigned int baseAddr)
{
    /* Enable the watchdog timer. Write the keys in the order */
    HWREG(baseAddr + TMR_WDTCR) = ((TMR_WDTCR_WDEN | TMR_WDTCR_WDFLAG) |
                                  (WDT_KEY_PRE_ACTIVE << TMR_WDTCR_WDKEY_SHIFT));
    HWREG(baseAddr + TMR_WDTCR) = ((HWREG(baseAddr + TMR_WDTCR) &
                                    (~TMR_WDTCR_WDKEY)) |
                                   (WDT_KEY_ACTIVE << TMR_WDTCR_WDKEY_SHIFT));
                                        
}

/**
 * \brief   Re-activate the Watchdog timer. The WDT shall be enabled before
 *          this API is called. The user shall call this API before the WDT
 *          expires, to avoid a reset.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 *
 * \return  None.
 *
 **/
void TimerWatchdogReactivate(unsigned int baseAddr)
{
    /* Write the first keys in the order. This order shall not change */
    HWREG(baseAddr + TMR_WDTCR) = ((HWREG(baseAddr + TMR_WDTCR) &
                                    (~TMR_WDTCR_WDKEY)) |
                                   (WDT_KEY_PRE_ACTIVE << TMR_WDTCR_WDKEY_SHIFT));
    HWREG(baseAddr + TMR_WDTCR) = ((HWREG(baseAddr + TMR_WDTCR) &
                                    (~TMR_WDTCR_WDKEY)) |
                                   (WDT_KEY_ACTIVE << TMR_WDTCR_WDKEY_SHIFT));
 
}

/**
 * \brief   Set the Period register(s) of the specified timer(s).
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which period to be set.
 * \param   period        The period value of the timer.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers 
 *
 * \return  None.
 *
 **/
void TimerPeriodSet(unsigned int baseAddr, unsigned int timer, 
                    unsigned int period)
{
    if(TMR_TIMER12 & timer)
    {
        /* Write the period for Timer12 */
        HWREG(baseAddr + TMR_PRD12) = period;
    }

    if(TMR_TIMER34 & timer)
    {
        /* Write the period for Timer34 */
        HWREG(baseAddr + TMR_PRD34) = period;
    }
}

/**
 * \brief   Returns the Period register contents of the specified timer.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which period to be read.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 \n
 *     TMR_TIMER12 - Timer12 
 *
 * \return  Period Value
 *
 **/
unsigned int TimerPeriodGet(unsigned int baseAddr, unsigned int timer)
{
    /* Return the appropriate period */
    return((timer == TMR_TIMER12) ? HWREG(baseAddr + TMR_PRD12) :
                                    HWREG(baseAddr + TMR_PRD34));
}

/**
 * \brief   Set the Counter register of the specified timer.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which counter to be set.
 * \param   counter       The counter value of the timer.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers \n
 *
 * \return  None.
 *
 **/
void TimerCounterSet(unsigned int baseAddr, unsigned int timer, 
                     unsigned int counter)
{
    if(TMR_TIMER12 & timer)
    {
        /* Write the counter for Timer12 */
        HWREG(baseAddr + TMR_TIM12) = counter;
    }

    if(TMR_TIMER34 & timer)
    {
        /* Write the counter for Timer34 */
        HWREG(baseAddr + TMR_TIM34) = counter;
    }
}

/**
 * \brief   Returns the Counter register contents of the specified timer.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which counter to be read.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 \n
 *     TMR_TIMER12 - Timer12 \n
 *
 * \return  Counter Value.
 *
 **/
unsigned int TimerCounterGet(unsigned int baseAddr, unsigned int timer)
{
    /* Return the appropriate period */
    return((timer == TMR_TIMER12) ? HWREG(baseAddr + TMR_TIM12) :
                                    HWREG(baseAddr + TMR_TIM34));
}


/**
 * \brief   Set the Reload period of the specified timer.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which reload period to be set.
 * \param   reload        The reload period value of the timer.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers
 *
 * \return  None.
 *
 **/
void TimerReloadSet(unsigned int baseAddr, unsigned int timer, 
                    unsigned int reload)
{
    if(TMR_TIMER12 & timer)
    {
        /* Write the reload value for Timer12 */
        HWREG(baseAddr + TMR_REL12) = reload;
    }

    if(TMR_TIMER34 & timer)
    {
        /* Write the reload value for Timer34 */
        HWREG(baseAddr + TMR_REL34) = reload;
    }
}

/**
 * \brief   Returns the Reload Period of the specified timer.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which Reload value to be read.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 \n
 *     TMR_TIMER12 - Timer12 \n
 *
 * \return  Reload Value
 *
 **/
unsigned int TimerReloadGet(unsigned int baseAddr, unsigned int timer)
{
    /* Return the appropriate reload value */
    return((TMR_TIMER12 == timer) ? HWREG(baseAddr + TMR_REL12) :
                                    HWREG(baseAddr + TMR_REL34));
}

/**
 * \brief   Returns the capture value of the specified timer.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which capture value to be read.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 \n
 *     TMR_TIMER12 - Timer12 \n
 *
 * \return  Capture Value
 *
 **/
unsigned int TimerCaptureGet(unsigned int baseAddr, unsigned int timer)
{
    /* Return the appropriate value */
    return((TMR_TIMER12 == timer) ? HWREG(baseAddr + TMR_CAP12) :
                                    HWREG(baseAddr + TMR_CAP34));
}

/**
 * \brief   Set the Compare value of the specified CMP register.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   regIndex      Index of the CMP Register
 * \param   compare       The value to be written
 *
 * regIndex can take any value from 0 to 7.
 *                   If regIndex = n, CMPn will be set with the value compare.
 *
 * \return  None.
 *
 **/
void TimerCompareSet(unsigned int baseAddr, unsigned int regIndex, 
                     unsigned int compare)
{
    /* Write only to the desired Compare register according to the index */
    HWREG(baseAddr + TMR_CMP(regIndex)) = compare;
}


/**
 * \brief   Returns the Compare value of the specified CMP register.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   regIndex      Index of the CMP Register
 *
 * regIndex can take any value from 0 to 7.
 *                         If regIndex = n, contents of CMPn will be returned.
 *
 * \return  Compare Value.
 *
 **/
unsigned int TimerCompareGet(unsigned int baseAddr, unsigned int regIndex)
{
    /* Return the counter value according to the index requested */
    return(HWREG(baseAddr + TMR_CMP(regIndex)));
}

/**
 * \brief   Enables the specified timer interrupts. The timer interrupts which
 *          are to be enabled can be passed as parameter to this function.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   intFlags      Timer Interrupts to be enabled
 *
 * intFlags can take any, or a combination of the following values \n
 *    TMR_INT_TMR12_CAPT_MODE - Enable Timer12 interrupt in Capture Mode \n
 *    TMR_INT_TMR12_NON_CAPT_MODE - Enable Timer12 interrupt in normal mode \n
 *    TMR_INT_TMR34_CAPT_MODE - Enable Timer34 interrupt in Capture mode \n
 *    TMR_INT_TMR34_NON_CAPT_MODE - Enable Timer34 interrupt in normal mode \n
 *
 * \return  None.
 *
 **/
void TimerIntEnable(unsigned int baseAddr, unsigned int intFlags)
{
    /* Enable the mentioned interrupts. One or more interrupts are enabled */
    HWREG(baseAddr + TMR_INTCTLSTAT) |= intFlags;
}

/**
 * \brief   Disables the specified timer interrupts.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   intFlags      Timer Interrupts to be disabled
 *
 * intFlags can take any, or a combination of the following values \n
 *    TMR_INT_TMR12_CAPT_MODE - Disable Timer12 interrupt in Capture Mode \n
 *    TMR_INT_TMR12_NON_CAPT_MODE - Disable Timer12 interrupt in normal mode \n
 *    TMR_INT_TMR34_CAPT_MODE - Disable Timer34 interrupt in Capture mode \n
 *    TMR_INT_TMR34_NON_CAPT_MODE - Disable Timer34 interrupt in normal mode \n
 *
 * \return  None.
 *
 **/
void TimerIntDisable(unsigned int baseAddr, unsigned int intFlags)
{
    /* Disable the mentioned interrupts */
    HWREG(baseAddr + TMR_INTCTLSTAT) &= ~(intFlags);
}

/**
 * \brief   Returns the status of specified timer interrupts.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   statFlag      Status flags to be read.
 *
 * intFlags can take any or combination of the following values \n
 *    TMR_INTSTAT12_TIMER_NON_CAPT - Timer12 interrupt status in normal mode \n
 *    TMR_INTSTAT12_TIMER_CAPT - Timer12 interrupt status in capture mode \n
 *    TMR_INTSTAT34_TIMER_NON_CAPT - Timer34 interrupt status in normal mode \n
 *    TMR_INTSTAT34_TIMER_CAPT - Timer34 interrupt status in capture mode \n
 *
 * \return  Status of Interrupt. Returns all the fields of which status is set
 *
 * Note : This API will return the same fields which is passed as parameter, if
 * all the specified interrupt status is set. The return value will be 0 if 
 * none of the interrupt status in the parameter passed is set. 
 *     
 **/
unsigned int TimerIntStatusGet(unsigned int baseAddr, unsigned int statFlag)
{
    /* Return statuses of the interrupts. There is no shifting applied here */
    return((HWREG(baseAddr + TMR_INTCTLSTAT)) & statFlag);

}

/**
 * \brief   Clears the status of specified timer interrupts.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   statFlag      Status flags to be cleared.
 *
 * intFlags can take any or combination of the following values \n
 *    TMR_INTSTAT12_TIMER_NON_CAPT - Timer12 interrupt status in normal mode \n
 *    TMR_INTSTAT12_TIMER_CAPT - Timer12 interrupt status in capture mode \n
 *    TMR_INTSTAT34_TIMER_NON_CAPT - Timer34 interrupt status in normal mode \n
 *    TMR_INTSTAT34_TIMER_CAPT - Timer34 interrupt status in capture mode \n
 *
 * \return  None
 *     
 **/
unsigned int TimerIntStatusClear(unsigned int baseAddr, unsigned int statFlag)
{
    volatile unsigned status = HWREG(baseAddr + TMR_INTCTLSTAT) & statFlag;

    /* Return statuses of the interrupts. There is no shifting applied here */
    HWREG(baseAddr + TMR_INTCTLSTAT) |= status;
    return status;

}

/**
 * \brief   Sets the Prescalar Counter of Timer34.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   psc34         4 bit prescalar value
 *
 * \return  None.
 *
 **/
void TimerPreScalarCount34Set(unsigned int baseAddr, unsigned int psc34)
{
    HWREG(baseAddr + TMR_TGCR) &= ~(PRESCALE_MASK << TMR_TGCR_PSC34_SHIFT); 

    /* Set the Prescalar value. This is applicable only for Timer34 */
    HWREG(baseAddr + TMR_TGCR) |= ((psc34 & PRESCALE_MASK) << 
                                   TMR_TGCR_PSC34_SHIFT);
}


/**
 * \brief   Returns the Prescalar Counter of Timer34.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 *
 * \return  Prescalar Value.
 *
 **/
unsigned int TimerPreScalarCount34Get(unsigned int baseAddr)
{
    /* 
    ** Return the prescalar value. This is only for Timer34 in 32 bit 
    ** unchained mode.
    */ 
    return((HWREG(baseAddr + TMR_TGCR) & TMR_TGCR_PSC34) >> 
           TMR_TGCR_PSC34_SHIFT);
}


/**
 * \brief   Sets the Timer Divide Down Ratio of Timer34.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   tddr34        TDDR34, Timer Divide Down Ratio
 *
 * \return  None.
 *
 **/
void TimerDivDwnRatio34Set(unsigned int baseAddr, unsigned int tddr34)
{
    HWREG(baseAddr + TMR_TGCR) &= ~(TDDR_MASK << TMR_TGCR_TDDR34_SHIFT);

    /* Set the TDDR. This is only for Timer34  in unchained mode */
    HWREG(baseAddr + TMR_TGCR) |= ((tddr34 & TDDR_MASK) << 
                                   TMR_TGCR_TDDR34_SHIFT);
}

/**
 * \brief   returns the Timer Divide Down Ratio of Timer34.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 *
 * \return  TDDR34 value.
 *
 **/
unsigned int TimerDivDwnRatio34Get(unsigned int baseAddr)
{
    /* Return the TDDR value. Only applicable in unchained mode for Timer34 */
    return((HWREG(baseAddr + TMR_TGCR) & TMR_TGCR_TDDR34) >> 
            TMR_TGCR_TDDR34_SHIFT);
}

/**
 * \brief   Configures the Timer for Capture Mode. The Timer Module Shall be
 *          Configured in 32 bit unchained mode before this API is called.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which capture to be configured.
 * \param   cfgCap        Configuration of Capture Mode.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers \n
 *
 * cfgCap can take the values \n
 *     TMR_CAPT_DISABLE - Capture Mode disable \n
 *     TMR_CAPT_ENABLE_RIS_EDGE - Capture enable at rising edge \n
 *     TMR_CAPT_ENABLE_FALL_EDGE - Capture enable at falling edge \n
 *     TMR_CAPT_ENABLE_BOTH_EDGE - Capture enable at both edges 
 *
 * \return  None.
 *
 **/
void TimerCaptureConfigure(unsigned int baseAddr, unsigned int timer, 
                           unsigned int cfgCap)
{
    /* Clear the bits CAPTEN and edge selection bits */
    HWREG(baseAddr + TMR_TCR) &= ~(timer & (TMR_TCR_CAPEVTMODE12 |
                                            TMR_TCR_CAPMODE12 |
                                            TMR_TCR_CAPEVTMODE34 |
                                            TMR_TCR_CAPMODE34) );

    /* Write the capture configuration along with the edge selection */
    HWREG(baseAddr + TMR_TCR) |= (cfgCap & timer);
}


/**
 * \brief   Enables the timer(s) for read reset mode. The timer shall be
 *          Configured in 32 bit unchained mode before this API is called.
 *          Read reset determines the effect of timer counter read on TIMn. 
 *          If Read reset is enabled, the timer counter will be reset when
 *          the timer counter register TIMn is read.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which Read Reset to be enabled.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers
 *
 * \return  None.
 *
 **/
void TimerReadResetEnable(unsigned int baseAddr, unsigned int timer)
{
    /* 
    ** Enable the read reset mode for the specified timers; The timer counter
    ** for the corresponding timer will be reset for further TIMn reads
    */
    HWREG(baseAddr + TMR_TCR) |= (timer & (TMR_TCR_READRSTMODE12 |
                                           TMR_TCR_READRSTMODE34));
}


/**
 * \brief   Disables the timer for read reset mode.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which Read Reset to be disabled.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers
 *
 * \return  None.
 *
 **/
void TimerReadResetDisable(unsigned int baseAddr, unsigned int timer)
{
    /* Disable the Read Reset mode */
    HWREG(baseAddr + TMR_TCR) &= ~(timer & (TMR_TCR_READRSTMODE12 |
                                            TMR_TCR_READRSTMODE34));
}

/**
 * \brief   Sets the Input Gate Enable. Allows the timer to gate the internal
 *          timer clock source. The timer starts counting when the input pin
 *          goes from Low to High and stops counting when transition happens
 *          from high to low.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which input gate to be enabled.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers
 *
 * \return  None.
 *
 **/
void TimerInputGateEnable(unsigned int baseAddr, unsigned int timer)
{
    /* Set the input gate enable for the timers requested */
    HWREG(baseAddr + TMR_TCR) |= (timer & (TMR_TCR_TIEN12 |
                                           TMR_TCR_TIEN34));
}

/**
 * \brief   Disable the Input Gate. Timer clock will not be gated by input pin.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which input gate to be disabled.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers
 *
 * \return  None.
 *
 **/
void TimerInputGateDisable(unsigned int baseAddr, unsigned int timer)
{
    /* Disable the input gating for the requested timers */
    HWREG(baseAddr + TMR_TCR) &= ~(timer & (TMR_TCR_TIEN12 |
                                            TMR_TCR_TIEN34));
}

/**
 * \brief   Sets the pulse width for the specified timer. Determines the pulse.
 *          width in the TSTATn bit and the OUT pin in pulse mode.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which pulse width to be set.
 * \param   pulseWidth    Pulse width to be set.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers \n
 *
 * pulseWidth can take the following values \n
 *     TMR_PULSE_WIDTH_1_CLK - 1 clock cycle  \n
 *     TMR_PULSE_WIDTH_2_CLK - 2 clock cycles \n
 *     TMR_PULSE_WIDTH_3_CLK - 3 clock cycles \n
 *     TMR_PULSE_WIDTH_4_CLK - 4 clock cycles
 *
 * \return  None.
 *
 **/
void TimerPulseWidthSet(unsigned int baseAddr, unsigned int timer, 
                        unsigned int pulseWidth)
{
    /* Clear the bits for Pulse width selection */
    HWREG(baseAddr + TMR_TCR) &= ~(timer & (TMR_TCR_PWID34 |
                                            TMR_TCR_PWID12));

    /* Set the pulse width for the appropriate timer */
    HWREG(baseAddr + TMR_TCR) |= (timer & (TMR_TCR_PWID34 |
                                           TMR_TCR_PWID12));
}


/**
 * \brief   Sets the clock mode. Once the clock mode is set, the outpin behaves
 *          as 50% duty cycle signal.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which clock mode to be set.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers
 *
 * \return  None.
 *
 **/
void TimerClockModeSet(unsigned int baseAddr, unsigned int timer)
{
    /* Set the clock mode for the appropriate timer */
    HWREG(baseAddr + TMR_TCR) |= (timer & (TMR_TCR_CP12 |
                                           TMR_TCR_CP34));
}

/**
 * \brief   Sets the pulse mode. The outpin goes active when the timer count
 *          reaches the period.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which pulse mode to be set.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34 only \n
 *     TMR_TIMER12 - Timer12 only \n
 *     TMR_TIMER_BOTH - Both timers
 *
 * \return  None.
 *
 **/
void TimerPulseModeSet(unsigned int baseAddr, unsigned int timer)
{
    /*
    ** Clear the clock mode for the appropriate timer
    ** This will enable the pulse mode
    */
    HWREG(baseAddr + TMR_TCR) &= ~(timer & (TMR_TCR_CP12 |
                                            TMR_TCR_CP34));
}

/**
 * \brief   Returns the timer status. The timer status Drives the value of the
 *          timer output TM64P_OUTn when configured.
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which status to be read
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Timer34  \n
 *     TMR_TIMER12 - Timer12 \n
 *     TMR_TIMER_BOTH - Both timers \n
 *
 * Note : This API returns 0 if none of the status bits is set.
 *
 * \return  Status of the timer. Returns the following values or the
 *          combination of both. \n
 *          TMR_OUT12_ASSERTED - TMR64P_OUT12 is asserted \n
 *          TMR_OUT34_ASSERTED - TMR64P_OUT34 is asserted 
 *
 **/
unsigned int TimerOUTStatusGet(unsigned int baseAddr, unsigned int timer)
{
    /* Return the status */
    return(HWREG(baseAddr + TMR_TCR) & (timer & (TMR_TCR_TSTAT12 |
                                                 TMR_TCR_TSTAT34)));
}

/**
 * \brief   Inverts the TMR64P_INn signal
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which inversion to be enabled.
 *
 * timer can take the following values. \n
 *     TMR_TIMER34 - Inverts TMR64P_IN34 signal \n
 *     TMR_TIMER12 - Inverts TMR64P_IN12 signal \n
 *     TMR_TIMER_BOTH - Inverts both TMR64P_IN12 and TMR64P_IN34 signals
 *
 * \return  None.
 *
 **/
void TimerInvertINEnable(unsigned int baseAddr, unsigned int timer)
{
    /* Enable the invert for IN pin for the timers requested */
    HWREG(baseAddr + TMR_TCR) |= (timer & (TMR_TCR_INVINP12 |
                                           TMR_TCR_INVINP34));
}

/**
 * \brief   Disables the Inversion of the TMR64P_INn signal
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which inversion to be disabled.
 * 
 *  timer can take the values \n
 *     TMR_TIMER34 - TMR64P_INT34 inversion will be disabled \n
 *     TMR_TIMER12 - TMR64P_IN12 inversion will be disabled \n
 *     TMR_TIMER_BOTH - Both TMR64P_IN12 and TMR64P_IN34 inversion disabled
 *
 * \return  None.
 *
 **/
void TimerInvertINDisable(unsigned int baseAddr, unsigned int timer)
{
    /* Disable the invert for IN pin for timers requested */
    HWREG(baseAddr + TMR_TCR) &= ~(timer & (TMR_TCR_INVINP12 |
                                            TMR_TCR_INVINP34));
}

/**
 * \brief   Inverts the TMR64P_OUTn signal
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which inversion to be enabled.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - Inverts TMR64P_OUT34 signal \n
 *     TMR_TIMER12 - Inverts TMR64P_OUT12 signal \n
 *     TMR_TIMER_BOTH - Inverts both TMR64P_OUT12 and TMR64P_OUT34 signals
 *
 * \return  None.
 *
 **/
void TimerInvertOUTEnable(unsigned int baseAddr, unsigned int timer)
{
    /* Enable the invert for OUT pin for the timers requested */
    HWREG(baseAddr + TMR_TCR) |= (timer & (TMR_TCR_INVOUTP12 |
                                           TMR_TCR_INVOUTP34));
}

/**
 * \brief   Disables the inversion the TMR64P_OUTn signal
 *
 * \param   baseAddr      Base Address of the Timer Module Registers.
 * \param   timer         The timer, of which inversion to be disabled.
 *
 * timer can take the values \n
 *     TMR_TIMER34 - TMR64P_OUT34 inversion will be disabled \n
 *     TMR_TIMER12 - TMR64P_OUT12 inversion will be disabled \n
 *     TMR_TIMER_BOTH - Both TMR64P_OUT12 and TMR64P_OUT34 inversion disabled
 *
 * \return  None.
 *
 **/
void TimerInvertOUTDisable(unsigned int baseAddr, unsigned int timer)
{
    /* Disable the invert for OUT pin for the timers requested */
    HWREG(baseAddr + TMR_TCR) &= ~(timer & (TMR_TCR_INVOUTP12 |
                                            TMR_TCR_INVOUTP34));
}

/***************************** End Of File ***********************************/
