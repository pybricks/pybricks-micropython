// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_GPIO

#include <stddef.h>

#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/busy_count.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/os.h>

#include "button_gpio.h"

/**
 * Reads the current button state.
 * @returns Flags that reflect the current button state.
 */
static pbio_button_flags_t pbdrv_button_gpio_read(void) {
    pbio_button_flags_t flags = 0;

    for (int i = 0; i < PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON; i++) {
        const pbdrv_button_gpio_platform_t *platform = &pbdrv_button_gpio_platform[i];

        if (!!pbdrv_gpio_input(&platform->gpio) ^ platform->active_low) {
            flags |= platform->button;
        }
    }

    return flags;
}

#if PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE

static pbio_button_flags_t pbdrv_button_state;

static pbio_os_process_t pbdrv_button_process;

pbio_error_t pbdrv_button_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    static pbio_button_flags_t prev, next;

    PBIO_OS_ASYNC_BEGIN(state);

    pbio_os_timer_set(&timer, 10);

    // Loop until cancellation is requested on power off and buttons are released.
    while (!pbdrv_button_process.request || pbdrv_button_state) {

        PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&timer));

        next = pbdrv_button_gpio_read();

        // when we get the same state twice in a row, consider it "good"
        if (next == prev) {
            pbdrv_button_state = next;
        }

        prev = next;

        pbio_os_timer_extend(&timer);
    }

    // Wait a while after release to prevent accidental power on.
    PBIO_OS_AWAIT_MS(state, &timer, 200);
    pbio_busy_count_down();

    PBIO_OS_ASYNC_END(PBIO_ERROR_CANCELED);
}

#endif // PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE

pbio_button_flags_t pbdrv_button_get_pressed(void) {
    #if PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE
    return pbdrv_button_state;
    #else
    return pbdrv_button_gpio_read();
    #endif
}

void pbdrv_button_init(void) {
    for (int i = 0; i < PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON; i++) {
        const pbdrv_button_gpio_platform_t *platform = &pbdrv_button_gpio_platform[i];
        pbdrv_gpio_set_pull(&platform->gpio, platform->pull);
        pbdrv_gpio_input(&platform->gpio);
    }

    #if PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE
    pbio_os_process_start(&pbdrv_button_process, pbdrv_button_process_thread, NULL);
    #endif
}

void pbdrv_button_deinit(void) {
    #if PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE
    pbio_busy_count_up();
    pbio_os_process_make_request(&pbdrv_button_process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);
    #endif
}

#endif // PBDRV_CONFIG_BUTTON_GPIO
