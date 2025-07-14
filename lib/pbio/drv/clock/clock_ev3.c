// SPDX-License-Identifier: MPL-1.0
// Copyright (c) 2016 Tobias Schießl

// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors


#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_TIAM1808

#include <stdint.h>

#include <contiki.h>

#include <pbio/os.h>

#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/armv5/cpu.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/timer.h>

// The input to the Timer0 module is PLL0_AUXCLK. This will always run at 24 MHz on the EV3,
// regardless of what the CPU speed is set to. We configure the timer to generate interrupts
// every millisecond, and we configure the timer to reset to 0 when it reaches a count value
// corresponding to one millisecond of time. Finer timing resolution can be obtained by
// combining the software-managed millisecond counter with the timer value.
//
// The system tick uses the "12" half of the timer, and the PRU1 (TODO) uses the "34" half.
static const uint32_t auxclk_freq_hz = SOC_ASYNC_2_FREQ;
static const uint32_t timer_ms_period = (auxclk_freq_hz / 1000) - 1;

/**
 * The current tick in milliseconds
 */
volatile uint32_t systick_ms = 0;

/**
 * The systick interrupt service routine (ISR) which will be called every millisecond.
 */
void systick_isr_C(void) {
    /* Clear the interrupt status in AINTC and in timer */
    IntSystemStatusClear(SYS_INT_TINT12_0);
    TimerIntStatusClear(SOC_TMR_0_REGS, TMR_INTSTAT12_TIMER_NON_CAPT);

    ++systick_ms;

    etimer_request_poll();
    pbio_os_request_poll();
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
    TimerPeriodSet(SOC_TMR_0_REGS, TMR_TIMER12, timer_ms_period);

    /* Register the Timer ISR */
    IntRegister(SYS_INT_TINT12_0, systick_isr_C);

    /* Set the channel number for Timer interrupt, it will map to IRQ */
    IntChannelSet(SYS_INT_TINT12_0, 3);

    /* Enable timer interrupts in AINTC */
    IntSystemEnable(SYS_INT_TINT12_0);

    /* Enable the timer interrupt */
    TimerIntEnable(SOC_TMR_0_REGS, TMR_INT_TMR12_NON_CAPT_MODE);

    /* Start the timer */
    TimerEnable(SOC_TMR_0_REGS, TMR_TIMER12, TMR_ENABLE_CONT);
}

uint32_t pbdrv_clock_get_us(void) {
    // TODO: TIAM1808 implementation.
    return 0;
}

uint32_t pbdrv_clock_get_ms(void) {
    return systick_ms;
}

uint32_t pbdrv_clock_get_100us(void) {
    // REVISIT: Use actual time since this isn't providing better resolution.
    return pbdrv_clock_get_ms() * 10;
}

#endif // PBDRV_CONFIG_CLOCK_TIAM1808
