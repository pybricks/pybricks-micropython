// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <stdbool.h>

#include <pbio/error.h>
#include <pbio/light.h>
#include <pbio/port.h>

#include <pbdrv/light.h>

#if PBDRV_CONFIG_LIGHT

typedef struct {
    pbio_light_pattern_t pattern;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} user_data_t;

static user_data_t user_light_data;

static bool user_mode_active = true;

static const uint8_t flash_pattern_data[8] = {
    0, 0, 0, 0, 1, 0, 1, 0
};

// breathe effect varying from 20% to 100% of 255 in a sort of exponential cosine
// round(((0.8*((2^(1-cos(2*π*n/64))-1)/3))+0.2)*255) where n = 0..63
static const uint8_t breathe_pattern_data[64] = {
    51, 51, 52, 53, 55, 57, 59, 63, 66, 71, 76, 81, 87, 94, 102, 110, 119, 129,
    139, 149, 160, 172, 183, 194, 205, 215, 225, 234, 241, 247, 251, 254, 255,
    254, 251, 247, 241, 234, 225, 215, 205, 194, 183, 172, 160, 149, 139, 129,
    119, 110, 102, 94, 87, 81, 76, 71, 66, 63, 59, 57, 55, 53, 52, 51
};

pbio_error_t _pbio_light_on(pbio_port_t port, pbio_light_color_t color, pbio_light_pattern_t pattern) {
    user_data_t data;
    pbio_error_t err;

    if (port != PBIO_PORT_SELF) {
        // TODO: handle lights on I/O ports
        return PBIO_ERROR_INVALID_PORT;
    }

    if (pattern > PBIO_LIGHT_PATTERN_BREATHE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    err = pbdrv_light_get_rgb_for_color(port, color, &data.r, &data.g, &data.b);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    data.pattern = pattern;

    user_light_data = data;

    return PBIO_SUCCESS;
}

void _pbio_light_poll(uint32_t now) {
    uint16_t scale;
    user_data_t data;
    uint8_t idx;

    if (!user_mode_active) {
        return;
    }

    data = user_light_data;

    switch (data.pattern) {
    case PBIO_LIGHT_PATTERN_NONE:
        break;
    case PBIO_LIGHT_PATTERN_BREATHE:
        // breathe pattern has 64 values over the course of two seconds (2048ms)
        idx = (now >> 5) & (64 - 1);
        scale = breathe_pattern_data[idx] + 1;
        data.r = data.r * scale / 256;
        data.g = data.g * scale / 256;
        data.b = data.b * scale / 256;
        break;
    case PBIO_LIGHT_PATTERN_FLASH:
        // flash pattern has 8 value over the course of two seconds (2048ms)
        idx = (now >> 8) & (8 - 1);
        scale = flash_pattern_data[idx] + 1;
        data.r = data.r * scale / 256;
        data.g = data.g * scale / 256;
        data.b = data.b * scale / 256;
        break;
    }

    pbdrv_light_set_rgb(PBIO_PORT_SELF, data.r, data.g, data.b);
}

/**
 * Enables or disables user mode. When user mode is enabled, all lights will
 * respond to user functions. When disabled, all lights will only respond to
 * system functions.
 * @param user_mode     *true* to enable user mode, otherwise *false*
 */
void _pbio_light_set_user_mode(bool user_mode) {
    user_mode_active = user_mode;
}

#endif // PBDRV_CONFIG_LIGHT
