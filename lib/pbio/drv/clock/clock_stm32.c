// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_STM32

#include <contiki.h>

#include STM32_H

#if CLOCK_CONF_SECOND != 1000
#error Clock must be set to 1 msec ticks
#endif

volatile clock_time_t clock_time_ticks;

void clock_init(void) {
    // STM32 does platform-specific clock init early in SystemInit()
}

clock_time_t clock_time(void) {
    return clock_time_ticks;
}

// The SysTick timer counts down at 168 MHz, so we can use that knowledge
// to grab a microsecond counter.
uint32_t clock_usecs(void) {
    uint32_t irq_state, counter, msec, status;

    irq_state = __get_PRIMASK();
    __disable_irq();
    counter = SysTick->VAL;
    msec = clock_time_ticks;
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

// delay for given number of microseconds
void clock_delay_usec(uint16_t usec) {
    if (__get_PRIMASK() == 1) {
        // IRQs enabled, so can use systick counter to do the delay
        uint32_t start = clock_usecs();
        while (clock_usecs() - start < usec) {
        }
    } else {
        // IRQs disabled, so need to use a busy loop for the delay
        // sys freq is always a multiple of 2MHz, so division here won't lose precision
        const uint32_t ucount = PBDRV_CONFIG_SYS_CLOCK_RATE / 2000000 * usec / 2;
        for (uint32_t count = 0; ++count <= ucount;) {
        }
    }
}

void SysTick_Handler(void) {
    clock_time_ticks++;

    // Read the systick control regster. This has the side effect of clearing
    // the COUNTFLAG bit, which makes the logic in clock_usecs
    // work properly.
    SysTick->CTRL;

    etimer_request_poll();
}

uint32_t HAL_GetTick(void) {
    return clock_time_ticks;
}

// We provide our own version of HAL_Delay that calls __WFI while waiting,
// and works when interrupts are disabled.  This function is intended to be
// used only by the ST HAL functions.
void HAL_Delay(uint32_t Delay) {
    if (__get_PRIMASK() == 0) {
        // IRQs enabled, so can use systick counter to do the delay
        uint32_t start = clock_time_ticks;
        // Wraparound of tick is taken care of by 2's complement arithmetic.
        while (clock_time_ticks - start < Delay) {
            // Enter sleep mode, waiting for (at least) the SysTick interrupt.
            __WFI();
        }
    } else {
        // IRQs disabled, so need to use a busy loop for the delay.
        // To prevent possible overflow of the counter we use a double loop.
        const uint32_t count_1ms = PBDRV_CONFIG_SYS_CLOCK_RATE / 4000;
        for (int i = 0; i < Delay; i++) {
            for (uint32_t count = 0; ++count <= count_1ms;) {
            }
        }
    }
}

#endif // PBDRV_CONFIG_CLOCK_STM32
