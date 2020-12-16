// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <contiki.h>

#include <pbdrv/led.h>
#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/event.h>
#include <pbio/light.h>
#include <pbio/util.h>
#include <pbsys/config.h>
#include <pbsys/status.h>

#include "../src/light/color_light.h"

#if PBSYS_CONFIG_STATUS_LIGHT

typedef enum {
    PBSYS_STATUS_LIGHT_INDICATION_NONE,
    PBSYS_STATUS_LIGHT_INDICATION_HIGH_CURRENT,
    PBSYS_STATUS_LIGHT_INDICATION_LOW_VOLTAGE,
    PBSYS_STATUS_LIGHT_INDICATION_BLE_ADVERTISING,
    PBSYS_STATUS_LIGHT_INDICATION_BLE_ADVERTISING_AND_LOW_VOLTAGE,
    PBSYS_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL,
    PBSYS_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL_AND_LOW_VOLTAGE,
} pbsys_status_light_indication_t;

/** A single element of a status light indication pattern. */
typedef struct {
    /** Color to display or ::PBIO_COLOR_NONE for system/user color. */
    pbio_color_t color;
    /** Duration to display color in milliseconds divided by poll interval (50 ms) */
    uint8_t duration;
} pbsys_status_light_indication_pattern_element_t;

/** Sentinel value for status light indication patterns. */
#define PBSYS_STATUS_LIGHT_INDICATION_PATTERN_END { .duration = 0 }

// Most indications patterns are selected to match the official LEGO firmware
// with the exception that the BLE advertising color is blue in Pybricks instead
// of white so that users can known which firmware is loaded when the hub is
// powered on. Timing is slightly different in some cases since we use 50 ms
// interval instead of 10 ms. Patterns are rotated compared to LEGO firmware
// so that we don't have the light off at the beginning of the pattern.
static const pbsys_status_light_indication_pattern_element_t *const
pbsys_status_light_indication_pattern[] = {
    [PBSYS_STATUS_LIGHT_INDICATION_HIGH_CURRENT] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_ORANGE, .duration = 1 },
        { .color = PBIO_COLOR_BLACK, .duration = 1 },
        { .color = PBIO_COLOR_ORANGE, .duration = 1 },
        { .color = PBIO_COLOR_BLACK, .duration = 1 },
        { .color = PBIO_COLOR_ORANGE, .duration = 1 },
        { .color = PBIO_COLOR_NONE, .duration = 22 },
        { .color = PBIO_COLOR_BLACK, .duration = 1 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_END
    },
    [PBSYS_STATUS_LIGHT_INDICATION_LOW_VOLTAGE] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_ORANGE, .duration = 6 },
        { .color = PBIO_COLOR_BLACK, .duration = 8 },
        { .color = PBIO_COLOR_ORANGE, .duration = 6 },
        { .color = PBIO_COLOR_BLACK, .duration = 4 },
        { .color = PBIO_COLOR_NONE, .duration = 16 },
        { .color = PBIO_COLOR_BLACK, .duration = 4 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_END
    },
    [PBSYS_STATUS_LIGHT_INDICATION_BLE_ADVERTISING] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_BLUE, .duration = 1 },
        { .color = PBIO_COLOR_BLACK, .duration = 2 },
        { .color = PBIO_COLOR_BLUE, .duration = 1 },
        { .color = PBIO_COLOR_BLACK, .duration = 22 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_END
    },
    [PBSYS_STATUS_LIGHT_INDICATION_BLE_ADVERTISING_AND_LOW_VOLTAGE] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_ORANGE, .duration = 1 },
        { .color = PBIO_COLOR_BLACK, .duration = 2 },
        { .color = PBIO_COLOR_ORANGE, .duration = 1 },
        { .color = PBIO_COLOR_BLACK, .duration = 22 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_END
    },
    [PBSYS_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_NONE, .duration = 8 },
        { .color = PBIO_COLOR_WHITE, .duration = 1 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_END
    },
    [PBSYS_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL_AND_LOW_VOLTAGE] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_NONE, .duration = 8 },
        { .color = PBIO_COLOR_BLACK, .duration = 4 },
        { .color = PBIO_COLOR_ORANGE, .duration = 6 },
        { .color = PBIO_COLOR_BLACK, .duration = 8 },
        { .color = PBIO_COLOR_ORANGE, .duration = 6 },
        { .color = PBIO_COLOR_BLACK, .duration = 4 },
        { .color = PBIO_COLOR_NONE, .duration = 8 },
        { .color = PBIO_COLOR_WHITE, .duration = 1 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_END
    },
};

typedef struct {
    /** Color light struct for PBIO light implementation. */
    pbio_color_light_t color_light;
    /** The most recent user color value. */
    pbio_color_hsv_t user_color;
    /** The user program is currently allowed to control the status light. */
    bool allow_user_update;
    /** The current status light indication. */
    pbsys_status_light_indication_t indication;
    /** The current index in the current status light indication pattern. */
    uint8_t pattern_index;
    /** The current duration count in the current status light indication pattern. */
    uint8_t pattern_count;
} pbsys_status_light_t;

static pbsys_status_light_t pbsys_status_light_instance;

/** The system status light instance. */
pbio_color_light_t *pbsys_status_light = &pbsys_status_light_instance.color_light;

