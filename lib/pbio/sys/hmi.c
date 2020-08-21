// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user.

// TODO: implement user program start/stop from button
// TODO: implement additional buttons and Bluetooth light for SPIKE Prime
// TODO: implement additional buttons and menu system (via screen) for NXT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pbdrv/reset.h>
#include <pbdrv/led.h>
#include <pbio/button.h>
#include <pbio/color.h>
#include <pbio/light.h>
#include <pbsys/config.h>
#include <pbsys/status.h>

#if PBSYS_CONFIG_STATUS_LIGHT

typedef enum {
    PBSYS_HMI_STATUS_LIGHT_INDICATION_NONE,
    PBSYS_HMI_STATUS_LIGHT_INDICATION_HIGH_CURRENT,
    PBSYS_HMI_STATUS_LIGHT_INDICATION_LOW_VOLTAGE,
    PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_ADVERTISING,
    PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_ADVERTISING_AND_LOW_VOLTAGE,
    PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL,
    PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL_AND_LOW_VOLTAGE,
} pbsys_hmi_status_light_indication_t;

/** A single element of a status light indication pattern. */
typedef struct {
    /** Color to display or ::PBIO_COLOR_NONE for system/user color. */
    pbio_color_t color;
    /** Duration to display color in milliseconds divided by poll interval (50 ms) */
    uint8_t duration;
} pbsys_hmi_status_light_indication_pattern_element_t;

/** Sentinel value for status light indication patterns. */
#define PBSYS_HMI_STATUS_LIGHT_INDICATION_PATTERN_END { .duration = 0 }

// Most indications patterns are selected to match the official LEGO firmware
// with the exception that the BLE advertising color is blue in Pybricks instead
// of white so that users can known which firmware is loaded when the hub is
// powered on. Timing is slightly different in some cases since we use 50 ms
// interval instead of 10 ms.
static const pbsys_hmi_status_light_indication_pattern_element_t *const
pbsys_hmi_status_light_indication_pattern[] = {
    [PBSYS_HMI_STATUS_LIGHT_INDICATION_HIGH_CURRENT] =
        (const pbsys_hmi_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_BLACK, .duration = 1 },
        { .color = PBIO_COLOR_ORANGE, .duration = 1 },
        { .color = PBIO_COLOR_BLACK, .duration = 1 },
        { .color = PBIO_COLOR_ORANGE, .duration = 1 },
        { .color = PBIO_COLOR_BLACK, .duration = 1 },
        { .color = PBIO_COLOR_ORANGE, .duration = 1 },
        { .color = PBIO_COLOR_NONE, .duration = 22 },
        PBSYS_HMI_STATUS_LIGHT_INDICATION_PATTERN_END
    },
    [PBSYS_HMI_STATUS_LIGHT_INDICATION_LOW_VOLTAGE] =
        (const pbsys_hmi_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_BLACK, .duration = 4 },
        { .color = PBIO_COLOR_ORANGE, .duration = 6 },
        { .color = PBIO_COLOR_BLACK, .duration = 8 },
        { .color = PBIO_COLOR_ORANGE, .duration = 6 },
        { .color = PBIO_COLOR_BLACK, .duration = 4 },
        { .color = PBIO_COLOR_NONE, .duration = 16 },
        PBSYS_HMI_STATUS_LIGHT_INDICATION_PATTERN_END
    },
    [PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_ADVERTISING] =
        (const pbsys_hmi_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_BLACK, .duration = 22 },
        { .color = PBIO_COLOR_BLUE, .duration = 1 },
        { .color = PBIO_COLOR_BLACK, .duration = 2 },
        { .color = PBIO_COLOR_BLUE, .duration = 1 },
        PBSYS_HMI_STATUS_LIGHT_INDICATION_PATTERN_END
    },
    [PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_ADVERTISING_AND_LOW_VOLTAGE] =
        (const pbsys_hmi_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_BLACK, .duration = 22 },
        { .color = PBIO_COLOR_ORANGE, .duration = 1 },
        { .color = PBIO_COLOR_BLACK, .duration = 2 },
        { .color = PBIO_COLOR_ORANGE, .duration = 1 },
        PBSYS_HMI_STATUS_LIGHT_INDICATION_PATTERN_END
    },
    [PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL] =
        (const pbsys_hmi_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_WHITE, .duration = 1 },
        { .color = PBIO_COLOR_NONE, .duration = 8 },
        PBSYS_HMI_STATUS_LIGHT_INDICATION_PATTERN_END
    },
    [PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL_AND_LOW_VOLTAGE] =
        (const pbsys_hmi_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_WHITE, .duration = 1 },
        { .color = PBIO_COLOR_NONE, .duration = 8 },
        { .color = PBIO_COLOR_BLACK, .duration = 4 },
        { .color = PBIO_COLOR_ORANGE, .duration = 6 },
        { .color = PBIO_COLOR_BLACK, .duration = 8 },
        { .color = PBIO_COLOR_ORANGE, .duration = 6 },
        { .color = PBIO_COLOR_BLACK, .duration = 4 },
        { .color = PBIO_COLOR_NONE, .duration = 8 },
        PBSYS_HMI_STATUS_LIGHT_INDICATION_PATTERN_END
    },
};

