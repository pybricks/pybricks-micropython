// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_EV3DEV

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <pbdrv/led.h>

#include <pbio/color.h>
#include <pbio/error.h>

#include "led_ev3dev.h"
#include "led.h"

#define NUM_LEDS 4

static FILE *brightness_sysfs_attr[NUM_LEDS];

static pbio_error_t pbdrv_led_ev3dev_set_hsv(pbdrv_led_dev_t *dev, const pbio_color_hsv_t *hsv) {
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

static const pbdrv_led_funcs_t pbdrv_led_ev3dev_funcs = {
    .set_hsv = pbdrv_led_ev3dev_set_hsv,
};

void pbdrv_led_ev3dev_init(pbdrv_led_dev_t *devs) {

    const char *const trigger_paths[NUM_LEDS] = {
        "/sys/class/leds/led0:red:brick-status/trigger",
        "/sys/class/leds/led1:red:brick-status/trigger",
        "/sys/class/leds/led0:green:brick-status/trigger",
        "/sys/class/leds/led1:green:brick-status/trigger"
    };

    const char *const brightness_paths[NUM_LEDS] = {
        "/sys/class/leds/led0:red:brick-status/brightness",
        "/sys/class/leds/led1:red:brick-status/brightness",
        "/sys/class/leds/led0:green:brick-status/brightness",
        "/sys/class/leds/led1:green:brick-status/brightness"
    };

    for (int i = 0; i < NUM_LEDS; i++) {
        FILE *trigger_sysfs_attr = fopen(trigger_paths[i], "w");
        if (!trigger_sysfs_attr) {
            continue;
        }
        fprintf(trigger_sysfs_attr, "none");
        fclose(trigger_sysfs_attr);

        brightness_sysfs_attr[i] = fopen(brightness_paths[i], "w");
        if (!brightness_sysfs_attr[i]) {
            continue;
        }
        setbuf(brightness_sysfs_attr[i], NULL);
    }

    devs[0].funcs = &pbdrv_led_ev3dev_funcs;
}

#endif // PBDRV_CONFIG_LED_EV3DEV
