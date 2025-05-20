// SPDX-License-Identifier: MIT
// Copyright (c) 2020,2022 The Pybricks Authors

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <contiki.h>
#include <pbdrv/bluetooth.h>

#include <pbdrv/charger.h>
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
    PBSYS_STATUS_LIGHT_INDICATION_WARNING_NONE,
    PBSYS_STATUS_LIGHT_INDICATION_WARNING_HIGH_CURRENT,
    PBSYS_STATUS_LIGHT_INDICATION_WARNING_LOW_VOLTAGE,
    PBSYS_STATUS_LIGHT_INDICATION_WARNING_SHUTDOWN_REQUESTED,
    PBSYS_STATUS_LIGHT_INDICATION_WARNING_SHUTDOWN,
} pbsys_status_light_indication_warning_t;

typedef enum {
    PBSYS_STATUS_LIGHT_INDICATION_BLUETOOTH_BLE_NONE,
    PBSYS_STATUS_LIGHT_INDICATION_BLUETOOTH_BLE_ADVERTISING,
    PBSYS_STATUS_LIGHT_INDICATION_BLUETOOTH_BLE_CONNECTED_IDLE,
} pbsys_status_light_indication_ble_t;

/** A single element of a status light indication pattern. */
typedef struct {
    /** Color to display or ::PBIO_COLOR_NONE for system/user color. */
    pbio_color_t color;
    /**
     * Duration to display the color, expressed as the number of poll
     * intervals (50 ms each). Use 0 to ignore that element and restart the
     * pattern. Use 255 to indicate that the element should last forever.
     */
    uint8_t duration;
} pbsys_status_light_indication_pattern_element_t;

/** Sentinel value for status light indication patterns. */
#define PBSYS_STATUS_LIGHT_DURATION_REPEAT (0)
#define PBSYS_STATUS_LIGHT_DURATION_FOREVER (255)
#define PBSYS_STATUS_LIGHT_INDICATION_PATTERN_REPEAT \
    { .duration = PBSYS_STATUS_LIGHT_DURATION_REPEAT }
#define PBSYS_STATUS_LIGHT_INDICATION_PATTERN_FOREVER(color_num) \
    { .color = color_num, .duration = PBSYS_STATUS_LIGHT_DURATION_FOREVER, }

static const pbsys_status_light_indication_pattern_element_t *const
pbsys_status_light_indication_pattern_warning[] = {
    // Transparent, i.e. no warning overlay.
    [PBSYS_STATUS_LIGHT_INDICATION_WARNING_NONE] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_FOREVER(PBIO_COLOR_NONE),
    },
    // Two red blinks, pause, then repeat. Overlays on lower priority signal.
    [PBSYS_STATUS_LIGHT_INDICATION_WARNING_HIGH_CURRENT] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_RED, .duration = 2 },
        { .color = PBIO_COLOR_NONE, .duration = 2 },
        { .color = PBIO_COLOR_RED, .duration = 2 },
        { .color = PBIO_COLOR_NONE, .duration = 22 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_REPEAT
    },
    // Two orange blinks, pause, then repeat. Overlays on lower priority signal.
    [PBSYS_STATUS_LIGHT_INDICATION_WARNING_LOW_VOLTAGE] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_ORANGE, .duration = 2 },
        { .color = PBIO_COLOR_NONE, .duration = 2 },
        { .color = PBIO_COLOR_ORANGE, .duration = 2 },
        { .color = PBIO_COLOR_NONE, .duration = 22 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_REPEAT
    },
    // Rapidly repeating blue blink. Overrides lower priority signal.
    [PBSYS_STATUS_LIGHT_INDICATION_WARNING_SHUTDOWN_REQUESTED] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_BLACK, .duration = 1 },
        { .color = PBIO_COLOR_BLUE, .duration = 1 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_REPEAT
    },
    // Black, so override to be off. Overrides lower priority signal.
    [PBSYS_STATUS_LIGHT_INDICATION_WARNING_SHUTDOWN] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_FOREVER(PBIO_COLOR_BLACK),
    },
};

typedef struct {
    const char *name;
    pbio_color_t color; // Use pbio_color_t enum for configuration
} hub_color_config_t;

