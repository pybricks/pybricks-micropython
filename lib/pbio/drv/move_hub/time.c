// SPDX-License-Identifier: MIT
// This file is based on the STM32 SysTick code from MicroPython
// Copyright (c) 2013, 2014 Damien P. George 

#include "stm32f070xb.h"

volatile uint32_t pbdrv_time_msec_ticks;

uint32_t pbdrv_time_get_msec() {
    return pbdrv_time_msec_ticks;
}

// The SysTick timer counts down at 168 MHz, so we can use that knowledge
// to grab a microsecond counter.
uint32_t pbdrv_time_get_usec(void) {
    uint32_t irq_state, counter, msec, status;

    irq_state = __get_PRIMASK();
    __disable_irq();
    counter = SysTick->VAL;
    msec = pbdrv_time_msec_ticks;
    status = SysTick->CTRL;
    __set_PRIMASK(irq_state);

    // It's still possible for the countflag bit to get set if the counter was
    // reloaded between reading VAL and reading CTRL. With interrupts  disabled
    // it definitely takes less than 50 HCLK cycles between reading VAL and
    // reading CTRL, so the test (counter > 50) is to cover the case where VAL
    // is +ve and very close to zero, and the COUNTFLAG bit is also set.
    if ((status & SysTick_CTRL_COUNTFLAG_Msk) && counter > 50) {
        // This means that the HW reloaded VAL between the time we read VAL and the
        // time we read CTRL, which implies that there is an interrupt pending
        // to increment the tick counter.
        msec++;
    }
    uint32_t load = SysTick->LOAD;
    counter = load - counter; // Convert from decrementing to incrementing

    // ((load + 1) / 1000) is the number of counts per microsecond.
    //
    // counter / ((load + 1) / 1000) scales from the systick clock to microseconds
    // and is the same thing as (counter * 1000) / (load + 1)
    return msec * 1000 + (counter * 1000) / (load + 1);
}

void SysTick_Handler(void) {
    pbdrv_time_msec_ticks++;

    // Read the systick control regster. This has the side effect of clearing
    // the COUNTFLAG bit, which makes the logic in mp_hal_ticks_us
    // work properly.
    SysTick->CTRL;
}
