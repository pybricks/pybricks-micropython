// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors
// Copyright (c) 2013, 2014 Damien P. George

#include <pbio/main.h>

#include "py/builtin.h"
#include "py/lexer.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "shared/runtime/pyexec.h"

#include "api_common.h"
#include "ev3api.h"
#include "kernel_cfg.h"
#include "kernel/task.h"


// Implementation for MICROPY_EVENT_POLL_HOOK
void pb_poll(void) {
    pbio_process_events();
    mp_handle_pending(true);
}

// Runs MicroPython with the given program data.
void pb_stack_get_info(char **sstack, char **estack) {
    extern const TINIB _kernel_tinib_table[];
    const TINIB *main_task_init_block = &_kernel_tinib_table[MAIN_TASK - 1];
    *sstack = main_task_init_block->sstk;
    *estack = *sstack + (uint32_t)main_task_init_block->sstksz;
}

static FILE *bt = NULL;

bool bluetooth_is_connected(uint32_t *in_waiting) {
    if (!bt) {
        bt = ev3_serial_open_file(EV3_SERIAL_BT);
        syslog(LOG_NOTICE, "open bt file");
    }
    T_SERIAL_RPOR rpor;
    ER ercd = serial_ref_por(EV3_SERIAL_BT, &rpor);
    *in_waiting = 0;
    return ercd == E_OK;
}


void mp_hal_delay_ms(mp_uint_t Delay) {
    uint32_t start = pbdrv_clock_get_ms();
    do {
        MICROPY_EVENT_POLL_HOOK
    } while (pbdrv_clock_get_ms() - start < Delay);
}

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    uint32_t in_waiting;
    if ((poll_flags & MP_STREAM_POLL_RD) && bluetooth_is_connected(&in_waiting) && in_waiting) {
        ret |= MP_STREAM_POLL_RD;
    }

    return ret;
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    uint32_t in_waiting;
    if (!bluetooth_is_connected(&in_waiting)) {
        return 0;
    }
    // while (bluetooth_is_connected(&in_waiting) && in_waiting == 0) {
    //     MICROPY_EVENT_POLL_HOOK
    // }
    uint8_t c;
    serial_rea_dat(EV3_SERIAL_BT, (char *)&c, 1);
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    uint32_t in_waiting;
    if (bluetooth_is_connected(&in_waiting)) {
        serial_wri_dat(EV3_SERIAL_BT, str, len);
    }
}