// Define color configurations based on hub names
static const hub_color_config_t hub_color_configs[] = {
    {"Blue", PBIO_COLOR_BLUE},
    {"Pink", PBIO_COLOR_MAGENTA},
    {"Red", PBIO_COLOR_RED},
    {"Yellw", PBIO_COLOR_YELLOW},
    {"Orang", PBIO_COLOR_ORANGE},
    {"Green", PBIO_COLOR_GREEN},
    {"White", PBIO_COLOR_WHITE},
    // Add more named pbio_color_t configurations here
};
static const uint8_t num_hub_color_configs = sizeof(hub_color_configs) / sizeof(hub_color_configs[0]);

// Mutable pattern array for BLE advertising, to be populated at runtime.
static pbsys_status_light_indication_pattern_element_t
    pbsys_ble_advertising_pattern_elements[] = {
    { .color = PBIO_COLOR_BLUE, .duration = 3 },
    { .color = PBIO_COLOR_BLACK, .duration = 5 },
    { .color = PBIO_COLOR_BLUE, .duration = 3 },
    { .color = PBIO_COLOR_BLACK, .duration = 12 },
    PBSYS_STATUS_LIGHT_INDICATION_PATTERN_REPEAT
};

// Mutable pattern array for BLE connected, to be populated at runtime.
static pbsys_status_light_indication_pattern_element_t
    pbsys_ble_connected_pattern_elements[] = {
    { .color = PBIO_COLOR_BLUE, .duration = 2 },
    { .color = PBIO_COLOR_BLACK, .duration = 2 },
    { .color = PBIO_COLOR_BLUE, .duration = 2 },
    { .color = PBIO_COLOR_BLACK, .duration = 8 },
    { .color = PBIO_COLOR_BLUE, .duration = 2 },
    { .color = PBIO_COLOR_BLACK, .duration = 2 },
    { .color = PBIO_COLOR_BLUE, .duration = 2 },
    { .color = PBIO_COLOR_BLACK, .duration = 8 },
    PBSYS_STATUS_LIGHT_INDICATION_PATTERN_REPEAT
};

// Array of pointers to pattern elements. Pointers are const, but data can be mutable
// for the BLE advertising case to allow runtime color updates.
static pbsys_status_light_indication_pattern_element_t *const
pbsys_status_light_indication_pattern_ble[] = {
    [PBSYS_STATUS_LIGHT_INDICATION_BLUETOOTH_BLE_NONE] =
        (pbsys_status_light_indication_pattern_element_t *)(const pbsys_status_light_indication_pattern_element_t[]) {
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_FOREVER(PBIO_COLOR_NONE),
    },
    // Two blinks with hub-specific colors, pause, then repeat.
    [PBSYS_STATUS_LIGHT_INDICATION_BLUETOOTH_BLE_ADVERTISING] =
        pbsys_ble_advertising_pattern_elements,
    // Blue, always on.
    [PBSYS_STATUS_LIGHT_INDICATION_BLUETOOTH_BLE_CONNECTED_IDLE] =
        pbsys_ble_connected_pattern_elements
};

typedef struct {
    /** The current status light indication. */
    uint8_t indication;
    /** The current index in the current status light indication pattern. */
    uint8_t pattern_index;
    /** The current duration count in the current status light indication pattern. */
    uint8_t pattern_count;
} pbsys_status_light_pattern_state_t;

typedef struct {
    /** The color LED device */
    pbdrv_led_dev_t *led;
    /** The most recent user color value. */
    pbio_color_hsv_t user_color;
    /** The user program is currently allowed to control the status light. */
    bool allow_user_update;
    /** The current pattern state. */
    pbsys_status_light_pattern_state_t pattern_state;
    /** The current pattern overlay state. */
    pbsys_status_light_pattern_state_t pattern_overlay_state;
    /** Color light struct for PBIO light implementation. */
    pbio_color_light_t color_light;
} pbsys_status_light_t;

/** The system status light instance. */
static pbsys_status_light_t pbsys_status_light_instance_main;
pbio_color_light_t *pbsys_status_light_main = &pbsys_status_light_instance_main.color_light;

