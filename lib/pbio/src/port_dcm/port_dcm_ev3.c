// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbio/config.h>

#include <pbio/port.h>
#include <pbio/port_dcm.h>

#include <pbdrv/adc.h>
#include <pbdrv/clock.h>
#include <pbdrv/ioport.h>

#if PBIO_CONFIG_PORT_DCM_EV3

#include "port_dcm_nxt_sensors.h"

#define DEBUG 0
#if DEBUG
#include <pbio/debug.h>
#define debug_pr pbio_debug
#else
#define debug_pr(...)
#endif

/**
 * Pin port state flags.
 */
typedef enum {
    /**
     * Pin 1 ADC is less than 100mV.
     */
    PIN_STATE_ADC1_0_TO_100 = 1 << 0,
    /**
     * Pin 1 ADC is between 100mv and 3100 mV.
     */
    PIN_STATE_ADC1_100_to_3100 = 1 << 1,
    /**
     * Pin 1 ADC is between 3100mv and 4800 mV.
     */
    PIN_STATE_ADC1_3100_to_4800 = 1 << 2,
    /**
     * Pin 1 ADC is between 4800mv and 5000 mV.
     */
    PIN_STATE_ADC1_4800_to_5000 = 1 << 3,
    /**
     * Pin 2 GPIO is high.
     */
    PIN_STATE_P2_HIGH = 1 << 4,
    /**
     * Pin 5 GPIO is high.
     */
    PIN_STATE_P5_HIGH = 1 << 5,
    /**
     * Pin 6 GPIO is high.
     */
    PIN_STATE_P6_HIGH = 1 << 6,
    /**
     * Pin 1 state is irrelevant for the device.
     */
    PIN_STATE_MASK_P1 = PIN_STATE_ADC1_0_TO_100 | PIN_STATE_ADC1_100_to_3100 | PIN_STATE_ADC1_3100_to_4800 | PIN_STATE_ADC1_4800_to_5000,
    /**
     * Pin 6 state is irrelevant for the device.
     */
    PIN_STATE_MASK_P6 = PIN_STATE_P6_HIGH,
} pbio_port_dcm_pin_state_t;

/**
 * Device category types that can be distinguished. Some categories contain
 * only one known device.
 */
typedef enum {
    /**
     * Device category is EV3 UART sensor. P6 and ADC6 are arbitrary (can be set by sensor TX).
     */
    DCM_CATEGORY_LUMP = PIN_STATE_ADC1_0_TO_100 | PIN_STATE_P2_HIGH | PIN_STATE_P5_HIGH | PIN_STATE_MASK_P6,
    /**
     * Device category is EV3 analog sensor.
     */
    DCM_CATEGORY_EV3_ANALOG = PIN_STATE_ADC1_100_to_3100 | PIN_STATE_P2_HIGH | PIN_STATE_MASK_P6,
    /**
     * No device is connected.
     */
    DCM_CATEGORY_NONE = PIN_STATE_ADC1_4800_to_5000 | PIN_STATE_P2_HIGH | PIN_STATE_P5_HIGH,
    /**
     * Device is NXT Color sensor. P6 and ADC6 are arbitrary (can be set by sensor TX).
     */
    DCM_CATEGORY_NXT_COLOR = PIN_STATE_ADC1_0_TO_100 | PIN_STATE_P5_HIGH | PIN_STATE_MASK_P6,
    /**
     * Device category is NXT I2C.
     */
    DCM_CATEGORY_NXT_I2C = PIN_STATE_ADC1_4800_to_5000 | PIN_STATE_P5_HIGH | PIN_STATE_MASK_P6,
    /**
     * Device is NXT Temperature sensor or Energy Meter (special case of I2C).
     */
    DCM_CATEGORY_NXT_TEMPERATURE = DCM_CATEGORY_NXT_I2C | PIN_STATE_P2_HIGH,
    /**
     * Device is an NXT light sensor.
     */
    DCM_CATEGORY_NXT_LIGHT = PIN_STATE_MASK_P1,
    /**
     * Device is a first-iteration NXT touch sensor without auto-id.
     * Can only detect pressed state. Released appears as None.
     *
     * The NXT Touch sensor class allows for both variants by allowing this
     * state, the NXT_ANALOG state, as well as the none state. This means you
     * will not get an error if something is missing.
     */
    DCM_CATEGORY_NXT_TOUCH1_PRESSED = PIN_STATE_ADC1_100_to_3100 | PIN_STATE_P2_HIGH | PIN_STATE_P5_HIGH,
    /**
     * Device category is NXT analog sensor (sound sensor or second iteration touch sensor with auto-ID).
     */
    DCM_CATEGORY_NXT_ANALOG_OTHER = PIN_STATE_MASK_P1 | PIN_STATE_P5_HIGH,
} pbio_port_dcm_category_t;

