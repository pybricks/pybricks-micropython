// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors
//
// Display driver for a Virtual Hub

#include <pbdrv/config.h>

#if PBDRV_CONFIG_DISPLAY_VIRTUAL

#include <stdbool.h>
#include <string.h>

#include <pbdrv/display.h>

#include <pbio/image.h>
#include <pbio/os.h>

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
 * Number of display updates so far. Readers of the frame buffer compare this
 * to a previously read value to know whether the buffer changed.
 */
static uint32_t pbdrv_display_update_count;

/**
 * Initialize the display driver.
 */
void pbdrv_display_init(void) {

    // Clear display to start with.
    memset(&pbdrv_display_user_frame, 0, sizeof(pbdrv_display_user_frame));

    // Initialize image.
    pbio_image_init(&display_image, (uint8_t *)pbdrv_display_user_frame,
        PBDRV_CONFIG_DISPLAY_NUM_COLS, PBDRV_CONFIG_DISPLAY_NUM_ROWS,
        PBDRV_CONFIG_DISPLAY_NUM_COLS);
    display_image.print_font = &pbio_font_terminus_normal_16;
    display_image.print_value = 3;

    // Count the initial cleared frame as an update so readers pick it up.
    pbdrv_display_update_count = 1;
}

pbio_image_t *pbdrv_display_get_image(void) {
    return &display_image;
}

uint32_t pbdrv_display_get_telemetry_data(uint8_t *buffer, uint32_t *location) {
    static uint32_t chunk_index;
    static uint32_t reading_count;
    static uint32_t sent_count;
    static bool frame_just_completed;

    // Yield once between frames so other telemetry gets a turn.
    if (frame_just_completed) {
        frame_just_completed = false;
        return 0;
    }

    if (chunk_index == 0) {
        if (pbdrv_display_update_count == sent_count) {
            // Nothing new to send.
            return 0;
        }
        reading_count = pbdrv_display_update_count;
    }

    // Pixels are 2 bits each, so packed 4 per byte, LSB first.
    const uint8_t *pixels = (const uint8_t *)pbdrv_display_user_frame;
    uint32_t packed_size = sizeof(pbdrv_display_user_frame) / 4;
    uint32_t offset = chunk_index * PBDRV_DISPLAY_TELEMETRY_MAX_SIZE;
    uint32_t copy_size = packed_size - offset;
    if (copy_size > PBDRV_DISPLAY_TELEMETRY_MAX_SIZE) {
        copy_size = PBDRV_DISPLAY_TELEMETRY_MAX_SIZE;
    }
    for (uint32_t i = 0; i < copy_size; i++) {
        const uint8_t *p = &pixels[(offset + i) * 4];
        buffer[i] = (p[0] & 0x03) | ((p[1] & 0x03) << 2) | ((p[2] & 0x03) << 4) | ((p[3] & 0x03) << 6);
    }
    *location = chunk_index;

    if (offset + copy_size == packed_size) {
        // Frame complete. If it was updated (torn) while reading, the count
        // differs from the snapshot, so a fresh frame follows.
        sent_count = reading_count;
        chunk_index = 0;
        frame_just_completed = true;
    } else {
        chunk_index++;
    }
    return copy_size;
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
    pbdrv_display_update_count++;
    pbio_os_request_poll();
}

void pbdrv_display_deinit(void) {
}

#endif // PBDRV_CONFIG_DISPLAY_VIRTUAL