#if PBSYS_CONFIG_STATUS_LIGHT_BLUETOOTH
static pbsys_status_light_t pbsys_status_light_instance_ble;
static pbsys_status_light_pattern_state_t *ble_pattern_state = &pbsys_status_light_instance_ble.pattern_state;
static pbsys_status_light_pattern_state_t *warning_pattern_state = &pbsys_status_light_instance_main.pattern_state;
#else
static pbsys_status_light_pattern_state_t *ble_pattern_state = &pbsys_status_light_instance_main.pattern_state;
static pbsys_status_light_pattern_state_t *warning_pattern_state = &pbsys_status_light_instance_main.pattern_overlay_state;
#endif


static pbio_error_t pbsys_status_light_set_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv) {
    pbsys_status_light_t *instance = PBIO_CONTAINER_OF(light, pbsys_status_light_t, color_light);
    instance->user_color = *hsv;

    if (!instance->led) {
        return PBIO_ERROR_NO_DEV;
    }

    if (instance->allow_user_update) {
        return pbdrv_led_set_hsv(instance->led, hsv);
    }

    return PBIO_SUCCESS;
}

static const pbio_color_light_funcs_t pbsys_status_light_funcs = {
    .set_hsv = pbsys_status_light_set_hsv,
};

// Forward declarations
static uint8_t find_color_index(const char *name_to_find);
static void pbsys_user_program_light_colors_init(void);

// Static variables for hub color animations
static uint8_t selected_hub_color_index_1 = 0; // Default to the first configuration
static uint8_t selected_hub_color_index_2 = 0; // Default to the first configuration

void pbsys_status_light_init(void) {
    // REVISIT: Light ids currently hard-coded.
    pbdrv_led_get_dev(0, &pbsys_status_light_instance_main.led);
    pbio_color_light_init(pbsys_status_light_main, &pbsys_status_light_funcs);
    #if PBSYS_CONFIG_STATUS_LIGHT_BLUETOOTH
    pbdrv_led_get_dev(2, &pbsys_status_light_instance_ble.led);
    pbio_color_light_init(&pbsys_status_light_instance_ble.color_light, &pbsys_status_light_funcs);
    #endif

    // Initialize cached color indices based on hub name
    pbsys_user_program_light_colors_init();

    // Update the BLE advertising pattern with dynamically selected colors
    pbsys_ble_advertising_pattern_elements[0].color = hub_color_configs[selected_hub_color_index_1].color;
    pbsys_ble_advertising_pattern_elements[2].color = hub_color_configs[selected_hub_color_index_2].color;

    // Update the BLE connected pattern with dynamically selected colors
    pbsys_ble_connected_pattern_elements[0].color = hub_color_configs[selected_hub_color_index_1].color;
    pbsys_ble_connected_pattern_elements[2].color = hub_color_configs[selected_hub_color_index_1].color;
    pbsys_ble_connected_pattern_elements[4].color = hub_color_configs[selected_hub_color_index_2].color;
    pbsys_ble_connected_pattern_elements[6].color = hub_color_configs[selected_hub_color_index_2].color;
}

static void pbsys_user_program_light_colors_init(void) {

    // This block enables parsing the hub name to set colors.
    const char *full_hub_name_const = pbdrv_bluetooth_get_hub_name();

    // Assuming pbdrv_bluetooth_get_hub_name() is robust as per user feedback.
    // A NULL check could be added here if issues persist: if (full_hub_name_const != NULL)
    char hub_name_copy[16];
    // Ensure space for null terminator by copying at most sizeof-1 characters
    strncpy(hub_name_copy, full_hub_name_const, sizeof(hub_name_copy) - 1);
    hub_name_copy[sizeof(hub_name_copy) - 1] = '\0'; // Ensure null termination

    char *current_pos = hub_name_copy;
    char *name1_str = NULL;
    char *name2_str = NULL;

    // Expecting "Hub Color1/Color2" format. Skip "Hub " prefix.
    if (strncmp(current_pos, "Hub ", 4) == 0) {
        current_pos += 4; // Move pointer past "Hub "
    }
    // After skipping "Hub ", current_pos points to the start of "Color1" or an empty string.

    name1_str = current_pos; // Assume Color1 starts here
    char *slash_ptr = strchr(current_pos, '/');

    if (slash_ptr != NULL) {
        *slash_ptr = '\0'; // Terminate name1_str at the slash
        name2_str = slash_ptr + 1; // name2_str starts after the slash
        // Skip any leading spaces for name2_str, ensuring not to read past buffer
        while (*name2_str == ' ' && *name2_str != '\0') {
            name2_str++;
        }
        if (*name2_str == '\0') { // If name2_str is empty or only spaces
            name2_str = NULL;
        }
    }
    // If slash_ptr is NULL, name1_str is the rest of current_pos, and name2_str remains NULL.

    selected_hub_color_index_1 = find_color_index(name1_str);
    if (name2_str != NULL && name2_str[0] != '\0') {
        selected_hub_color_index_2 = find_color_index(name2_str);
    } else {
        selected_hub_color_index_2 = selected_hub_color_index_1;
    }
}

