// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/light.h>

#include "../src/light/color_light.h"

#define NUM_LEDS 4

static FILE *brightness_sysfs_attr[NUM_LEDS];
static pbio_color_light_t ev3dev_status_light_instance;
pbio_color_light_t *ev3dev_status_light = &ev3dev_status_light_instance;

static void ev3dev_status_light_set_trigger() {
    static bool is_set = false;

    // only need to set trigger once
    if (is_set) {
        return;
    }

    static const char *const trigger_paths[NUM_LEDS] = {
        "/sys/class/leds/led0:red:brick-status/trigger",
        "/sys/class/leds/led1:red:brick-status/trigger",
        "/sys/class/leds/led0:green:brick-status/trigger",
        "/sys/class/leds/led1:green:brick-status/trigger"
    };

    for (int i = 0; i < NUM_LEDS; i++) {
        FILE *trigger_sysfs_attr = fopen(trigger_paths[i], "w");
        if (!trigger_sysfs_attr) {
            continue;
        }
        fprintf(trigger_sysfs_attr, "none");
        fclose(trigger_sysfs_attr);
    }

    is_set = true;
}

static pbio_error_t ev3dev_status_light_set_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv) {
    ev3dev_status_light_set_trigger();

    pbio_color_rgb_t rgb;
    pbio_color_hsv_to_rgb(hsv, &rgb);

    // FIXME: need to adjust for chromacity to get better orange/yellow

    for (int i = 0; i < NUM_LEDS; i++) {
        if (!brightness_sysfs_attr[i]) {
            continue;
        }

        int ret = fseek(brightness_sysfs_attr[i], 0, SEEK_SET);
        if (ret == -1) {
            return PBIO_ERROR_IO;
        }

        ret = fprintf(brightness_sysfs_attr[i], "%d", i < 2 ? rgb.r : rgb.g);
        if (ret < 0) {
            return PBIO_ERROR_IO;
        }
    }

    return PBIO_SUCCESS;
}

static const pbio_color_light_funcs_t ev3dev_status_light_funcs = {
    .set_hsv = ev3dev_status_light_set_hsv,
};

void ev3dev_status_light_init() {
    static const char *const brightness_paths[NUM_LEDS] = {
        "/sys/class/leds/led0:red:brick-status/brightness",
        "/sys/class/leds/led1:red:brick-status/brightness",
        "/sys/class/leds/led0:green:brick-status/brightness",
        "/sys/class/leds/led1:green:brick-status/brightness"
    };

    for (int i = 0; i < NUM_LEDS; i++) {
        brightness_sysfs_attr[i] = fopen(brightness_paths[i], "w");
        if (!brightness_sysfs_attr[i]) {
            continue;
        }
        setbuf(brightness_sysfs_attr[i], NULL);
    }

    pbio_color_light_init(ev3dev_status_light, &ev3dev_status_light_funcs);
}
