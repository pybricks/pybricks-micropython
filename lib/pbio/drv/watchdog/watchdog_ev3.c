// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Watchdog timer driver for EV3.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_WATCHDOG_EV3

#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/timer.h>

// The input to Timer1 is PLL0_AUXCLK which is 24 MHz
// Configure the timeout to be 3 seconds
#define WDT_TIMEOUT_SECONDS     3ull
#define WDT_PERIOD_LSB          ((WDT_TIMEOUT_SECONDS * SOC_ASYNC_2_FREQ) & 0xffffffff)
#define WDT_PERIOD_MSB          (((WDT_TIMEOUT_SECONDS * SOC_ASYNC_2_FREQ) >> 32) & 0xffffffff)

void pbdrv_watchdog_init(void) {
    TimerDisable(SOC_TMR_1_REGS, TMR_TIMER_BOTH);
    TimerConfigure(SOC_TMR_1_REGS, TMR_CFG_64BIT_WATCHDOG);
    TimerPeriodSet(SOC_TMR_1_REGS, TMR_TIMER12, WDT_PERIOD_LSB);
    TimerPeriodSet(SOC_TMR_1_REGS, TMR_TIMER34, WDT_PERIOD_MSB);
    TimerWatchdogActivate(SOC_TMR_1_REGS);
}

void pbdrv_watchdog_update(void) {
    TimerWatchdogReactivate(SOC_TMR_1_REGS);
}

#endif // PBDRV_CONFIG_WATCHDOG_EV3
