// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbio/config.h>

#include <pbio/port_interface.h>
#include <pbio/port_dcm.h>
#include <pbio/int_math.h>
#include <pbdrv/ioport.h>
#include <pbdrv/adc.h>

#if PBIO_CONFIG_PORT_DCM_EV3

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#include <inttypes.h>
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define debug_pr pbdrv_uart_debug_printf
#define DBG_ERR(expr) expr
#else
#define debug_pr(...)
#define DBG_ERR(expr)
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
    DCM_CATEGORY_EV3_ANALOG = PIN_STATE_ADC1_100_to_3100 | PIN_STATE_P2_HIGH,
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
     * Device is NXT Temperature sensor (special case of I2C).
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
 * Converts a 10-bit ADC value to millivolts.
 *
 * @param [in]  adc_10bit   The 10-bit ADC value.
 * @return                  The voltage in mV.
 */
static uint32_t pbio_port_dcm_adc_to_mv(uint32_t adc_10bit) {
    return adc_10bit * 4888 / 1000;
}

/**
 * Gets the voltage on a pin.
 *
 * @param [in]  pins        The ioport pins.
 * @param [in]  pin         The pin number (1 or 6). Everything else is invalid.
 * @return                  The voltage in mV.
 */
static uint32_t pbio_port_dcm_get_mv(const pbdrv_ioport_pins_t *pins, uint8_t pin) {
    uint16_t adc;
    pbdrv_adc_get_ch(pin == 1 ? pins->adc_p1 : pins->adc_p6, &adc);
    // ADC returns a 10-bit value. Convert to 0--5000mV.
    return pbio_port_dcm_adc_to_mv(adc);
}

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

typedef struct __attribute__((__packed__)) {
    uint32_t calibration[3][4];
    uint16_t threshold_high;
    uint16_t threshold_low;
    uint16_t crc;
} pbio_port_dcm_nxt_color_sensor_data_t;

typedef struct {
    uint8_t bit;
    pbio_port_dcm_nxt_color_sensor_data_t data;
} pbio_port_dcm_nxt_color_sensor_state_t;

/**
 * Thread that reads one color sensor calibration data byte.
 *
 * This is slower than it needs to be since 1ms is the lowest timer resolution.
 * In total we read 54 bytes * 8 bits * 2 cycles = 864 cycles = 864ms.
 * Since this is only used during the initial sensor setup, this is not worth
 * optimizing with a dedicated timer.
 *
 * @param [in]  state         The thread state.
 * @param [in]  sensor_state  The sensor state.
 * @param [in]  pins          The ioport pins.
 * @param [in]  timer        The timer to use for timing.
 * @param [out] msg           The message byte.
 */
