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

#define PBDRV_UART_DEV_ID (1)

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    return 0;
}

int mp_hal_stdin_rx_chr(void) {

    pbdrv_uart_dev_t *uart_dev;
    pbio_error_t err = pbdrv_uart_get(PBDRV_UART_DEV_ID, &uart_dev);
    if (err != PBIO_SUCCESS) {
        return -1;
    }

    uint8_t buf[1];
    static struct pt pt;
    PT_INIT(&pt);
    while (PT_SCHEDULE(pbdrv_uart_read(&pt, uart_dev, buf, sizeof(buf), 250, &err))) {
        MICROPY_VM_HOOK_LOOP;
    }

    return err == PBIO_SUCCESS ? buf[0] : -1;
}

void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {

    pbdrv_uart_dev_t *uart_dev;
    pbio_error_t err = pbdrv_uart_get(PBDRV_UART_DEV_ID, &uart_dev);
    if (err != PBIO_SUCCESS) {
        return;
    }

    static struct pt pt;
    PT_INIT(&pt);
    while (PT_SCHEDULE(pbdrv_uart_write(&pt, uart_dev, (uint8_t *)str, len, 250, &err))) {
        MICROPY_VM_HOOK_LOOP;
    }
}

void mp_hal_stdout_tx_flush(void) {

    pbdrv_uart_dev_t *uart_dev;
    pbio_error_t err = pbdrv_uart_get(PBDRV_UART_DEV_ID, &uart_dev);
    if (err != PBIO_SUCCESS) {
        return;
    }

    pbdrv_uart_flush(uart_dev);
}
