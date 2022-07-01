// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George
// Copyright (c) 2018-2021 The Pybricks Authors

#include <contiki.h>

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_STM32

#include STM32_H

// NB: pbdrv_clock_ticks is intended to be private, but making it static
// breaks things.
volatile uint32_t pbdrv_clock_ticks;

void pbdrv_clock_init(void) {
    // STM32 does platform-specific clock init early in SystemInit()
}

uint32_t pbdrv_clock_get_ms(void) {
    return pbdrv_clock_ticks;
}

// The SysTick timer counts down at 168 MHz, so we can use that knowledge
// to grab a microsecond counter.
static uint32_t pbdrv_clock_get_time(uint32_t ticks_per_ms) {
    uint32_t irq_state, counter, msec, status;

    irq_state = __get_PRIMASK();
    __disable_irq();
    counter = SysTick->VAL;
    msec = pbdrv_clock_ticks;
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
    //
    // Generalizing to a given number of ticks per millisecond, we get:
    return msec * ticks_per_ms + (counter * ticks_per_ms) / (load + 1);
}

uint32_t pbdrv_clock_get_100us(void) {
    return pbdrv_clock_get_time(10);
}

uint32_t pbdrv_clock_get_us(void) {
    return pbdrv_clock_get_time(1000);
}

void SysTick_Handler(void) {
    pbdrv_clock_ticks++;

    // Read the systick control regster. This has the side effect of clearing
    // the COUNTFLAG bit, which makes the logic in pbdrv_clock_get_time
    // work properly.
    SysTick->CTRL;

    etimer_request_poll();
}

uint32_t HAL_GetTick(void) {
    return pbdrv_clock_ticks;
}

// We provide our own version of HAL_Delay that calls __WFI while waiting,
// and works when interrupts are disabled.  This function is intended to be
// used only by the ST HAL functions.
void HAL_Delay(uint32_t Delay) {
    if (__get_PRIMASK() == 0) {
        // IRQs enabled, so can use systick counter to do the delay
        uint32_t start = pbdrv_clock_ticks;
        // Wraparound of tick is taken care of by 2's complement arithmetic.
        while (pbdrv_clock_ticks - start < Delay) {
            // Enter sleep mode, waiting for (at least) the SysTick interrupt.
            __WFI();
        }
    } else {
        // IRQs disabled, so need to use a busy loop for the delay.
        // To prevent possible overflow of the counter we use a double loop.
        const uint32_t count_1ms = PBDRV_CONFIG_SYS_CLOCK_RATE / 4000;
        for (uint32_t i = 0; i < Delay; i++) {
            for (uint32_t count = 0; ++count <= count_1ms;) {
            }
        }
    }
}

#endif // PBDRV_CONFIG_CLOCK_STM32