static void pbsys_status_light_handle_status_change(void) {

    // Warning pattern precedence.
    pbsys_status_light_indication_warning_t warning_indication = PBSYS_STATUS_LIGHT_INDICATION_WARNING_NONE;
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        warning_indication = PBSYS_STATUS_LIGHT_INDICATION_WARNING_SHUTDOWN;
    #if PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS
    } else if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
        warning_indication = PBSYS_STATUS_LIGHT_INDICATION_WARNING_SHUTDOWN_REQUESTED;
    #endif
    } else if (pbsys_status_test(PBIO_PYBRICKS_STATUS_BATTERY_HIGH_CURRENT)) {
        warning_indication = PBSYS_STATUS_LIGHT_INDICATION_WARNING_HIGH_CURRENT;
    } else if (pbsys_status_test(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_WARNING)) {
        warning_indication = PBSYS_STATUS_LIGHT_INDICATION_WARNING_LOW_VOLTAGE;
    }

    // BLE pattern precedence.
    pbsys_status_light_indication_ble_t ble_indication = PBSYS_STATUS_LIGHT_INDICATION_BLUETOOTH_BLE_NONE;
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_BLE_ADVERTISING)) {
        ble_indication = PBSYS_STATUS_LIGHT_INDICATION_BLUETOOTH_BLE_ADVERTISING;
    } else if (pbsys_status_test(PBIO_PYBRICKS_STATUS_BLE_HOST_CONNECTED)
               #if !PBSYS_CONFIG_STATUS_LIGHT_BLUETOOTH
               // Hubs without Bluetooth light show idle state only when program not running.
               && !pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)
               #endif
               ) {
        ble_indication = PBSYS_STATUS_LIGHT_INDICATION_BLUETOOTH_BLE_CONNECTED_IDLE;
    }

    // If the indication changed, then reset the indication pattern to the
    // beginning. Reset both so that patterns with the same length stay in sync.
    if (ble_pattern_state->indication != ble_indication || warning_pattern_state->indication != warning_indication) {
        ble_pattern_state->indication = ble_indication;
        ble_pattern_state->pattern_index = ble_pattern_state->pattern_count = 0;

        warning_pattern_state->indication = warning_indication;
        warning_pattern_state->pattern_index = warning_pattern_state->pattern_count = 0;
    }
}

// Helper function to find the index of a color name in the config array
static uint8_t find_color_index(const char *name_to_find) {
    // Attempt to find the provided name first
    if (name_to_find != NULL && name_to_find[0] != '\0') {
        for (uint8_t i = 0; i < num_hub_color_configs; i++) {
            if (strcmp(name_to_find, hub_color_configs[i].name) == 0) {
                return i; // Found the name
            }
        }
    }

    // If name_to_find is NULL, empty, or not found, default to "blue".
    // Assumes "blue" is the first entry (index 0) in the hub_color_configs array.
    return 0;
}

#if PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS
static uint8_t animation_progress;
static bool use_first_color_in_pulse = true;

