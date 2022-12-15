// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_NXT

#include <pbio/button.h>
#include <pbio/config.h>
#include <pbio/error.h>

#include <base/drivers/avr.h>

void pbdrv_button_init(void) {
}

pbio_error_t pbdrv_button_is_pressed(pbio_button_flags_t *pressed) {

    nx_avr_button_t button = nx_avr_get_button();

    *pressed = 0;

    if (button == BUTTON_OK) {
        *pressed |= PBIO_BUTTON_CENTER;
    }
    if (button == BUTTON_LEFT) {
        *pressed |= PBIO_BUTTON_LEFT;
    }
    if (button == BUTTON_RIGHT) {
        *pressed |= PBIO_BUTTON_RIGHT;
    }
    if (button == BUTTON_CANCEL) {
        *pressed |= PBIO_BUTTON_DOWN;
    }

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BUTTON_NXT
