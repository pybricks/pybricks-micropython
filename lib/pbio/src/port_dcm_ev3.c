// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <contiki.h>
#include <pbio/config.h>

#include <pbio/port_interface.h>
#include <pbio/port_dcm.h>
#include <pbdrv/ioport.h>
#include <pbdrv/adc.h>

#if PBIO_CONFIG_PORT_DCM_EV3

#define DEBUG 1
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
    return adc * 4888 / 1000;
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

typedef struct {
    /** Ambient analog value. */
    uint32_t a;
    /** Red analog value. */
    uint32_t r;
    /** Green analog value. */
    uint32_t g;
    /** Blue analog value. */
    uint32_t b;
} pbio_port_dcm_analog_values_t;

typedef struct __attribute__((__packed__)) {
    uint32_t calibration[3][4];
    uint16_t threshold[2];
    uint16_t crc;
} pbio_port_dcm_nxt_color_sensor_data_t;

typedef struct {
    uint8_t bit;
    pbio_port_dcm_nxt_color_sensor_data_t data;
} pbio_port_dcm_nxt_color_sensor_state_t;

/**
 * Thread that reads one color sensor calibration data byte.
 *
 * This is slower than it needs to be since 1ms is the lowest etimer resolution.
 * In total we read 54 bytes * 8 bits * 2 cycles = 864 cycles = 864ms.
 * Since this is only used during the initial sensor setup, this is not worth
 * optimizing with a dedicated timer.
 *
 * @param [in]  pt          The process thread.
 * @param [in]  state       The sensor state.
 * @param [in]  pins        The ioport pins.
 * @param [in]  etimer      The etimer to use for timing.
 * @param [out] msg         The message byte.
 */
PT_THREAD(pbio_port_dcm_nxt_color_rx_msg(struct pt *pt, pbio_port_dcm_nxt_color_sensor_state_t *state, const pbdrv_ioport_pins_t *pins, struct etimer *etimer, uint8_t *msg)) {

    PT_BEGIN(pt);

    *msg = 0;
    pbdrv_gpio_input(&pins->p6);

    // Read 8 bits while toggling the "clock".
    for (state->bit = 0; state->bit < 8; state->bit++) {

        // Clock high and wait for the data bit to settle.
        pbdrv_gpio_out_high(&pins->p5);
        etimer_set(etimer, 1);
        PT_WAIT_UNTIL(pt, etimer_expired(etimer));

        *msg |= pbdrv_gpio_input(&pins->p6) << state->bit;

        // Clock low and wait for sensor to prepare next bit.
        pbdrv_gpio_out_low(&pins->p5);
        etimer_set(etimer, 1);
        PT_WAIT_UNTIL(pt, etimer_expired(etimer));
    }

    PT_END(pt);
}

/**
 * Thread that writes one color sensor data byte.
 *
 * @param [in]  pt          The process thread.
 * @param [in]  state       The sensor state.
 * @param [in]  pins        The ioport pins.
 * @param [in]  etimer      The etimer to use for timing.
 * @param [in]  msg         The message byte.
 */
PT_THREAD(pbio_port_dcm_nxt_color_tx_msg(struct pt *pt, pbio_port_dcm_nxt_color_sensor_state_t *state, const pbdrv_ioport_pins_t *pins, struct etimer *etimer, uint8_t msg)) {

    PT_BEGIN(pt);

    pbdrv_gpio_out_low(&pins->p5);
    pbdrv_gpio_out_low(&pins->p6);

    // Send 8 bits while toggling the "clock".
    for (state->bit = 0; state->bit < 8; state->bit++) {

        // Set the data bit.
        if (msg & (1 << state->bit)) {
            pbdrv_gpio_out_high(&pins->p6);
        } else {
            pbdrv_gpio_out_low(&pins->p6);
        }

        // Toggle the clock high and low, giving sensor time to register bit.
        pbdrv_gpio_out_high(&pins->p5);
        etimer_set(etimer, 1);
        PT_WAIT_UNTIL(pt, etimer_expired(etimer));
        pbdrv_gpio_out_low(&pins->p5);
        etimer_set(etimer, 1);
        PT_WAIT_UNTIL(pt, etimer_expired(etimer));
    }

    PT_END(pt);
}

// Device connection manager state for each port
struct _pbio_port_dcm_t {
    struct pt child;
    uint32_t count;
    bool connected;
    pbio_port_dcm_category_t category;
    pbio_port_dcm_analog_values_t nxt_analog_buf;
    pbio_port_dcm_nxt_color_sensor_state_t nxt_color;
};

static pbio_port_dcm_t dcm_state[PBIO_CONFIG_PORT_DCM_NUM_DEV];

#define DCM_LOOP_TIME_MS (10)
#define DCM_LOOP_STEADY_STATE_COUNT (20)
#define DCM_LOOP_DISCONNECT_COUNT (5)