static uint32_t default_user_program_light_animation_next(pbio_light_animation_t *animation) {
    // The brightness pattern has the form /\ through which we cycle in N steps.
    // It is reset back to the start when the user program starts.
    const uint8_t animation_progress_max = 200;
    pbio_color_hsv_t hsv; // Target HSV for the LED driver

    uint8_t current_color_index = use_first_color_in_pulse ? selected_hub_color_index_1 : selected_hub_color_index_2;
    pbio_color_t current_pb_color = hub_color_configs[current_color_index].color;

    // Convert the configured pbio_color_t to HSV
    pbio_color_to_hsv(current_pb_color, &hsv);

    // Override the V (brightness) component with the animation
    hsv.v = animation_progress < animation_progress_max / 2 ?
        animation_progress :
        animation_progress_max - animation_progress;

    // At low brightness some colors look like others
    if (hsv.v < 20) {
        hsv.v = 0;
    }

    pbsys_status_light_main->funcs->set_hsv(pbsys_status_light_main, &hsv);

    // This increment controls the speed of the pattern and wraps on completion
    uint8_t next_animation_progress = (animation_progress + 4) % animation_progress_max;

    // If animation wrapped around, toggle the color
    if (next_animation_progress < animation_progress) { // Check for wrap-around
        use_first_color_in_pulse = !use_first_color_in_pulse;
    }
    animation_progress = next_animation_progress;

    return 40;
}
#endif // PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS

void pbsys_status_light_handle_event(process_event_t event, process_data_t data) {
    if (event == PBIO_EVENT_STATUS_SET || event == PBIO_EVENT_STATUS_CLEARED) {
        pbsys_status_light_handle_status_change();
    }
    if (event == PBIO_EVENT_STATUS_SET && (pbio_pybricks_status_t)(intptr_t)data == PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING) {
        #if PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS
        animation_progress = 0;
        use_first_color_in_pulse = true; // Reset to first color at program start
        pbio_light_animation_init(&pbsys_status_light_main->animation, default_user_program_light_animation_next);
        pbio_light_animation_start(&pbsys_status_light_main->animation);
        #else
        pbio_color_light_off(pbsys_status_light_main);
        #endif // PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS
    }
}

/**
 * Advances the light to the next state in the pattern.
 *
 * @param [in]  state       The light pattern state.
 * @param [in]  pattern     The array of pattern arrays.
 * @return                  The new color for the light.
 */
static pbio_color_t pbsys_status_light_pattern_next(pbsys_status_light_pattern_state_t *state,
    const pbsys_status_light_indication_pattern_element_t *const *patterns) {

    const pbsys_status_light_indication_pattern_element_t *pattern =
        patterns[state->indication];
    pbio_color_t new_color = PBIO_COLOR_NONE;

    if (pattern != NULL) {
        // if we are at the end of a pattern, wrap around to the beginning
        if (pattern[state->pattern_index].duration == PBSYS_STATUS_LIGHT_DURATION_REPEAT) {
            state->pattern_index = 0;
            // count should already be set to 0 because of previous index increment
            assert(state->pattern_count == 0);
        }

        new_color = pattern[state->pattern_index].color;

        // If the current index does not run forever and we have exceeded the
        // pattern duration for the current index, move to the next index.
        if (pattern[state->pattern_index].duration != PBSYS_STATUS_LIGHT_DURATION_FOREVER &&
            ++state->pattern_count >= pattern[state->pattern_index].duration) {
            state->pattern_index++;
            state->pattern_count = 0;
        }
    }

    return new_color;
}

#if PBSYS_CONFIG_STATUS_LIGHT_BATTERY

#include <pbsys/battery.h>

typedef enum {
    /** Charger is not connected and battery is discharging. */
    PBSYS_BATTERY_LIGHT_DISCHARGING,
    /** Charger is connected and battery is charging and is not yet full. */
    PBSYS_BATTERY_LIGHT_CHARGING,
    /** Charger is connected and battery is charging and is "full". */
    PBSYS_BATTERY_LIGHT_CHARGING_FULL,
    /** Charger is connected and battery is discharging because it is past full. */
    PBSYS_BATTERY_LIGHT_OVERCHARGE,
    /** There is a problem with the battery or charger. */
    PBSYS_BATTERY_LIGHT_FAULT,
} pbsys_battery_light_state_t;

static pbsys_status_light_pattern_state_t pbsys_battery_light_pattern_state;

static const pbsys_status_light_indication_pattern_element_t *const
pbsys_battery_light_patterns[] = {
    [PBSYS_BATTERY_LIGHT_CHARGING] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_FOREVER(PBIO_COLOR_RED),
    },
    [PBSYS_BATTERY_LIGHT_CHARGING_FULL] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_FOREVER(PBIO_COLOR_GREEN),
    },
    [PBSYS_BATTERY_LIGHT_OVERCHARGE] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_GREEN, .duration = 56 },
        { .color = PBIO_COLOR_BLACK, .duration = 4 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_REPEAT
    },
    [PBSYS_BATTERY_LIGHT_FAULT] =
        (const pbsys_status_light_indication_pattern_element_t[]) {
        { .color = PBIO_COLOR_YELLOW, .duration = 10 },
        { .color = PBIO_COLOR_BLACK, .duration = 10 },
        PBSYS_STATUS_LIGHT_INDICATION_PATTERN_REPEAT
    },
};

