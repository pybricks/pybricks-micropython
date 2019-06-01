// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_GPIO

#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/config.h>
#include <pbio/error.h>

#include "button_gpio.h"

// PC13 is the green button (active low)

void _pbdrv_button_init(void) {
    for (int i = 0; i < PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON; i++) {
        const pbdrv_button_gpio_platform_t *platform = &pbdrv_button_gpio_platform[i];
        pbdrv_gpio_set_pull(&platform->gpio, platform->pull);
        pbdrv_gpio_input(&platform->gpio);
    }
}

#if PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_button_deinit(void) { }
#endif

pbio_error_t pbdrv_button_is_pressed(pbio_button_flags_t *pressed) {
    *pressed = 0;

    for (int i = 0; i < PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON; i++) {
        const pbdrv_button_gpio_platform_t *platform = &pbdrv_button_gpio_platform[i];
        if (!!pbdrv_gpio_input(&platform->gpio) ^ platform->active_low) {
            *pressed |= platform->button;
        }
    }

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BUTTON_GPIO