/**
 * Maps the passive port state to device category.
 *
 * Most states are exact matches, while some combinations allow some pins to
 * be in any state.
 *
 * @param [in]  state       The state.
 * @return                  The category type.
 */
static pbio_port_dcm_category_t pbio_port_dcm_get_category(pbio_port_dcm_pin_state_t state) {

    if ((state | PIN_STATE_MASK_P6) == DCM_CATEGORY_EV3_ANALOG) {
        return DCM_CATEGORY_EV3_ANALOG;
    }

    if ((state | PIN_STATE_MASK_P6) == DCM_CATEGORY_LUMP) {
        return DCM_CATEGORY_LUMP;
    }

    if ((state | PIN_STATE_MASK_P6) == DCM_CATEGORY_NXT_COLOR) {
        return DCM_CATEGORY_NXT_COLOR;
    }

    if ((state | PIN_STATE_MASK_P1) == DCM_CATEGORY_NXT_ANALOG_OTHER) {
        return DCM_CATEGORY_NXT_ANALOG_OTHER;
    }

    if ((state | PIN_STATE_MASK_P1) == DCM_CATEGORY_NXT_LIGHT) {
        return DCM_CATEGORY_NXT_LIGHT;
    }

    // All other can be tested for equality.
    return (pbio_port_dcm_category_t)state;
}

/**
 * Gets the state of the port. EV3 ports pins and adcs have several distinct
 * states they can be in. Several permutations correspond to specific devices.
 *
 * @param [in]  pins        The ioport pins.
 * @return                  The state.
 */
static pbio_port_dcm_pin_state_t pbio_port_dcm_get_state(const pbdrv_ioport_pins_t *pins) {
    pbio_port_dcm_pin_state_t state = 0;

    // Get the ADC value state for pin 1.
    uint32_t adc1 = pbio_port_dcm_get_mv(pins, 1);
    if (adc1 < 100) {
        state |= PIN_STATE_ADC1_0_TO_100;
    } else if (adc1 < 3100) {
        state |= PIN_STATE_ADC1_100_to_3100;
    } else if (adc1 < 4800) {
        state |= PIN_STATE_ADC1_3100_to_4800;
    } else {
        state |= PIN_STATE_ADC1_4800_to_5000;
    }

    // Get the GPIO state for pins 2, 5, and 6.
    if (pbdrv_gpio_input(&pins->p2)) {
        state |= PIN_STATE_P2_HIGH;
    }
    if (pbdrv_gpio_input(&pins->p5)) {
        state |= PIN_STATE_P5_HIGH;
    }
    if (pbdrv_gpio_input(&pins->p6)) {
        state |= PIN_STATE_P6_HIGH;
    }

    #if DEBUG == 2
    debug_pr("%d::: p1: %dmv p2:%d p5:%d p6:%d (%d mv)\n",
        pbio_port_dcm_get_category(state),
        adc1,
        (bool)(state & PIN_STATE_P2_HIGH),
        (bool)(state & PIN_STATE_P5_HIGH),
        (bool)(state & PIN_STATE_P6_HIGH),
        pbio_port_dcm_get_mv(pins, 6));
    #endif

    return state;
}

// Device connection manager state for each port
struct _pbio_port_dcm_t {
    pbio_os_state_t child;
    uint32_t count;
    bool connected;
    pbio_port_dcm_category_t category;
    /** State of an NXT Light Sensor or NXT Color Sensor. */
    pbio_port_dcm_nxt_optical_t optical;
    /** State of an analog touch sensor on pin 1 (NXT). */
    pbio_port_dcm_touch_t p1_touch;
    /** State of an analog touch sensor on pin 6 (EV3). */
    pbio_port_dcm_touch_t p6_touch;
};

