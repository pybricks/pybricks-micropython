// SPDX-License-Identifier: MPL-1.0
// Copyright (c) 2016 Tobias Schießl

// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors


#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_TIAM1808

#include <stdint.h>

#include <contiki.h>

#include "soc_AM1808.h"
#include "hw_syscfg0_AM1808.h"
#include "timer.h"
#include "evmAM1808.h"
#include "cpu.h"
#include "interrupt.h"
#include "stdio.h"

/**
 * The compare value to set for the 16 least significant bits of the hardware timer
 *
 * This is the value which causes the interrupt to be triggered every millisecond.
 */
#define TMR_PERIOD_LSB32              0x05CC
/*
 * The compare value to set for the 16 most significant bits of the hardware timer
 */
#define TMR_PERIOD_MSB32              0x0

/**
 * The current tick in milliseconds
 */
volatile uint32_t systick_ms = 0;

/**
 * The systick interrupt service routine (ISR) which will be called every millisecond.
 */
void systick_isr_C(void) {
    /* Disable the timer interrupt */
    TimerIntDisable(SOC_TMR_0_REGS, TMR_INT_TMR34_NON_CAPT_MODE);

    /* Clear the interrupt status in AINTC and in timer */
    IntSystemStatusClear(SYS_INT_TINT34_0);
    TimerIntStatusClear(SOC_TMR_0_REGS, TMR_INT_TMR34_NON_CAPT_MODE);

    ++systick_ms;

    etimer_request_poll();

    /* Enable the timer interrupt */
    TimerIntEnable(SOC_TMR_0_REGS, TMR_INT_TMR34_NON_CAPT_MODE);
}

/**
 * Disable the timer and therefore the systick
 *
 */
void systick_suspend(void) {
    /* Disable the timer interrupt */
    TimerDisable(SOC_TMR_0_REGS, TMR_TIMER34);
}

/**
 * Enable the timer and therefore the systick
 */
void systick_resume(void) {
    /* Enable the timer interrupt */
    TimerEnable(SOC_TMR_0_REGS, TMR_TIMER34, TMR_ENABLE_CONTRELOAD);
}

/**
 * Initialize the systick module, i.e. the hardware timer of the SoC
 *
 * This function will register the corresponding ISR, enable the timer
 * interrupt and configure interrupt channel 2 (normal interrupt) for the
 * hardware timer.
 */
void pbdrv_clock_init(void) {
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

uint32_t pbdrv_clock_get_us(void) {
    // TODO: TIAM1808 implementation.
    return 0;
}

uint32_t pbdrv_clock_get_ms(void) {
    return systick_ms;
}

uint32_t pbdrv_clock_get_100us(void) {
    // TODO: TIAM1808 implementation (1 count = 100us, so 10 counts per millisecond.)
    return 0;
}

#endif // PBDRV_CONFIG_CLOCK_TIAM1808
