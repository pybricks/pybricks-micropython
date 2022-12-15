// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_GPIO

#include <contiki.h>

#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/config.h>
#include <pbio/error.h>

#include "button_gpio.h"

#if PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE

PROCESS(pbdrv_button_process, "button");

static pbio_button_flags_t pbdrv_button_state;

#endif // PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE

void pbdrv_button_init(void) {
    for (int i = 0; i < PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON; i++) {
        const pbdrv_button_gpio_platform_t *platform = &pbdrv_button_gpio_platform[i];
        pbdrv_gpio_set_pull(&platform->gpio, platform->pull);
        pbdrv_gpio_input(&platform->gpio);
    }

    #if PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE
    process_start(&pbdrv_button_process);
    #endif
}

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


pbio_error_t pbdrv_button_is_pressed(pbio_button_flags_t *pressed) {
    #if PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE
    *pressed = pbdrv_button_state;
    #else
    *pressed = pbdrv_button_gpio_read();
    #endif

    return PBIO_SUCCESS;
}

#if PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE

PROCESS_THREAD(pbdrv_button_process, ev, data) {
    static struct etimer timer;
    static pbio_button_flags_t prev, next;

    PROCESS_BEGIN();

    etimer_set(&timer, 10);

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

        next = pbdrv_button_gpio_read();

        // when we get the same state twice in a row, consider it "good"
        if (next == prev) {
            pbdrv_button_state = next;
        }

        prev = next;

        etimer_reset(&timer);
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE

#endif // PBDRV_CONFIG_BUTTON_GPIO