PT_THREAD(pbio_port_dcm_await_new_nxt_analog_sample(pbio_port_dcm_t * dcm, struct etimer *etimer, const pbdrv_ioport_pins_t *pins, uint32_t *value)) {
    struct pt *pt = &dcm->child;

    PT_BEGIN(pt);

    // Wait for LED to settle.
    etimer_set(etimer, 1);
    PT_WAIT_UNTIL(pt, etimer_expired(etimer));

    // Request a new ADC sample. Revisit: Call back on completion instead of time.
    pbdrv_adc_update_soon();
    etimer_set(etimer, 4);
    PT_WAIT_UNTIL(pt, etimer_expired(etimer));

    // Get the value.
    uint8_t pin = dcm->category == DCM_CATEGORY_NXT_COLOR ? 6 : 1;
    *value = pbio_port_dcm_get_mv(pins, pin);

    PT_END(pt);
}

/**
 * Thread that detects the device type. It monitors the ID1 and ID2 pins
 * on the port to see when devices are connected or disconnected.
 *
 * @param [in]  pt          The process thread.
 * @param [in]  etimer      The etimer to use for timing.
 * @param [in]  dcm         The device connection manager.
 * @param [in]  pins        The ioport pins.
 */
PT_THREAD(pbio_port_dcm_thread(struct pt *pt, struct etimer *etimer, pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins)) {

    PT_BEGIN(pt);

    for (;;) {

        etimer_set(etimer, DCM_LOOP_TIME_MS);

        debug_pr("Start device scan\n");
        dcm->category = DCM_CATEGORY_NONE;
        dcm->connected = false;

        // Wait for any device to be connected.
        for (dcm->count = 0; dcm->count < DCM_LOOP_STEADY_STATE_COUNT; dcm->count++) {
            pbio_port_dcm_pin_state_t state = pbio_port_dcm_get_state(pins);
            pbio_port_dcm_category_t category = pbio_port_dcm_get_category(state);
            if (category != dcm->category || category == DCM_CATEGORY_NONE) {
                dcm->count = 0;
                dcm->category = category;
            }
            PT_WAIT_UNTIL(pt, etimer_expired(etimer));
            etimer_reset(etimer);
        }
        debug_pr("Device kind detected: %d\n", dcm->category);
        dcm->connected = true;

        // Now run processes for devices that require a process, and otherwise
        // wait for disconnection.

        if (dcm->category == DCM_CATEGORY_LUMP) {
            debug_pr("Continue as LUMP process\n");
            // Exit EV3 device manager, letting LUMP manager take over.
            // That process runs until the device no longer reports data.
            PT_EXIT(pt);
        }

        if (dcm->category == DCM_CATEGORY_NXT_TEMPERATURE) {
            // This device has no way to passively detect disconnection, so
            // we need to monitor I2C transactions to see if it is still there.
            debug_pr("Starting NXT temperature sensor process.\n");
            // TODO.
            debug_pr("Stopped NXT temperature sensor process.\n");
            continue;
        }

        if (dcm->category == DCM_CATEGORY_NXT_COLOR) {
            debug_pr("Initializing NXT Color Sensor.\n");

            // The original firmware has a reset sequence where p6 is high and
            // then p5 is toggled twice. It also works with 8 toggles, we can
            // just use the send function with 0xff to achieve the same effect.
            PT_SPAWN(pt, &dcm->child, pbio_port_dcm_nxt_color_tx_msg(&dcm->child, &dcm->nxt_color, pins, etimer, 0xff));
            etimer_set(etimer, 100);
            PT_WAIT_UNTIL(pt, etimer_expired(etimer));

            // Set to full color mode.
            PT_SPAWN(pt, &dcm->child, pbio_port_dcm_nxt_color_tx_msg(&dcm->child, &dcm->nxt_color, pins, etimer, 13));

            // Receive all calibration info.
            for (dcm->count = 0; dcm->count < sizeof(pbio_port_dcm_nxt_color_sensor_data_t); dcm->count++) {
                PT_SPAWN(pt, &dcm->child,
                    pbio_port_dcm_nxt_color_rx_msg(&dcm->child, &dcm->nxt_color, pins, etimer, (uint8_t *)&dcm->nxt_color.data + dcm->count));
            }

            // Checksum and continue on failure.
            debug_pr("Finished initializing NXT Color Sensor.\n");
        }

        if (dcm->category == DCM_CATEGORY_NXT_LIGHT || dcm->category == DCM_CATEGORY_NXT_COLOR) {
            debug_pr("Reading NXT Light/Color Sensor until disconnected.\n");
            // While plugged in, toggle through available colors.
            while (!pbdrv_gpio_input(&pins->p2)) {

                pbdrv_gpio_out_low(&pins->p5);
                PT_SPAWN(pt, &dcm->child, pbio_port_dcm_await_new_nxt_analog_sample(dcm, etimer, pins, &dcm->nxt_analog_buf.a));
                pbdrv_gpio_out_high(&pins->p5);
                PT_SPAWN(pt, &dcm->child, pbio_port_dcm_await_new_nxt_analog_sample(dcm, etimer, pins, &dcm->nxt_analog_buf.r));

                if (dcm->category == DCM_CATEGORY_NXT_LIGHT) {
                    // Light sensor doesn't have green and blue.
                    continue;
                }

                pbdrv_gpio_out_low(&pins->p5);
                PT_SPAWN(pt, &dcm->child, pbio_port_dcm_await_new_nxt_analog_sample(dcm, etimer, pins, &dcm->nxt_analog_buf.g));
                pbdrv_gpio_out_high(&pins->p5);
                PT_SPAWN(pt, &dcm->child, pbio_port_dcm_await_new_nxt_analog_sample(dcm, etimer, pins, &dcm->nxt_analog_buf.b));

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
            PT_WAIT_UNTIL(pt, etimer_expired(etimer));
            etimer_reset(etimer);
        }
        debug_pr("Device disconnected\n");
    }

    PT_END(pt);
}

pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index) {
    if (index >= PBIO_CONFIG_PORT_DCM_NUM_DEV) {
        return NULL;
    }
    pbio_port_dcm_t *dcm = &dcm_state[index];
    return dcm;
}

