// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors
// Copyright (c) 2013, 2014 Damien P. George

#include <stdint.h>

#include <contiki.h>

#include <pbdrv/config.h>
#include <pbdrv/clock.h>
#include <pbdrv/uart.h>

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


extern int32_t pbdrv_uart_char_get(pbdrv_uart_dev_t *uart_dev);

extern uint32_t pbdrv_uart_char_available(pbdrv_uart_dev_t *uart_dev);

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {

    pbdrv_uart_dev_t *uart_dev;
    pbio_error_t err = pbdrv_uart_get(1, &uart_dev);
    if (err != PBIO_SUCCESS) {
        return 0;
    }

    uintptr_t ret = 0;

    if ((poll_flags & MP_STREAM_POLL_RD) && pbdrv_uart_char_available(uart_dev)) {
        ret |= MP_STREAM_POLL_RD;
    }

    return ret;
}

int mp_hal_stdin_rx_chr(void) {

    pbdrv_uart_dev_t *uart_dev;
    pbio_error_t err = pbdrv_uart_get(0, &uart_dev);
    if (err != PBIO_SUCCESS) {
        return 0;
    }

    int val;
    while ((val = pbdrv_uart_char_get(uart_dev)) < 0) {
        MICROPY_EVENT_POLL_HOOK;
    }

    return val;
}

typedef struct {
    volatile uint8_t *thr;
    volatile uint8_t *lsr;
} pb_hal_uart_t;

void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    static struct pt pt;
    static pbio_error_t err;

    static pbdrv_uart_dev_t *uart_dev;
    pbdrv_uart_get(0, &uart_dev);

    PT_INIT(&pt);
    while (PT_SCHEDULE(pbdrv_uart_write(&pt, uart_dev, (uint8_t *)str, len, 250, &err))) {
        MICROPY_VM_HOOK_LOOP;
    }
}

void mp_hal_stdout_tx_flush(void) {
}
