// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors
//
// Display driver for a Virtual Hub

#include <pbdrv/config.h>

#if PBDRV_CONFIG_DISPLAY_VIRTUAL

#include <stdio.h>
#include <string.h>

#include <pbdrv/display.h>

#include <pbio/error.h>
#include <pbio/image.h>
#include <pbio/os.h>

#include "../rproc/rproc_virtual.h"

/**
 * User frame buffer. Each value is one pixel with value:
 *
 *  0: Empty / White
 *  1: Light Grey
 *  2: Dark Grey
 *  3: Black
 *
 * Non-atomic updated by the application are allowed.
 */
static uint8_t pbdrv_display_user_frame[PBDRV_CONFIG_DISPLAY_NUM_ROWS][PBDRV_CONFIG_DISPLAY_NUM_COLS] __attribute__((section(".noinit"), used));

/**
 * Image corresponding to the display.
 */
static pbio_image_t display_image;

/**
 * Flag to indicate that the user frame has been updated and needs to be
 * encoded and sent to the display driver.
 */
static bool pbdrv_display_user_frame_update_requested;

static pbio_os_process_t pbdrv_display_virtual_process;

/**
 * Display driver process. Initializes the display and updates the display
 * with the user frame buffer if the user data was updated.
 */
static pbio_error_t pbdrv_display_virtual_process_thread(pbio_os_state_t *state, void *context) {

    PBIO_OS_ASYNC_BEGIN(state);

    // Clear display to start with.
    memset(&pbdrv_display_user_frame, 0, sizeof(pbdrv_display_user_frame));
    pbdrv_display_user_frame_update_requested = true;

    // Update the display with the user frame buffer, if changed.
    for (;;) {
        PBIO_OS_AWAIT_UNTIL(state, pbdrv_display_user_frame_update_requested);
        pbdrv_display_user_frame_update_requested = false;
        pbdrv_rproc_virtual_socket_send(display_image.pixels, sizeof(pbdrv_display_user_frame));
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Initialize the display driver.
 */
void pbdrv_display_init(void) {

    // Initialize image.
    pbio_image_init(&display_image, (uint8_t *)pbdrv_display_user_frame,
        PBDRV_CONFIG_DISPLAY_NUM_COLS, PBDRV_CONFIG_DISPLAY_NUM_ROWS,
        PBDRV_CONFIG_DISPLAY_NUM_COLS);
    display_image.print_font = &pbio_font_terminus_normal_16;
    display_image.print_value = 3;

    pbio_os_process_start(&pbdrv_display_virtual_process, pbdrv_display_virtual_process_thread, NULL);
}

pbio_image_t *pbdrv_display_get_image(void) {
    return &display_image;
}

uint8_t pbdrv_display_get_max_value(void) {
    return 3;
}

uint8_t pbdrv_display_get_value_from_hsv(uint16_t h, uint8_t s, uint8_t v) {
    // Method to compute those values: see EV3 driver.
    static const uint16_t splits_x100[] = { 2105, 5526, 8421 };
    uint16_t l_x100 = v * (100 - s / 2);
    if (l_x100 < splits_x100[1]) {
        if (l_x100 < splits_x100[0]) {
            return 3;
        } else {
            return 2;
        }
    } else {
        if (l_x100 < splits_x100[2]) {
            return 1;
        } else {
            return 0;
        }
    }
}

void pbdrv_display_update(void) {
    pbdrv_display_user_frame_update_requested = true;
    pbio_os_request_poll();
}

void pbdrv_display_deinit(void) {
}

#endif // PBDRV_CONFIG_DISPLAY_VIRTUAL
