// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Button driver using resistor ladder.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_RESISTOR_LADDER

#include <stddef.h>

#include <pbdrv/resistor_ladder.h>

#include <pbio/busy_count.h>
#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/os.h>

static pbio_os_process_t pbdrv_button_init_process;

/**
 * Wait for the buttons to get ready before giving control to system code.
 *
 * The resistor ladders give unknown values for 10s of milliseconds after boot.
 *
 * This ensures that system level functions such as the UI don't need hacks to
 * wait for buttons to start working.
 */
pbio_error_t pbdrv_button_init_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;
    pbdrv_resistor_ladder_ch_flags_t flags;

    PBIO_OS_ASYNC_BEGIN(state);

    while (pbdrv_resistor_ladder_get(0, &flags) != PBIO_SUCCESS || pbdrv_resistor_ladder_get(1, &flags) != PBIO_SUCCESS) {
        PBIO_OS_AWAIT_MS(state, &timer, 10);
    }
    PBIO_OS_AWAIT_MS(state, &timer, 30);

    pbio_busy_count_down();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_button_init(void) {
    pbio_busy_count_up();
    pbio_os_process_start(&pbdrv_button_init_process, pbdrv_button_init_process_thread, NULL);
}

pbio_button_flags_t pbdrv_button_get_pressed(void) {
    pbdrv_resistor_ladder_ch_flags_t flags;
    pbio_error_t err;

    pbio_button_flags_t pressed = 0;

    // REVISIT: For now, this is hard-coded for SPIKE Prime. If more platforms
    // that use this are added, a resistor ladder flags to buttons map will
    // need to be added to platform data.

    err = pbdrv_resistor_ladder_get(0, &flags);
    if (err != PBIO_SUCCESS) {
        return pressed;
    }

    if (flags & PBDRV_RESISTOR_LADDER_CH_1) {
        pressed |= PBIO_BUTTON_CENTER;
    }

    err = pbdrv_resistor_ladder_get(1, &flags);
    if (err != PBIO_SUCCESS) {
        return pressed;
    }

    if (flags & PBDRV_RESISTOR_LADDER_CH_0) {
        pressed |= PBIO_BUTTON_LEFT;
    }

    if (flags & PBDRV_RESISTOR_LADDER_CH_1) {
        pressed |= PBIO_BUTTON_RIGHT;
    }

    if (flags & PBDRV_RESISTOR_LADDER_CH_2) {
        pressed |= PBIO_BUTTON_RIGHT_UP; // Bluetooth
    }

    return pressed;
}

#endif // PBDRV_CONFIG_BUTTON_RESISTOR_LADDER
