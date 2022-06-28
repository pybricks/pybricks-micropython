// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors
// Copyright (c) 2013, 2014 Damien P. George

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbsys/bluetooth.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mpconfig.h"
#include "py/stream.h"

// using "internal" pbdrv variable
extern volatile uint32_t pbdrv_clock_ticks;

// Core delay function that does an efficient sleep and may switch thread context.
// If IRQs are enabled then we must have the GIL.
void mp_hal_delay_ms(mp_uint_t Delay) {
    if (__get_PRIMASK() == 0) {
        // IRQs enabled, so can use systick counter to do the delay
        uint32_t start = pbdrv_clock_ticks;
        // Wraparound of tick is taken care of by 2's complement arithmetic.
        do {
            // This macro will execute the necessary idle behaviour.  It may
            // raise an exception, switch threads or enter sleep mode (waiting for
            // (at least) the SysTick interrupt).
            MICROPY_EVENT_POLL_HOOK
        } while (pbdrv_clock_ticks - start < Delay);
    } else {
        // IRQs disabled, so need to use a busy loop for the delay.
        // To prevent possible overflow of the counter we use a double loop.
        const uint32_t count_1ms = PBDRV_CONFIG_SYS_CLOCK_RATE / 4000;
        for (mp_uint_t i = 0; i < Delay; i++) {
            for (uint32_t count = 0; ++count <= count_1ms;) {
            }
        }
    }
}

// delay for given number of microseconds
void mp_hal_delay_us(mp_uint_t usec) {
    if (__get_PRIMASK() == 1) {
        // IRQs enabled, so can use systick counter to do the delay
        uint32_t start = pbdrv_clock_get_us();
        while (pbdrv_clock_get_us() - start < usec) {
        }
    } else {
        // IRQs disabled, so need to use a busy loop for the delay
        // sys freq is always a multiple of 2MHz, so division here won't lose precision
        const uint32_t ucount = PBDRV_CONFIG_SYS_CLOCK_RATE / 2000000 * usec / 2;
        for (uint32_t count = 0; ++count <= ucount;) {
        }
    }
}

#if PYBRICKS_HUB_DEBUG

#include "stm32f4xx.h"

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;

    if ((poll_flags & MP_STREAM_POLL_RD) && USART6->SR & USART_SR_RXNE) {
        ret |= MP_STREAM_POLL_RD;
    }

    return ret;
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    while (!(USART6->SR & USART_SR_RXNE)) {
        MICROPY_VM_HOOK_LOOP
    }

    return USART6->DR;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        while (!(USART6->SR & USART_SR_TXE)) {
            MICROPY_VM_HOOK_LOOP
        }
        USART6->DR = *str++;
    }
}

#else // !PYBRICKS_HUB_DEBUG

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;

    if ((poll_flags & MP_STREAM_POLL_RD) && pbsys_bluetooth_rx_get_available()) {
        ret |= MP_STREAM_POLL_RD;
    }

    return ret;
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    uint32_t size;
    uint8_t c;

    // wait for rx interrupt
    while (size = 1, pbsys_bluetooth_rx(&c, &size) != PBIO_SUCCESS) {
        MICROPY_EVENT_POLL_HOOK
    }

    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len) {
        uint32_t size = len;
        pbio_error_t err = pbsys_bluetooth_tx((const uint8_t *)str, &size);

        if (err == PBIO_SUCCESS) {
            str += size;
            len -= size;
            continue;
        }

        if (err != PBIO_ERROR_AGAIN) {
            // Ignoring error for now. This means stdout lost if Bluetooth is
            // disconnected.
            return;
        }

        MICROPY_EVENT_POLL_HOOK
    }
}

#endif // PYBRICKS_HUB_DEBUG
