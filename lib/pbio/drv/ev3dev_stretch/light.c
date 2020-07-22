// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LIGHT

#include <stdio.h>

#include <pbdrv/light.h>
#include <pbio/color.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

#define NLEDS 4

static FILE *f_brightness[NLEDS];

void _pbdrv_light_init(void) {

    const char *const trigger_paths[NLEDS] = {
        "/sys/class/leds/led0:red:brick-status/trigger",
        "/sys/class/leds/led1:red:brick-status/trigger",
        "/sys/class/leds/led0:green:brick-status/trigger",
        "/sys/class/leds/led1:green:brick-status/trigger"
    };

    const char *const brightness_paths[NLEDS] = {
        "/sys/class/leds/led0:red:brick-status/brightness",
        "/sys/class/leds/led1:red:brick-status/brightness",
        "/sys/class/leds/led0:green:brick-status/brightness",
        "/sys/class/leds/led1:green:brick-status/brightness"
    };

    for (int led = 0; led < NLEDS; led++) {
        FILE *f_trigger = fopen(trigger_paths[led], "w");
        if (!f_trigger) {
            continue;
        }
        fprintf(f_trigger, "none");
        fclose(f_trigger);
        f_brightness[led] = fopen(brightness_paths[led], "w");
        if (f_brightness[led]) {
            setbuf(f_brightness[led], NULL);
        }
    }
}

#if PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_light_deinit(void) {
    for (int led = 0; led < NLEDS; led++) {
        if (f_brightness[led]) {
            fclose(f_brightness[led]);
        }
    }
}
#endif

pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, const pbdrv_light_raw_rgb_t *raw) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }
    for (int led = 0; led < NLEDS; led++) {
        if (f_brightness[led]) {
            int ret;

            ret = fseek(f_brightness[led], 0, SEEK_SET);
            if (ret == -1) {
                return PBIO_ERROR_IO;
            }
            ret = fprintf(f_brightness[led], "%d", led < 2 ? raw->r : raw->g);
            if (ret < 0) {
                return PBIO_ERROR_IO;
            }
        }
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_color_t color,
    pbdrv_light_raw_rgb_t *raw) {

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbio_color_rgb_t rgb;
    pbio_color_to_rgb(color, &rgb);

    // Adjust for chromacity
    uint32_t r = rgb.r * 1000;
    uint32_t g = rgb.g * 270;
    uint32_t b = rgb.b * 200;

    // Adjust for apparent brightness
    // + 1 protects against division by zero
    uint32_t Y = ((174 * r + 1590 * g + 327 * b) >> 16) + 1;
    raw->r = r / Y;
    raw->g = g / Y;
    raw->b = b / Y;

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_LIGHT