pbsys_battery_light_state_t pbsys_battery_light_get_state(void) {
    switch (pbdrv_charger_get_status()) {
        case PBDRV_CHARGER_STATUS_CHARGE:
            if (pbsys_battery_is_full()) {
                return PBSYS_BATTERY_LIGHT_CHARGING_FULL;
            }

            return PBSYS_BATTERY_LIGHT_CHARGING;
        case PBDRV_CHARGER_STATUS_COMPLETE:
            return PBSYS_BATTERY_LIGHT_OVERCHARGE;
        case PBDRV_CHARGER_STATUS_FAULT:
            return PBSYS_BATTERY_LIGHT_FAULT;
        default:
            return PBSYS_BATTERY_LIGHT_DISCHARGING;
    }
}

#endif // PBSYS_CONFIG_STATUS_LIGHT_BATTERY

static void pbsys_status_light_set_pattern_or_user_color(pbsys_status_light_t *instance, pbio_color_t pattern_color) {
    if (!instance->led) {
        return;
    }
    if (instance->allow_user_update) {
        pbdrv_led_set_hsv(instance->led, &instance->user_color);
    } else {
        pbio_color_hsv_t hsv;
        pbio_color_to_hsv(pattern_color, &hsv);
        pbdrv_led_set_hsv(instance->led, &hsv);
    }
}

void pbsys_status_light_poll(void) {

    pbio_color_t new_warning_color = pbsys_status_light_pattern_next(
        warning_pattern_state, pbsys_status_light_indication_pattern_warning);

    pbio_color_t new_ble_color = pbsys_status_light_pattern_next(
        ble_pattern_state, (const pbsys_status_light_indication_pattern_element_t *const *)pbsys_status_light_indication_pattern_ble);


    pbio_color_t new_main_color = new_warning_color;
    #if !PBSYS_CONFIG_STATUS_LIGHT_BLUETOOTH
    // Overlay warnings on ble state on hubs with just one light.
    if (new_warning_color == PBIO_COLOR_NONE) {
        new_main_color = new_ble_color;
    }
    #endif

    // If the new system indication is not overriding the status light and a user
    // program is running, then we can allow the user program to directly change
    // the status light.
    pbsys_status_light_instance_main.allow_user_update =
        new_main_color == PBIO_COLOR_NONE && pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);

    pbsys_status_light_set_pattern_or_user_color(&pbsys_status_light_instance_main, new_main_color);
    #if PBSYS_CONFIG_STATUS_LIGHT_BLUETOOTH
    pbsys_status_light_set_pattern_or_user_color(&pbsys_status_light_instance_ble, new_ble_color);
    #endif

    // REVISIT: We should be able to make updating the state event driven instead of polled.
    #if PBSYS_CONFIG_STATUS_LIGHT_BATTERY

    pbsys_battery_light_state_t new_state = pbsys_battery_light_get_state();

    if (new_state != pbsys_battery_light_pattern_state.indication) {
        pbsys_battery_light_pattern_state.indication = new_state;
        pbsys_battery_light_pattern_state.pattern_count =
            pbsys_battery_light_pattern_state.pattern_index = 0;
    }
    pbio_color_t new_battery_color = pbsys_status_light_pattern_next(
        &pbsys_battery_light_pattern_state, pbsys_battery_light_patterns);

    // FIXME: Use sys light instance like the other lights.
    pbdrv_led_dev_t *led;
    if (pbdrv_led_get_dev(1, &led) == PBIO_SUCCESS) {
        pbio_color_hsv_t hsv;
        pbio_color_to_hsv(new_battery_color, &hsv);
        pbdrv_led_set_hsv(led, &hsv);
    }

    #endif // PBSYS_CONFIG_STATUS_LIGHT_BATTERY
}

#endif // PBSYS_CONFIG_STATUS_LIGHT
