/**
*  \file systick.c
*    
*  \brief This header contains function definitions required to manage the tick in milliseconds on the EV3.
*
*  The tick is provided by a hardware timer of the AM1808 SoC which will trigger an interrupt every millisecond. The timer runs in 32 bit mode.
* 
*  \author Tobias Schießl
*/

/* Include statements */
#include "soc_AM1808.h"
#include "hw_syscfg0_AM1808.h"
#include "timer.h"
#include "evmAM1808.h"
#include "cpu.h"
#include "systick.h"
#include "interrupt.h"
#include "stdio.h"

/* Macro definitions */
/**
* \brief The compare value to set for the 16 least significant bits of the hardware timer
*
* This is the value which causes the interrupt to be triggered every millisecond.
**/ 
#define TMR_PERIOD_LSB32              0x05CC
/**
* \brief The compare value to set for the 16 most significant bits of the hardware timer
**/ 
#define TMR_PERIOD_MSB32              0x0

/* Global variable definitions */
/**
* \brief The current tick in milliseconds
**/ 
volatile U32 systick_ms = 0;

/* Local function declarations */
void systick_isr_C(void);

/**
* \brief The systick interrupt service routine (ISR) which will be called every millisecond
* 
* This ISR will increase the systick_ms variable and reset the interrupt flags.
*
* \return none
**/
void systick_isr_C(void) {
    /* Disable the timer interrupt */
    TimerIntDisable(SOC_TMR_0_REGS, TMR_INT_TMR34_NON_CAPT_MODE);

    /* Clear the interrupt status in AINTC and in timer */
    IntSystemStatusClear(SYS_INT_TINT34_0);
    TimerIntStatusClear(SOC_TMR_0_REGS, TMR_INT_TMR34_NON_CAPT_MODE);
  
    ++systick_ms;
  
    /* Enable the timer interrupt */
    TimerIntEnable(SOC_TMR_0_REGS, TMR_INT_TMR34_NON_CAPT_MODE);
}

/**
* \brief Get the current tick in milliseconds
*
* \return The current tick in milliseconds
**/
U32 systick_get_ms(void) {
    return systick_ms;
}

/**
* \brief Wait for the specified amount of time
* 
* Waiting with this function is an active waiting which will block until the time has elapsed.
*
* \param ms - The time to wait in milliseconds
*
* \return none
**/
void systick_wait_ms(U32 ms) {
    volatile U32 final = ms + systick_ms;
    while (systick_ms < final)
        ;
}

/**
* \brief Wait for the specified amount of time
* 
* Waiting with this function is an active waiting which will block until the time has elapsed.
*
* \param ns - The time to wait in nanoseconds
*
* \return none
**/
void systick_wait_ns(U32 ns) {
	// We assume that counting to 85 equals about 100 ns
    for (int i = 0; i < (int)(((float)85 / (float)100) * ns); ++i)
		; // Do nothing
}

/**
* \brief Initialize the systick module, i.e. the hardware timer of the SoC
* 
* This function will register the corresponding ISR, enable the timer interrupt and configure interrupt channel 2 (normal interrupt) for the hardware timer.
*
* \return none
**/
void systick_init(void) {
    /* Set up the ARM Interrupt Controller for generating timer interrupt */
    
    /* Set up the timer */
    TimerConfigure(SOC_TMR_0_REGS, TMR_CFG_32BIT_UNCH_CLK_BOTH_INT);
    TimerPeriodSet(SOC_TMR_0_REGS, TMR_TIMER34, TMR_PERIOD_LSB32);
    TimerReloadSet(SOC_TMR_0_REGS, TMR_TIMER34, TMR_PERIOD_LSB32);
    
    /* Register the Timer ISR */
    IntRegister(SYS_INT_TINT34_0, systick_isr_C);
  
    /* Set the channel number for Timer interrupt, it will map to IRQ */
    IntChannelSet(SYS_INT_TINT34_0, 2);
    
    /* Enable timer interrupts in AINTC */
    IntSystemEnable(SYS_INT_TINT34_0);
    
    /* Enable the timer interrupt */
    TimerIntEnable(SOC_TMR_0_REGS, TMR_INT_TMR34_NON_CAPT_MODE); 
    
    /* Start the timer */
    TimerEnable(SOC_TMR_0_REGS, TMR_TIMER34, TMR_ENABLE_CONTRELOAD);
}

/**
* \brief Disable the timer and therefore the systick
*
* \return none
**/
void systick_suspend(void) {
    /* Disable the timer interrupt */
    TimerDisable(SOC_TMR_0_REGS, TMR_TIMER34); 
}

/**
* \brief Enable the timer and therefore the systick
*
* \return none
**/
void systick_resume(void) {
    /* Enable the timer interrupt */
    TimerEnable(SOC_TMR_0_REGS, TMR_TIMER34, TMR_ENABLE_CONTRELOAD);
}