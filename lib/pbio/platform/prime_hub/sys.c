// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <string.h>

#include "pbdrv/config.h"
#include "pbdrv/light.h"

#include "pbio/button.h"
#include "pbio/event.h"
#include "pbio/light.h"
#include "pbio/motorpoll.h"

#include "pbsys/sys.h"

#include "sys/clock.h"
#include "sys/etimer.h"
#include "sys/process.h"

#define USE_HAL_DRIVER
#include "stm32f4xx.h"

// workaround upstream NVIC_SystemReset() not decorated with noreturn
void NVIC_SystemReset(void) __attribute__((noreturn));

typedef enum {
    LED_STATUS_BUTTON_PRESSED   = 1 << 0,
} led_status_flags_t;

// Bitmask of status indicators
static led_status_flags_t led_status_flags;

// values for keeping track of how long button has been pressed
static bool button_pressed;
static clock_time_t button_press_start_time;

// user program stop function
static pbsys_stop_callback_t user_stop_func;
// user program stdin event function
static pbsys_stdin_event_callback_t user_stdin_event_func;

PROCESS(pbsys_process, "System");

void pbsys_prepare_user_program(const pbsys_user_program_callbacks_t *callbacks) {
    if (callbacks) {
        user_stop_func = callbacks->stop;
        user_stdin_event_func = callbacks->stdin_event;
    } else {
        user_stop_func = NULL;
        user_stdin_event_func = NULL;
    }
    _pbio_light_set_user_mode(true);
    pbio_light_on_with_pattern(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_GREEN, PBIO_LIGHT_PATTERN_BREATHE);
}

void pbsys_unprepare_user_program(void) {
    user_stop_func = NULL;
    user_stdin_event_func = NULL;
    _pbio_light_set_user_mode(false);
    pbdrv_light_raw_rgb_t raw;
    pbdrv_light_get_rgb_for_color(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_BLUE, &raw);
    pbdrv_light_set_rgb(PBIO_PORT_SELF, &raw);
    _pbio_motorpoll_reset_all();
}

void pbsys_reboot(bool fw_update) {
    // this function never returns
    NVIC_SystemReset();
}

void pbsys_power_off(void) {
    int i;

    // blink pattern like LEGO firmware
    for (i = 0; i < 3; i++) {
        pbdrv_light_raw_rgb_t raw;
        pbdrv_light_get_rgb_for_color(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_WHITE, &raw);
        pbdrv_light_set_rgb(PBIO_PORT_SELF, &raw);
        clock_delay_usec(50000);
        pbdrv_light_get_rgb_for_color(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_NONE, &raw);
        pbdrv_light_set_rgb(PBIO_PORT_SELF, &raw);
        clock_delay_usec(30000);
    }

    // PWM doesn't work while IRQs are disabled? so this needs to be after
    __disable_irq();

    // need to loop because power will stay on as long as button is pressed
    while (true) {
        GPIOA->BSRR = GPIO_BSRR_BR_13;
    }
}

static void init(void) {
    _pbio_light_set_user_mode(false);
    pbdrv_light_raw_rgb_t raw;
    pbdrv_light_get_rgb_for_color(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_BLUE, &raw);
    pbdrv_light_set_rgb(PBIO_PORT_SELF, &raw);
}

static void update_button(clock_time_t now) {
    pbio_button_flags_t btn;

    pbio_button_is_pressed(&btn);

    if (btn & PBIO_BUTTON_CENTER) {
        if (button_pressed) {

            // if the button is held down for 5 seconds, power off
            if (now - button_press_start_time > clock_from_msec(5000)) {
                // turn off light briefly like official LEGO firmware
                pbdrv_light_raw_rgb_t raw;
                pbdrv_light_get_rgb_for_color(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_NONE, &raw);
                pbdrv_light_set_rgb(PBIO_PORT_SELF, &raw);
                for (int i = 0; i < 10; i++) {
                    clock_delay_usec(58000);
                }

                pbsys_power_off();
            }
        } else {
            button_press_start_time = now;
            button_pressed = true;
            led_status_flags |= LED_STATUS_BUTTON_PRESSED;
            if (user_stop_func) {
                user_stop_func();
            }
        }
    } else {
        button_pressed = false;
        led_status_flags &= ~LED_STATUS_BUTTON_PRESSED;
    }
}

PROCESS_THREAD(pbsys_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    init();
    etimer_set(&timer, clock_from_msec(50));

    while (true) {
        PROCESS_WAIT_EVENT();
        if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            clock_time_t now = clock_time();
            etimer_reset(&timer);
            update_button(now);
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
