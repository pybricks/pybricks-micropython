/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <pbdrv/config.h>

#include "sys/clock.h"
#include "sys/etimer.h"

#include "stm32f070xb.h"

#if CLOCK_CONF_SECOND != 1000
#error Clock must be set to 1 msec ticks
#endif

volatile clock_time_t clock_time_ticks;

void clock_init(void) {
    // SysTick set for 1ms ticks
    SysTick_Config(PBDRV_CONFIG_SYS_CLOCK_RATE / 1000);
}

clock_time_t clock_time() {
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