static pbio_error_t pbsys_status_light_set_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv) {
    pbsys_status_light_t *instance = PBIO_CONTAINER_OF(light, pbsys_status_light_t, color_light);
    instance->user_color = *hsv;
    if (instance->allow_user_update) {
        // FIXME: currently system status light is hard-coded as LED at index 0
        // on all platforms
        pbdrv_led_dev_t *led;
        if (pbdrv_led_get_dev(0, &led) == PBIO_SUCCESS) {
            return pbdrv_led_set_hsv(led, hsv);
        }
    }

    return PBIO_SUCCESS;
}

static const pbio_color_light_funcs_t pbsys_status_light_funcs = {
    .set_hsv = pbsys_status_light_set_hsv,
};

void pbsys_status_light_init(void) {
    pbio_color_light_init(pbsys_status_light, &pbsys_status_light_funcs);
}

static void pbsys_status_light_handle_status_change(void) {
    pbsys_status_light_t *instance = &pbsys_status_light_instance;
    pbsys_status_light_indication_t new_indication = PBSYS_STATUS_LIGHT_INDICATION_NONE;
    bool ble_advertising = pbsys_status_test(PBSYS_STATUS_BLE_ADVERTISING);
    bool ble_low_signal = pbsys_status_test(PBSYS_STATUS_BLE_LOW_SIGNAL);
    bool low_voltage = pbsys_status_test(PBSYS_STATUS_BATTERY_LOW_VOLTAGE_WARNING);
    bool high_current = pbsys_status_test(PBSYS_STATUS_BATTERY_HIGH_CURRENT);

    // This determines which indication has the highest precedence.
    if (ble_advertising && low_voltage) {
        new_indication = PBSYS_STATUS_LIGHT_INDICATION_BLE_ADVERTISING_AND_LOW_VOLTAGE;
    } else if (ble_advertising) {
        new_indication = PBSYS_STATUS_LIGHT_INDICATION_BLE_ADVERTISING;
    } else if (high_current) {
        new_indication = PBSYS_STATUS_LIGHT_INDICATION_HIGH_CURRENT;
    } else if (ble_low_signal && low_voltage) {
        new_indication = PBSYS_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL_AND_LOW_VOLTAGE;
    } else if (ble_low_signal) {
        new_indication = PBSYS_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL;
    } else if (low_voltage) {
        new_indication = PBSYS_STATUS_LIGHT_INDICATION_LOW_VOLTAGE;
    }

    // if the indication changed, then reset the indication pattern to the beginning
    if (instance->indication != new_indication) {
        instance->indication = new_indication;
        instance->pattern_index = 0;
        instance->pattern_count = 0;
    }
}

static clock_time_t default_user_program_light_animation_next(pbio_light_animation_t *animation) {
    // The brightness pattern has the form /\ through which we cycle in N steps.
    static uint8_t cycle = 0;
    const uint8_t cycle_max = 200;

    pbio_color_hsv_t hsv = {
        .h = PBIO_COLOR_HUE_BLUE,
        .s = 100,
        .v = cycle < cycle_max / 2 ? cycle : cycle_max - cycle,
    };

    pbsys_status_light->funcs->set_hsv(pbsys_status_light, &hsv);

    // This increment controls the speed of the pattern and wraps on completion
    cycle = (cycle + 4) % cycle_max;

    return clock_from_msec(40);
}

void pbsys_status_light_handle_event(process_event_t event, process_data_t data) {
    if (event == PBIO_EVENT_STATUS_SET || event == PBIO_EVENT_STATUS_CLEARED) {
        pbsys_status_light_handle_status_change();
    }
    if (event == PBIO_EVENT_STATUS_SET && (pbsys_status_t)data == PBSYS_STATUS_USER_PROGRAM_RUNNING) {
        pbio_light_animation_init(&pbsys_status_light->animation, default_user_program_light_animation_next);
        pbio_light_animation_start(&pbsys_status_light->animation);
    }
}

void pbsys_status_light_poll(void) {
    pbsys_status_light_t *instance = &pbsys_status_light_instance;
    const pbsys_status_light_indication_pattern_element_t *pattern =
        pbsys_status_light_indication_pattern[instance->indication];
    pbio_color_t new_color = PBIO_COLOR_NONE;

    if (pattern != NULL) {
        // if we are at the end of a pattern (indicated by duration == 0), wrap around to the beginning
        if (pattern[instance->pattern_index].duration == 0) {
            instance->pattern_index = 0;
            // count should already be set to 0 because of previous index increment
            assert(instance->pattern_count == 0);
        }

        new_color = pattern[instance->pattern_index].color;

        // if we have exceeded the pattern duration for the current index, move to the next index
        if (++instance->pattern_count >= pattern[instance->pattern_index].duration) {
            instance->pattern_index++;
            instance->pattern_count = 0;
        }
    }

    // If the new system indication is not overriding the status light and a user
    // program is running, then we can allow the user program to directly change
    // the status light.
    instance->allow_user_update =
        new_color == PBIO_COLOR_NONE && pbsys_status_test(PBSYS_STATUS_USER_PROGRAM_RUNNING);

    // FIXME: currently system status light is hard-coded as LED at index 0 on
    // all platforms
    pbdrv_led_dev_t *led;
    if (pbdrv_led_get_dev(0, &led) == PBIO_SUCCESS) {
        if (instance->allow_user_update) {
            pbdrv_led_set_hsv(led, &instance->user_color);
        } else {
            if (new_color == PBIO_COLOR_NONE) {
                // System ID color
                new_color = PBIO_COLOR_BLUE;
            }
            pbio_color_hsv_t hsv;
            pbio_color_to_hsv(new_color, &hsv);
            pbdrv_led_set_hsv(led, &hsv);
        }
    }
}

#endif // PBSYS_CONFIG_STATUS_LIGHT
