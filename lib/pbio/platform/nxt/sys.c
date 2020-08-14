// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <contiki.h>

#include "pbdrv/bluetooth.h"
#include "pbdrv/config.h"
#include "pbdrv/reset.h"

#include "pbio/button.h"
#include "pbio/event.h"
#include "pbio/light.h"
#include "pbio/motorpoll.h"

#include <pbsys/battery.h>
#include <pbsys/sys.h>

typedef enum {
    LED_STATUS_BUTTON_PRESSED   = 1 << 0,
} led_status_flags_t;

// ring buffer size for stdin data - must be power of 2!
#define STDIN_BUF_SIZE 128

// Bitmask of status indicators
static led_status_flags_t led_status_flags;

// values for keeping track of how long button has been pressed
static bool button_pressed;
static clock_time_t button_press_start_time;

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
}

void pbsys_unprepare_user_program(void) {
    user_stop_func = NULL;
    user_stdin_event_func = NULL;
    // _pbio_motorpoll_reset_all();
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

static void update_button(clock_time_t now) {
    pbio_button_flags_t btn;

    pbio_button_is_pressed(&btn);

    // the orange center button acts as the power button
    if (btn & PBIO_BUTTON_CENTER) {
        if (button_pressed) {

            // if the button is held down for 5 seconds, power off
            if (now - button_press_start_time > clock_from_msec(5000)) {
                pbdrv_reset(PBDRV_RESET_ACTION_POWER_OFF);
            }
        } else {
            button_press_start_time = now;
            button_pressed = true;
            led_status_flags |= LED_STATUS_BUTTON_PRESSED;
        }
    } else {
        button_pressed = false;
        led_status_flags &= ~LED_STATUS_BUTTON_PRESSED;
    }

    // the dark gray button stops user programs
    if (btn & PBIO_BUTTON_DOWN && user_stop_func) {
        user_stop_func();
    }
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

    pbsys_battery_init();
    etimer_set(&timer, clock_from_msec(50));

    while (true) {
        PROCESS_WAIT_EVENT();
        if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            clock_time_t now = clock_time();
            etimer_reset(&timer);
            update_button(now);
            pbsys_battery_poll();
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
    }

    PROCESS_END();
}
