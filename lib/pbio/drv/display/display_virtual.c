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

#include <pbsys/telemetry.h>

#include <lego/device.h>

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
}

pbio_image_t *pbdrv_display_get_image(void) {
    return &display_image;
}

// chunks of 8 packed rows, so we can send the whole screen in fixed size chunks.
#define PBDRV_DISPLAY_TELEMETRY_CHUNK_ROWS (8)
#define PBDRV_DISPLAY_TELEMETRY_CHUNK_SIZE (PBDRV_DISPLAY_TELEMETRY_CHUNK_ROWS * PBDRV_CONFIG_DISPLAY_NUM_COLS / 4)
#define PBDRV_DISPLAY_TELEMETRY_CHUNK_NUM (PBDRV_CONFIG_DISPLAY_NUM_ROWS / PBDRV_DISPLAY_TELEMETRY_CHUNK_ROWS)

pbsys_telemetry_error_t pbdrv_display_iterate_data(pbsys_telemetry_packet_t *tel, bool *done, uint32_t *size) {

    // Display update count when the current frame read-out started. Initialized
    // to differ from the update count so the first frame is always sent.
    static uint32_t last_update_count = UINT32_MAX;

    // Next chunk to send. Zero means between frames.
    static uint32_t chunk = 0;

    // Doesn't fit now. Caller retries this same chunk with a fresh buffer.
    if (*size < PBDRV_DISPLAY_TELEMETRY_CHUNK_SIZE) {
        return PBSYS_TELEMETRY_ERROR_NO_ROOM;
    }

    // Between frames, start reading out a new one only if it changed. Changes
    // during read-out are picked up by the next one, accepting that a read-out
    // may span parts of two frames.
    if (chunk == 0) {
        if (last_update_count == pbdrv_display_update_count) {
            *done = true;
            return PBSYS_TELEMETRY_ERROR_NO_REPORT;
        }
        last_update_count = pbdrv_display_update_count;
    }

    for (uint32_t i = 0; i < PBDRV_DISPLAY_TELEMETRY_CHUNK_SIZE; i++) {
        // Pack 4 consecutive pixels (2 bits each, LSB first) per byte.
        const uint8_t *p = (const uint8_t *)pbdrv_display_user_frame + (chunk * PBDRV_DISPLAY_TELEMETRY_CHUNK_SIZE + i) * 4;
        tel->payload[i] = (p[0] & 0x03) | ((p[1] & 0x03) << 2) | ((p[2] & 0x03) << 4) | ((p[3] & 0x03) << 6);
    }

    // Header with position indicating chunk progress.
    tel->manufacturer = PBSYS_TELEMETRY_MANUFACTURER_LEGO;
    tel->id = LEGO_DEVICE_TYPE_ID_EV3_DISPLAY;
    tel->location = chunk;
    tel->mode = 0;
    *size = PBDRV_DISPLAY_TELEMETRY_CHUNK_SIZE;

    chunk = (chunk + 1) % PBDRV_DISPLAY_TELEMETRY_CHUNK_NUM;
    *done = chunk == 0;
    return PBSYS_TELEMETRY_SUCCESS;
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
