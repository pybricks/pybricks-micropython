// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_NXT

#include <pbio/button.h>
#include <pbio/config.h>
#include <pbio/error.h>

#include <nxt/nxt_avr.h>

void _pbdrv_button_init(void) {
}

pbio_error_t pbdrv_button_is_pressed(pbio_button_flags_t *pressed) {
    uint32_t buttons;

    buttons = buttons_get();

    *pressed = 0;
    if (buttons & BUTTON_ENTER) {
        *pressed |= PBIO_BUTTON_CENTER;
    }
    if (buttons & BUTTON_LEFT) {
        *pressed |= PBIO_BUTTON_LEFT;
    }
    if (buttons & BUTTON_RIGHT) {
        *pressed |= PBIO_BUTTON_RIGHT;
    }
    if (buttons & BUTTON_ESCAPE) {
        *pressed |= PBIO_BUTTON_DOWN;
    }

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BUTTON_NXT
