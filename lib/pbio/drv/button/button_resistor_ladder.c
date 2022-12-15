// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Button driver using resistor ladder.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_RESISTOR_LADDER

#include <pbdrv/resistor_ladder.h>
#include <pbio/button.h>
#include <pbio/error.h>

void pbdrv_button_init(void) {
}

pbio_error_t pbdrv_button_is_pressed(pbio_button_flags_t *pressed) {
    pbdrv_resistor_ladder_ch_flags_t flags;
    pbio_error_t err;

    *pressed = 0;

    // REVISIT: For now, this is hard-coded for SPIKE Prime. If more platforms
    // that use this are added, a resistor ladder flags to buttons map will
    // need to be added to platform data.

    err = pbdrv_resistor_ladder_get(0, &flags);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (flags & PBDRV_RESISTOR_LADDER_CH_1) {
        *pressed |= PBIO_BUTTON_CENTER;
    }

    err = pbdrv_resistor_ladder_get(1, &flags);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (flags & PBDRV_RESISTOR_LADDER_CH_0) {
        *pressed |= PBIO_BUTTON_LEFT;
    }

    if (flags & PBDRV_RESISTOR_LADDER_CH_1) {
        *pressed |= PBIO_BUTTON_RIGHT;
    }

    if (flags & PBDRV_RESISTOR_LADDER_CH_2) {
        *pressed |= PBIO_BUTTON_RIGHT_UP; // Bluetooth
    }

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BUTTON_RESISTOR_LADDER