static struct {
    /** The current status light indication. */
    pbsys_hmi_status_light_indication_t indication;
    /** The current index in the current status light indication pattern. */
    uint8_t pattern_index;
    /** The current duration count in the current status light indication pattern. */
    uint8_t pattern_count;
} pbsys_hmi_status_light;

static void pbsys_hmi_poll_status_light() {
    // REVISIT: it could be more efficient to do this only in response to a
    // status change event instead of every poll cycle
    pbsys_hmi_status_light_indication_t new_indication = PBSYS_HMI_STATUS_LIGHT_INDICATION_NONE;
    bool ble_advertising = pbsys_status_test(PBSYS_STATUS_BLE_ADVERTISING);
    bool ble_low_signal = pbsys_status_test(PBSYS_STATUS_BLE_LOW_SIGNAL);
    bool low_voltage = pbsys_status_test(PBSYS_STATUS_BATTERY_LOW_VOLTAGE_WARNING);
    bool high_current = pbsys_status_test(PBSYS_STATUS_BATTERY_HIGH_CURRENT);

    // This determines which indication has the highest precedence.
    if (ble_advertising && low_voltage) {
        new_indication = PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_ADVERTISING_AND_LOW_VOLTAGE;
    } else if (ble_advertising) {
        new_indication = PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_ADVERTISING;
    } else if (high_current) {
        new_indication = PBSYS_HMI_STATUS_LIGHT_INDICATION_HIGH_CURRENT;
    } else if (ble_low_signal && low_voltage) {
        new_indication = PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL_AND_LOW_VOLTAGE;
    } else if (ble_low_signal) {
        new_indication = PBSYS_HMI_STATUS_LIGHT_INDICATION_BLE_LOW_SIGNAL;
    } else if (low_voltage) {
        new_indication = PBSYS_HMI_STATUS_LIGHT_INDICATION_LOW_VOLTAGE;
    }

    // if the indication changed, then reset the indication pattern to the beginning
    if (pbsys_hmi_status_light.indication != new_indication) {
        pbsys_hmi_status_light.indication = new_indication;
        pbsys_hmi_status_light.pattern_index = 0;
        pbsys_hmi_status_light.pattern_count = 0;
    }

    const pbsys_hmi_status_light_indication_pattern_element_t *pattern =
        pbsys_hmi_status_light_indication_pattern[pbsys_hmi_status_light.indication];
    pbio_color_t new_color = PBIO_COLOR_NONE;

    if (pattern != NULL) {
        // if we are at the end of a pattern (indicated by duration == 0), wrap around to the beginning
        if (pattern[pbsys_hmi_status_light.pattern_index].duration == 0) {
            pbsys_hmi_status_light.pattern_index = 0;
            pbsys_hmi_status_light.pattern_count = 0;
        }

        new_color = pattern[pbsys_hmi_status_light.pattern_index].color;

        // if we have exceeded the pattern duration for the current index, move to the next index
        if (++pbsys_hmi_status_light.pattern_count >= pattern[pbsys_hmi_status_light.pattern_index].duration) {
            pbsys_hmi_status_light.pattern_index++;
            pbsys_hmi_status_light.pattern_count = 0;
        }
    }

    // FIXME: currently system status light is hard-coded as LED at index 0 on
    // all platforms
    pbdrv_led_dev_t *led;
    if (pbdrv_led_get_dev(0, &led) == PBIO_SUCCESS) {
        if (new_color == PBIO_COLOR_NONE && pbsys_status_test(PBSYS_STATUS_USER_PROGRAM_RUNNING)) {
            _pbio_light_set_user_mode(true);
            // TODO: get cached user color and set it here
        } else {
            _pbio_light_set_user_mode(false);
            if (new_color == PBIO_COLOR_NONE) {
                // System ID color
                new_color = PBIO_COLOR_BLUE;
            }
            pbdrv_led_on(led, new_color);
        }
    }
}

#endif // PBSYS_CONFIG_STATUS_LIGHT

/**
 * Polls the HMI.
 *
 * This is called periodically to update the current HMI state.
 */
void pbsys_hmi_poll() {
    pbio_button_flags_t btn;
    pbio_button_is_pressed(&btn);

    if (btn & PBIO_BUTTON_CENTER) {
        pbsys_status_set(PBSYS_STATUS_POWER_BUTTON_PRESSED);
        // power off when button is held down for 3 seconds
        if (pbsys_status_test_debounce(PBSYS_STATUS_POWER_BUTTON_PRESSED, true, 3000)) {
            pbdrv_reset(PBDRV_RESET_ACTION_POWER_OFF);
        }
    } else {
        pbsys_status_clear(PBSYS_STATUS_POWER_BUTTON_PRESSED);
    }

    #if PBSYS_CONFIG_STATUS_LIGHT
    pbsys_hmi_poll_status_light();
    #endif
}