static pbio_port_dcm_t dcm_state[PBIO_CONFIG_PORT_DCM_NUM_DEV];

#define DCM_LOOP_TIME_MS (10)
#define DCM_LOOP_STEADY_STATE_COUNT (20)
#define DCM_LOOP_DISCONNECT_COUNT (5)

/**
 * Thread that detects the device type. It monitors the ID1 and ID2 pins
 * on the port to see when devices are connected or disconnected.
 *
 * @param [in]  state       The protothread state.
 * @param [in]  timer       The timer to use for timing.
 * @param [in]  dcm         The device connection manager.
 * @param [in]  pins        The ioport pins.
 */
pbio_error_t pbio_port_dcm_thread(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins) {

    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    debug_pr("Start device scan\n");
    dcm->category = DCM_CATEGORY_NONE;
    dcm->connected = false;
    dcm->p1_touch = (pbio_port_dcm_touch_t) {0};
    dcm->p6_touch = (pbio_port_dcm_touch_t) {0};

    // Wait for any device to be connected.
    for (dcm->count = 0; dcm->count < DCM_LOOP_STEADY_STATE_COUNT; dcm->count++) {
        pbio_port_dcm_pin_state_t pin_state = pbio_port_dcm_get_state(pins);
        pbio_port_dcm_category_t category = pbio_port_dcm_get_category(pin_state);
        if (category != dcm->category || category == DCM_CATEGORY_NONE) {
            dcm->count = 0;
            dcm->category = category;
        }
        PBIO_OS_AWAIT_MS(state, timer, DCM_LOOP_TIME_MS);
    }
    dcm->connected = true;
    debug_pr("Device kind detected: %d\n", dcm->category);

    // Now run processes for devices that require a process, and otherwise
    // wait for disconnection.

    if (dcm->category == DCM_CATEGORY_LUMP) {
        debug_pr("Continue as LUMP process\n");
        // Exit EV3 device manager, letting LUMP manager take over.
        // That process runs until the device no longer reports data.
        return PBIO_SUCCESS;
    }

    if (dcm->category == DCM_CATEGORY_NXT_TEMPERATURE) {
        // This device has no way to passively detect disconnection, so
        // we need to monitor I2C transactions to see if it is still there.
        debug_pr("Starting NXT temperature sensor process.\n");
        // TODO.
        debug_pr("Stopped NXT temperature sensor process.\n");
        return PBIO_SUCCESS;
    }

    if (dcm->category == DCM_CATEGORY_NXT_LIGHT) {
        debug_pr("Reading NXT Light Sensor until disconnected.\n");
        dcm->optical.rgba.last_sample_time = 0;
        // While plugged in, get reflected and ambient light intensity.
        while (!pbdrv_gpio_input(&pins->p2)) {
            PBIO_OS_AWAIT(state, &dcm->child, pbio_port_dcm_nxt_light_sample(&dcm->child, &dcm->optical, pins, timer));
        }
        return PBIO_SUCCESS;
    }

    if (dcm->category == DCM_CATEGORY_NXT_COLOR) {
        // On failure, exit so that detection starts over and we try again.
        PBIO_OS_AWAIT(state, &dcm->child, err = pbio_port_dcm_nxt_color_init(&dcm->child, &dcm->optical, pins, timer));
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // While plugged in, toggle through available colors and measure intensity.
        while (!pbdrv_gpio_input(&pins->p2)) {
            PBIO_OS_AWAIT(state, &dcm->child, pbio_port_dcm_nxt_color_sample(&dcm->child, &dcm->optical, pins, timer));
        }
        pbdrv_gpio_out_low(&pins->p5);
        debug_pr("Color Sensor disconnected.\n");
        return PBIO_SUCCESS;
    }

    if (dcm->category == DCM_CATEGORY_NXT_TOUCH1_PRESSED) {
        // The first NXT touch sensor has no ID pin, so it is only seen while
        // pressed. Released looks the same as disconnected, so just wait for
        // release, which is debounced by the pressed state checker.
        PBIO_OS_AWAIT_WHILE(state, pbio_port_dcm_get_pressed(dcm, pins));
        debug_pr("NXT Touch Sensor released or disconnected\n");
        return PBIO_SUCCESS;
    }

    // For everything else, disconnection is detected by just one pin going
    // high rather than all pins going back to the none state. This is
    // because other pins are used for data transfer and may vary between
    // high/low during normal operation.
    for (dcm->count = 0; dcm->count < DCM_LOOP_DISCONNECT_COUNT; dcm->count++) {
        // Monitor P5 for EV3 analog, P2 for all NXT sensors.
        const pbdrv_gpio_t *gpio = dcm->category == DCM_CATEGORY_EV3_ANALOG ? &pins->p5 : &pins->p2;
        if (!pbdrv_gpio_input(gpio)) {
            dcm->count = 0;
        }
        PBIO_OS_AWAIT_MS(state, timer, DCM_LOOP_TIME_MS);
    }
    debug_pr("Device disconnected\n");

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index) {
    if (index >= PBIO_CONFIG_PORT_DCM_NUM_DEV) {
        return NULL;
    }
    pbio_port_dcm_t *dcm = &dcm_state[index];
    return dcm;
}

