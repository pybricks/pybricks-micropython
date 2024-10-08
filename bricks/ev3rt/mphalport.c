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

void pb_event_poll_hook_leave(void) {
}

// Runs MicroPython with the given program data.
void pb_stack_get_info(char **sstack, char **estack) {
    extern const TINIB _kernel_tinib_table[];
    const TINIB *main_task_init_block = &_kernel_tinib_table[MAIN_TASK - 1];
    *sstack = main_task_init_block->sstk;
    *estack = *sstack + (uint32_t)main_task_init_block->sstksz;
}


// On the /ev3rt/etc/rc.conf.ini choose:
// [Bluetooth] TurnOff=1 and [Sensors] DisablePort1=1 to use sensor port 1 for stdio
// [Bluetooth] TurnOff=0 and [Sensors] DisablePort1=0 to use bluetooth rfcomm0 for stdio
static FILE *fd = NULL;
extern const bool_t *ev3rt_bluetooth_disabled;
extern const bool_t *ev3rt_sensor_port_1_disabled;
static serial_port_t serial_port_type = EV3_SERIAL_UART;

static bool serial_configured(uint32_t *in_waiting) {
    if (!fd) {
        serial_port_type = (*ev3rt_sensor_port_1_disabled && *ev3rt_bluetooth_disabled) ? serial_port_type : EV3_SERIAL_BT;

        fd = ev3_serial_open_file(serial_port_type);
        syslog(LOG_NOTICE, "open serial file");

        // Disables default echo and extra CR on REPL, which confuses MicroPython.
        serial_ctl_por(serial_port_type, (/*IOCTL_ECHO | IOCTL_CRLF | */ IOCTL_FCSND | IOCTL_FCRCV));
    }
    T_SERIAL_RPOR rpor;
    ER ercd = serial_ref_por(serial_port_type, &rpor);
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
    if ((poll_flags & MP_STREAM_POLL_RD) && serial_configured(&in_waiting) && in_waiting) {
        ret |= MP_STREAM_POLL_RD;
    }

    return ret;
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    uint32_t in_waiting;
    if (!serial_configured(&in_waiting)) {
        return 0;
    }
    // while (serial_configured(&in_waiting) && in_waiting == 0) {
    //     MICROPY_EVENT_POLL_HOOK
    // }
    uint8_t c;
    serial_rea_dat(serial_port_type, (char *)&c, 1);
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    uint32_t in_waiting;
    if (serial_configured(&in_waiting)) {
        serial_wri_dat(serial_port_type, str, len);
    }
}

void mp_hal_stdout_tx_flush(void) {
    // currently not buffered
}
