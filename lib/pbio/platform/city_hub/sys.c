// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <contiki.h>

#include "pbdrv/bluetooth.h"

#include "pbio/color.h"
#include "pbio/event.h"
#include "pbio/light.h"
#include "pbio/motorpoll.h"

#include <pbsys/battery.h>
#include <pbsys/status.h>
#include <pbsys/supervisor.h>
#include <pbsys/sys.h>

#include "stm32f030xc.h"

#include "../sys/hmi.h"

// ring buffer size for stdin data - must be power of 2!
#define STDIN_BUF_SIZE 128

// user program stop function
static pbsys_stop_callback_t user_stop_func;
// user program stdin event function
static pbsys_stdin_event_callback_t user_stdin_event_func;

// stdin ring buffer
static uint8_t stdin_buf[STDIN_BUF_SIZE];
static uint8_t stdin_buf_head, stdin_buf_tail;

PROCESS(pbsys_process, "System");

void pbsys_prepare_user_program(const pbsys_user_program_callbacks_t *callbacks) {
    if (callbacks) {
        user_stop_func = callbacks->stop;
        user_stdin_event_func = callbacks->stdin_event;
    } else {
        user_stop_func = NULL;
        user_stdin_event_func = NULL;
    }
    pbsys_status_set(PBSYS_STATUS_USER_PROGRAM_RUNNING);
}

void pbsys_unprepare_user_program(void) {
    pbsys_status_clear(PBSYS_STATUS_USER_PROGRAM_RUNNING);
    user_stop_func = NULL;
    user_stdin_event_func = NULL;
    _pbio_motorpoll_reset_all();
}

pbio_error_t pbsys_stdin_get_char(uint8_t *c) {
    if (stdin_buf_head == stdin_buf_tail) {
        return PBIO_ERROR_AGAIN;
    }

    *c = stdin_buf[stdin_buf_tail];
    stdin_buf_tail = (stdin_buf_tail + 1) & (STDIN_BUF_SIZE - 1);

    return PBIO_SUCCESS;
}

pbio_error_t pbsys_stdout_put_char(uint8_t c) {
    return pbdrv_bluetooth_tx(c);
}

static void init(void) {
    IWDG->KR = 0x5555; // enable register access
    IWDG->PR = IWDG_PR_PR_2; // divide by 64
    IWDG->RLR = 1875; // 40kHz / 64 / 1875 = 0.33.. Hz => 3 second timeout
    IWDG->KR = 0xaaaa; // refresh counter
    IWDG->KR = 0xcccc; // start watchdog timer
}

static void handle_stdin_char(uint8_t c) {
    uint8_t new_head = (stdin_buf_head + 1) & (STDIN_BUF_SIZE - 1);

    // optional hook function can steal the character
    if (user_stdin_event_func && user_stdin_event_func(c)) {
        return;
    }

    // otherwise write character to ring buffer

    if (new_head == stdin_buf_tail) {
        // overflow. drop the data :-(
        return;
    }
    stdin_buf[stdin_buf_head] = c;
    stdin_buf_head = new_head;
}

PROCESS_THREAD(pbsys_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    init();
    pbsys_battery_init();
    pbsys_hmi_init();
    etimer_set(&timer, clock_from_msec(50));

    while (true) {
        PROCESS_WAIT_EVENT();
        pbsys_hmi_handle_event(ev, data);
        if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            etimer_reset(&timer);
            pbsys_battery_poll();
            pbsys_hmi_poll();
            pbsys_supervisor_poll();
        } else if (ev == PBIO_EVENT_STATUS_SET) {
            if ((pbsys_status_t)data == PBSYS_STATUS_POWER_BUTTON_PRESSED && user_stop_func) {
                user_stop_func();
            }
        } else if (ev == PBIO_EVENT_UART_RX) {
            pbio_event_uart_rx_data_t *rx = data;
            handle_stdin_char(rx->byte);
        } else if (ev == PBIO_EVENT_COM_CMD) {
            pbio_com_cmd_t cmd = (uint32_t)data;

            switch (cmd) {
                case PBIO_COM_CMD_START_USER_PROGRAM:
                    break;
                case PBIO_COM_CMD_STOP_USER_PROGRAM:
                    if (user_stop_func) {
                        user_stop_func();
                    }
                    break;
            }
        }
        IWDG->KR = 0xaaaa;
    }

    PROCESS_END();
}