static pbio_error_t matches_category(pbio_port_dcm_t *dcm, pbio_port_dcm_category_t expected) {
    return dcm->category == expected ? PBIO_SUCCESS : PBIO_ERROR_NO_DEV;
}

pbio_error_t pbio_port_dcm_assert_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t *expected_type_id) {

    // NXT Touch sensors can't be detected definitively, so allow passing none
    // state also. Raise only if something *else* is definitively connected.
    if (*expected_type_id == LEGO_DEVICE_TYPE_ID_NXT_TOUCH_SENSOR) {
        // Still scanning, so category may be a transient state.
        if (!dcm->connected) {
            return PBIO_SUCCESS;
        }
        switch (dcm->category) {
            // This one is technically covered by the early exit above, but
            // left in for clarity.
            case DCM_CATEGORY_NONE:
            case DCM_CATEGORY_NXT_TOUCH1_PRESSED:
            case DCM_CATEGORY_NXT_ANALOG_OTHER:
                return PBIO_SUCCESS;
            default:
                return PBIO_ERROR_NO_DEV;
        }
    }

    // Still busy detecting.
    if (!dcm->connected || dcm->category == DCM_CATEGORY_NONE) {
        return PBIO_ERROR_NO_DEV;
    }

    switch (*expected_type_id) {
        case LEGO_DEVICE_TYPE_ID_ANY_LUMP_UART:
            return matches_category(dcm, DCM_CATEGORY_LUMP);
        case LEGO_DEVICE_TYPE_ID_EV3_TOUCH_SENSOR:
            return matches_category(dcm, DCM_CATEGORY_EV3_ANALOG);
        case LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR:
            return matches_category(dcm, DCM_CATEGORY_NXT_COLOR);
        case LEGO_DEVICE_TYPE_ID_NXT_TEMPERATURE_SENSOR:
            return matches_category(dcm, DCM_CATEGORY_NXT_TEMPERATURE);
        case LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR:
            return matches_category(dcm, DCM_CATEGORY_NXT_LIGHT);
        case LEGO_DEVICE_TYPE_ID_NXT_SOUND_SENSOR:
            return matches_category(dcm, DCM_CATEGORY_NXT_ANALOG_OTHER);
        case LEGO_DEVICE_TYPE_ID_NXT_ANALOG:
            // Allow any NXT analog device as well as anything wired like the
            // NXT Light Sensor.
            return (dcm->category == DCM_CATEGORY_NXT_ANALOG_OTHER || dcm->category == DCM_CATEGORY_NXT_LIGHT) ?
                   PBIO_SUCCESS : PBIO_ERROR_NO_DEV;
        case LEGO_DEVICE_TYPE_ID_NXT_I2C:
            // Temperature sensor is also an I2C sensor.
            return (dcm->category == DCM_CATEGORY_NXT_I2C || dcm->category == DCM_CATEGORY_NXT_TEMPERATURE) ?
                   PBIO_SUCCESS : PBIO_ERROR_NO_DEV;
        default:
            return PBIO_ERROR_NO_DEV;
    }
}

uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins, bool active) {

    // This category measures analog on pin 6.
    if (dcm->category == DCM_CATEGORY_EV3_ANALOG) {
        return pbio_port_dcm_get_mv(pins, 6);
    }

    // Some NXT sensors have an active mode by setting P5 high.
    if (active) {
        pbdrv_gpio_out_high(&pins->p5);
    } else {
        pbdrv_gpio_out_low(&pins->p5);
    }

    // Everything else measures analog on pin 1.
    return pbio_port_dcm_get_mv(pins, 1);
}

// EV3 touch sensor thresholds (mV).
#define DCM_TOUCH_EV3_PRESS_MV (3000)
#define DCM_TOUCH_EV3_RELEASE_MV (1000)

bool pbio_port_dcm_get_pressed(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins) {

    // EV3 touch sensor on pin 6 is high when pressed.
    if (dcm->category == DCM_CATEGORY_EV3_ANALOG) {
        uint32_t mv = pbio_port_dcm_get_mv(pins, 6);
        return pbio_port_dcm_update_touch(&dcm->p6_touch,
            mv > DCM_TOUCH_EV3_PRESS_MV, mv < DCM_TOUCH_EV3_RELEASE_MV);
    }

    return pbio_port_dcm_nxt_touch_get_pressed(&dcm->p1_touch, pins);
}

/**
 * Gets the NXT Light Sensor or NXT Color Sensor that is running, if any.
 *
 * @param [in]  dcm         The device connection manager.
 * @return                  The sensor type, or ::LEGO_DEVICE_TYPE_ID_NONE.
 */
static lego_device_type_id_t pbio_port_dcm_get_optical_type(pbio_port_dcm_t *dcm) {
    if (dcm->category == DCM_CATEGORY_NXT_COLOR) {
        return LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR;
    }
    if (dcm->category == DCM_CATEGORY_NXT_LIGHT) {
        return LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR;
    }
    return LEGO_DEVICE_TYPE_ID_NONE;
}

pbio_error_t pbio_port_dcm_get_color(pbio_port_dcm_t *dcm, pbio_color_t *color_hsv, lego_device_type_id_t *id, uint8_t *mode) {
    return pbio_port_dcm_nxt_get_color(&dcm->optical, pbio_port_dcm_get_optical_type(dcm), color_hsv, id, mode);
}

pbio_error_t pbio_port_dcm_get_light_intensity(pbio_port_dcm_t *dcm, uint32_t *reflected, uint32_t *ambient, lego_device_type_id_t *id, uint8_t *mode) {
    return pbio_port_dcm_nxt_get_light_intensity(&dcm->optical, pbio_port_dcm_get_optical_type(dcm), reflected, ambient, id, mode);
}

pbsys_telemetry_error_t pbio_port_dcm_get_telemetry(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins, pbsys_telemetry_packet_t *tel, uint32_t *size) {

    // Best-effort display for useful info despite device limitations:
    // - The only EV3 analog sensor ever known is a touch sensor.
    // - The old style NXT Touch Sensor only shows when pressed.
    // - The sound sensor shows up as a Touch Sensor as a side effect
    //   but will still report correct analog values for volume.
    if (dcm->category == DCM_CATEGORY_EV3_ANALOG ||
        dcm->category == DCM_CATEGORY_NXT_TOUCH1_PRESSED ||
        dcm->category == DCM_CATEGORY_NXT_ANALOG_OTHER) {
        if (*size < sizeof(uint8_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        tel->id = dcm->category == DCM_CATEGORY_EV3_ANALOG ?
            LEGO_DEVICE_TYPE_ID_EV3_TOUCH_SENSOR :
            LEGO_DEVICE_TYPE_ID_NXT_TOUCH_SENSOR;

        // Pressed state.
        tel->mode = 0;
        tel->payload[0] = pbio_port_dcm_get_pressed(dcm, pins);
        *size = sizeof(uint8_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    // Uknown device.
    tel->id = LEGO_DEVICE_TYPE_ID_NONE;
    tel->mode = 0;
    *size = 0;
    return PBSYS_TELEMETRY_SUCCESS;
}

#endif // PBIO_CONFIG_PORT_DCM_EV3