static pbio_error_t must_equal(lego_device_type_id_t candidate, lego_device_type_id_t expected) {
    return candidate == expected ? PBIO_SUCCESS : PBIO_ERROR_NO_DEV;
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
    if (!dcm->connected) {
        return PBIO_ERROR_NO_DEV;
    }

    switch (dcm->category) {
        case DCM_CATEGORY_NONE:
            return PBIO_ERROR_NO_DEV;
        case DCM_CATEGORY_EV3_ANALOG:
            // This is the only known EV3 analog sensor.
            return must_equal(*expected_type_id, LEGO_DEVICE_TYPE_ID_EV3_TOUCH_SENSOR);
        case DCM_CATEGORY_NXT_COLOR:
            // Only one sensor in this category.
            return must_equal(*expected_type_id, LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR);
        case DCM_CATEGORY_NXT_TEMPERATURE:
            // Only one sensor in this category.
            return must_equal(*expected_type_id, LEGO_DEVICE_TYPE_ID_NXT_TEMPERATURE_SENSOR);
        case DCM_CATEGORY_NXT_LIGHT:
            // For legacy compatibility, allow anything wired like a NXT light
            // sensor to be detected and used as a generic analog sensor.
            if (*expected_type_id == LEGO_DEVICE_TYPE_ID_NXT_ANALOG) {
                *expected_type_id = LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR;
                return PBIO_SUCCESS;
            } else {
                // If a specific type is requested, it must be the light sensor.
                return must_equal(*expected_type_id, LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR);
            }
        case DCM_CATEGORY_NXT_ANALOG_OTHER:
            // In principle, the sound sensor can be distinguished using ADCP6,
            // but we pass both here for simplicity.
            if (*expected_type_id == LEGO_DEVICE_TYPE_ID_NXT_ANALOG ||
                *expected_type_id == LEGO_DEVICE_TYPE_ID_NXT_SOUND_SENSOR ||
                *expected_type_id == LEGO_DEVICE_TYPE_ID_NXT_TOUCH_SENSOR) {
                return PBIO_SUCCESS;
            } else {
                return PBIO_ERROR_NO_DEV;
            }
        case DCM_CATEGORY_NXT_I2C:
            // TODO: Check standard LEGO I2C device ID.
            return PBIO_ERROR_NO_DEV;
        default:
            return PBIO_ERROR_NO_DEV;
    }
}

// add arg for color mode, none, red, green, blue.
uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins, pbio_port_dcm_analog_light_type_t light_type) {

    // This category measures analog on pin 6.
    if (dcm->category == DCM_CATEGORY_EV3_ANALOG) {
        return pbio_port_dcm_get_mv(pins, 6);
    }

    // Return buffered values in case of NXT light and color sensor.
    if (dcm->category == DCM_CATEGORY_NXT_LIGHT || dcm->category == DCM_CATEGORY_NXT_COLOR) {
        switch (light_type) {
            case PBIO_PORT_DCM_ANALOG_LIGHT_TYPE_NONE:
                return dcm->nxt_analog_buf.a;
            case PBIO_PORT_DCM_ANALOG_LIGHT_TYPE_RED:
                return dcm->nxt_analog_buf.r;
            case PBIO_PORT_DCM_ANALOG_LIGHT_TYPE_GREEN:
                return dcm->nxt_analog_buf.g;
            case PBIO_PORT_DCM_ANALOG_LIGHT_TYPE_BLUE:
                return dcm->nxt_analog_buf.b;
            default:
                return 0;
        }
    }

    // Everything else measures analog on pin 1.
    return pbio_port_dcm_get_mv(pins, 1);

}

#endif // PBIO_CONFIG_PORT_DCM_EV3