static pbio_error_t pbio_port_dcm_nxt_color_rx_msg(pbio_os_state_t *state, pbio_port_dcm_nxt_color_sensor_state_t *sensor_state, const pbdrv_ioport_pins_t *pins, pbio_os_timer_t *timer, uint8_t *msg) {

    PBIO_OS_ASYNC_BEGIN(state);

    *msg = 0;
    pbdrv_gpio_input(&pins->p6);

    // Read 8 bits while toggling the "clock".
    for (sensor_state->bit = 0; sensor_state->bit < 8; sensor_state->bit++) {

        // Clock high and wait for the data bit to settle.
        pbdrv_gpio_out_high(&pins->p5);
        PBIO_OS_AWAIT_MS(state, timer, 1);

        *msg |= pbdrv_gpio_input(&pins->p6) << sensor_state->bit;

        // Clock low and wait for sensor to prepare next bit.
        pbdrv_gpio_out_low(&pins->p5);
        PBIO_OS_AWAIT_MS(state, timer, 1);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Thread that writes one color sensor data byte.
 *
 * @param [in]  state         The thread state.
 * @param [in]  sensor_state  The sensor state.
 * @param [in]  pins          The ioport pins.
 * @param [in]  timer        The timer to use for timing.
 * @param [in]  msg           The message byte.
 */
static pbio_error_t pbio_port_dcm_nxt_color_tx_msg(pbio_os_state_t *state, pbio_port_dcm_nxt_color_sensor_state_t *sensor_state, const pbdrv_ioport_pins_t *pins, pbio_os_timer_t *timer, uint8_t msg) {

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_gpio_out_low(&pins->p5);
    pbdrv_gpio_out_low(&pins->p6);

    // Send 8 bits while toggling the "clock".
    for (sensor_state->bit = 0; sensor_state->bit < 8; sensor_state->bit++) {

        // Set the data bit.
        if (msg & (1 << sensor_state->bit)) {
            pbdrv_gpio_out_high(&pins->p6);
        } else {
            pbdrv_gpio_out_low(&pins->p6);
        }

        // Toggle the clock high and low, giving sensor time to register bit.
        pbdrv_gpio_out_high(&pins->p5);
        PBIO_OS_AWAIT_MS(state, timer, 1);
        pbdrv_gpio_out_low(&pins->p5);
        PBIO_OS_AWAIT_MS(state, timer, 1);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// Device connection manager state for each port
struct _pbio_port_dcm_t {
    pbio_os_state_t child;
    uint32_t count;
    bool connected;
    pbio_port_dcm_category_t category;
    pbio_port_dcm_analog_rgba_t nxt_rgba;
    pbio_port_dcm_analog_rgba_t nxt_rgba_calibrated;
    pbio_port_dcm_nxt_color_sensor_state_t nxt_color_state;
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

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        debug_pr("Start device scan\n");
        dcm->category = DCM_CATEGORY_NONE;
        dcm->connected = false;

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
        debug_pr("Device kind detected: %d\n", dcm->category);
        dcm->connected = true;

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
            continue;
        }

        if (dcm->category == DCM_CATEGORY_NXT_LIGHT) {
            debug_pr("Reading NXT Light Sensor until disconnected.\n");
            // While plugged in, get reflected and ambient light intensity.
            while (!pbdrv_gpio_input(&pins->p2)) {
                // Reflected intensity.
                pbdrv_gpio_out_high(&pins->p5);
                PBIO_OS_AWAIT(state, &dcm->child, pbdrv_adc_await_new_samples(&dcm->child, &timer->start, 200));
                dcm->nxt_rgba.r = pbio_port_dcm_get_mv(pins, 1);

                // Ambient intensity.
                pbdrv_gpio_out_low(&pins->p5);
                PBIO_OS_AWAIT(state, &dcm->child, pbdrv_adc_await_new_samples(&dcm->child, &timer->start, 200));
                dcm->nxt_rgba.a = pbio_port_dcm_get_mv(pins, 1);
            }
            continue;
        }

        if (dcm->category == DCM_CATEGORY_NXT_COLOR) {
            debug_pr("Initializing NXT Color Sensor.\n");

            // The original firmware has a reset sequence where p6 is high and
            // then p5 is toggled twice. It also works with 8 toggles, we can
            // just use the send function with 0xff to achieve the same effect.
            PBIO_OS_AWAIT(state, &dcm->child, pbio_port_dcm_nxt_color_tx_msg(&dcm->child, &dcm->nxt_color_state, pins, timer, 0xff));
            PBIO_OS_AWAIT_MS(state, timer, 100);

            // Set to full color mode.
            PBIO_OS_AWAIT(state, &dcm->child, pbio_port_dcm_nxt_color_tx_msg(&dcm->child, &dcm->nxt_color_state, pins, timer, 13));

            // Receive all calibration info.
            for (dcm->count = 0; dcm->count < sizeof(pbio_port_dcm_nxt_color_sensor_data_t); dcm->count++) {
                PBIO_OS_AWAIT(state, &dcm->child,
                    pbio_port_dcm_nxt_color_rx_msg(&dcm->child, &dcm->nxt_color_state, pins, timer, (uint8_t *)&dcm->nxt_color_state.data + dcm->count));
            }

            // REVISIT: Test checksum and exit on failure.
            debug_pr("Finished initializing NXT Color Sensor.\n");

            // While plugged in, toggle through available colors and measure intensity.
            while (!pbdrv_gpio_input(&pins->p2)) {

                pbdrv_gpio_out_low(&pins->p5);
                PBIO_OS_AWAIT(state, &dcm->child, pbdrv_adc_await_new_samples(&dcm->child, &timer->start, 200));
                dcm->nxt_rgba.a = pbio_port_dcm_get_mv(pins, 6);

                pbdrv_gpio_out_high(&pins->p5);
                PBIO_OS_AWAIT(state, &dcm->child, pbdrv_adc_await_new_samples(&dcm->child, &timer->start, 200));
                dcm->nxt_rgba.r = pbio_port_dcm_get_mv(pins, 6);

                pbdrv_gpio_out_low(&pins->p5);
                PBIO_OS_AWAIT(state, &dcm->child, pbdrv_adc_await_new_samples(&dcm->child, &timer->start, 2000));
                dcm->nxt_rgba.g = pbio_port_dcm_get_mv(pins, 6);

                pbdrv_gpio_out_high(&pins->p5);
                PBIO_OS_AWAIT(state, &dcm->child, pbdrv_adc_await_new_samples(&dcm->child, &timer->start, 200));
                dcm->nxt_rgba.b = pbio_port_dcm_get_mv(pins, 6);
            }
            pbdrv_gpio_out_low(&pins->p5);
            continue;
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
    }

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
        switch (dcm->category) {
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

/**
 * Scales an RGB value based on ambient light and calibration data.
 *
 * Clamps the output to 0--1000.
 *
 * @param [in]  value       The raw value.
 * @param [in]  ambient     The ambient light value.
 * @param [in]  scale       The calibration scale.
 */
static uint32_t scale_rgb(uint32_t value, uint32_t ambient, uint32_t scale) {
    return value <= ambient ? 0 : pbio_int_math_clamp((value - ambient) * scale / 57000, 1000);
}

enum {
    NXT_COLOR_CALIBRATION_HIGH_AMBIENT = 0,
    NXT_COLOR_CALIBRATION_MEDIUM_AMBIENT = 1,
    NXT_COLOR_CALIBRATION_LOW_AMBIENT = 2,
};

pbio_port_dcm_analog_rgba_t *pbio_port_dcm_get_analog_rgba(pbio_port_dcm_t *dcm) {

    pbio_port_dcm_analog_rgba_t *rgba = &dcm->nxt_rgba;
    pbio_port_dcm_analog_rgba_t *calibrated = &dcm->nxt_rgba_calibrated;

    if (dcm->category == DCM_CATEGORY_NXT_LIGHT) {
        // Intensity is inverted.
        uint32_t ambient = 5000 - rgba->a;
        uint32_t reflection = 5000 - rgba->r;
        uint32_t difference = reflection <= ambient ? 0 : reflection - ambient;

        // With higher ambient light, contrast is less pronounced due to the
        // nonlinearity of the sensor. Scale up the difference to compensate.
        // Also normalize to approximately 0--1000.
        uint32_t scale = ambient <= 825 ? 0: ambient - 825;
        calibrated->r = pbio_int_math_clamp(difference * scale / 1200, 1000);
        calibrated->g = 0;
        calibrated->b = 0;
        calibrated->a = pbio_int_math_bind((ambient - 1200) / 4, 0, 1000);
        return calibrated;
    }

    if (dcm->category == DCM_CATEGORY_NXT_COLOR) {
        pbio_port_dcm_nxt_color_sensor_data_t *data = &dcm->nxt_color_state.data;

        // Select calibration row based on ambient light, similar to NXT firmware.
        uint8_t row = NXT_COLOR_CALIBRATION_HIGH_AMBIENT;
        if (rgba->a < pbio_port_dcm_adc_to_mv(data->threshold_low)) {
            row = NXT_COLOR_CALIBRATION_LOW_AMBIENT;
        } else if (rgba->a < pbio_port_dcm_adc_to_mv(data->threshold_high)) {
            row = NXT_COLOR_CALIBRATION_MEDIUM_AMBIENT;
        }

        // Take difference between reflection and ambient, then scale by
        // calibration value, similar to NXT firmware.
        calibrated->r = scale_rgb(rgba->r, rgba->a, data->calibration[row][0]);
        calibrated->g = scale_rgb(rgba->g, rgba->a, data->calibration[row][1]);
        calibrated->b = scale_rgb(rgba->b, rgba->a, data->calibration[row][2]);
        calibrated->a = scale_rgb(rgba->a, 220, data->calibration[row][3] / 4);

        debug_pr("Calibration row: %d because tr0: %d tr1: %d a: %d\n", row,
            pbio_port_dcm_adc_to_mv(data->threshold_high),
            pbio_port_dcm_adc_to_mv(data->threshold_low),
            rgba->a);
        debug_pr("Calibration: %d %d %d %d\n",
            data->calibration[row][0],
            data->calibration[row][1],
            data->calibration[row][2],
            data->calibration[row][3]);
        debug_pr("old: r: %d, g: %d, b: %d, a: %d\n", rgba->r, rgba->g, rgba->b, rgba->a);
        debug_pr("new: r: %d, g: %d, b: %d, a: %d\n", calibrated->r, calibrated->g, calibrated->b, calibrated->a);

        return calibrated;
    }

    return NULL;
}

#endif // PBIO_CONFIG_PORT_DCM_EV3
