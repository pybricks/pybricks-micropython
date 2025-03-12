// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors
// Copyright (c) 2013, 2014 Damien P. George

#include <stdint.h>

#include <contiki.h>

#include <pbdrv/config.h>
#include <pbdrv/clock.h>
#include <pbio/main.h>
#include <pbsys/bluetooth.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mpconfig.h"
#include "py/stream.h"

void pb_stack_get_info(char **sstack, char **estack) {
    extern uint32_t _estack;
    extern uint32_t _sstack;
    *sstack = (char *)&_sstack;
    *estack = (char *)&_estack;
}

static inline int arm_wfi(void) {
    __asm volatile (
        "mov	r0, #0\n"
        "mcr	p15, 0, r0, c7, c0, 4\n"        /* wait for interrupt */
        ::: "r0"
        );
    return 0;
}

void pb_event_poll_hook_leave(void) {
    // There is a possible race condition where an interrupt occurs and sets the
    // Contiki poll_requested flag after all events have been processed. So we
    // have a critical section where we disable interrupts and check see if there
    // are any last second events. If not, we can call __WFI(), which still wakes
    // up the CPU on interrupt even though interrupts are otherwise disabled.
    mp_uint_t state = disable_irq();
    if (!process_nevents()) {
        arm_wfi();
    }
    enable_irq(state);
}

// Core delay function that does an efficient sleep and may switch thread context.
// If IRQs are enabled then we must have the GIL.
void mp_hal_delay_ms(mp_uint_t Delay) {
    // IRQs enabled, so can use systick counter to do the delay
    uint32_t start = pbdrv_clock_get_ms();
    // Wraparound of tick is taken care of by 2's complement arithmetic.
    do {
        // This macro will execute the necessary idle behaviour.  It may
        // raise an exception, switch threads or enter sleep mode (waiting for
        // (at least) the SysTick interrupt).
        MICROPY_EVENT_POLL_HOOK
    } while (pbdrv_clock_get_ms() - start < Delay);
}

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    return ret;
}

int mp_hal_stdin_rx_chr(void) {
    uint8_t c = 0;
    return c;
}

typedef struct {
    volatile uint8_t *thr;
    volatile uint8_t *lsr;
} pb_hal_uart_t;

// Sensor port 1
static pb_hal_uart_t UART0 = { .thr = (volatile uint8_t *)0x01D0C000, .lsr = (volatile uint8_t *)0x01D0C014 };

static void debug(pb_hal_uart_t *uart, const char *s) {
    while (*s) {
        while ((*uart->lsr & (1 << 5)) == 0) {
        }
        *uart->thr = *s++;
    }
}

void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    debug(&UART0, str);
    // MICROPY_EVENT_POLL_HOOK
}

void mp_hal_stdout_tx_flush(void) {
}
