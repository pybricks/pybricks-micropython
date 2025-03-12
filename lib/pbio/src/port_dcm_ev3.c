// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <contiki.h>
#include <pbio/config.h>

#include <pbio/port_interface.h>
#include <pbio/port_dcm.h>
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
 * Device candidate types.
 */
typedef enum {
    /**
     * Candidate device is EV3 UART sensor. P6 and ADC6 are arbitrary (can be set by sensor TX).
     */
    DCM_CANDIDATE_LUMP = PIN_STATE_ADC1_0_TO_100 | PIN_STATE_P2_HIGH | PIN_STATE_P5_HIGH | PIN_STATE_MASK_P6,
    /**
     * Candidate device is EV3 analog sensor.
     */
    DCM_CANDIDATE_EV3_ANALOG = PIN_STATE_ADC1_100_to_3100 | PIN_STATE_P2_HIGH,
    /**
     * No candidate device is connected.
     */
    DCM_CANDIDATE_NONE = PIN_STATE_ADC1_4800_to_5000 | PIN_STATE_P2_HIGH | PIN_STATE_P5_HIGH,
    /**
     * Candidate device is NXT Color sensor. P6 and ADC6 are arbitrary (can be set by sensor TX).
     */
    DCM_CANDIDATE_NXT_COLOR = PIN_STATE_ADC1_0_TO_100 | PIN_STATE_P5_HIGH | PIN_STATE_MASK_P6,
    /**
     * Candidate device is NXT I2C.
     */
    DCM_CANDIDATE_NXT_I2C = PIN_STATE_ADC1_4800_to_5000 | PIN_STATE_P5_HIGH | PIN_STATE_MASK_P6,
    /**
     * Candidate device is NXT Temperature sensor (special case of I2C).
     */
    DCM_CANDIDATE_NXT_TEMPERATURE = DCM_CANDIDATE_NXT_I2C | PIN_STATE_P2_HIGH,
    /**
     * Candidate device is an NXT light sensor.
     */
    DCM_CANDIDATE_NXT_LIGHT = PIN_STATE_MASK_P1,
    /**
     * Candidate device is first-iteration NXT touch sensor without auto-id.
     * Can only detect pressed. Released appears as None.
     *
     * The NXT Touch sensor class allows for both variants by allowing this
     * state, the NXT_ANALOG state, as well as the none state. This means you
     * will not get an error if something is missing.
     */
    DCM_CANDIDATE_NXT_TOUCH1_PRESSED = PIN_STATE_ADC1_100_to_3100 | PIN_STATE_P2_HIGH | PIN_STATE_P5_HIGH,
    /**
     * Candidate device is NXT analog sensor (sound sensor or second iteration touch sensor with auto-ID).
     */
    DCM_CANDIDATE_NXT_ANALOG = PIN_STATE_MASK_P1 | PIN_STATE_P5_HIGH,
} pbio_port_dcm_candidate_t;

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
 * Maps the passive port state to candidate device kind.
 *
 * Most states are exact matches, while some combinations allow some pins to
 * be in any state.
 *
 * @param [in]  state       The state.
 * @return                  The candidate type.
 */
static pbio_port_dcm_candidate_t pbio_port_dcm_get_candidate(pbio_port_dcm_pin_state_t state) {

    if ((state | PIN_STATE_MASK_P6) == DCM_CANDIDATE_LUMP) {
        return DCM_CANDIDATE_LUMP;
    }

    if ((state | PIN_STATE_MASK_P6) == DCM_CANDIDATE_NXT_COLOR) {
        return DCM_CANDIDATE_NXT_COLOR;
    }

    if ((state | PIN_STATE_MASK_P1) == DCM_CANDIDATE_NXT_ANALOG) {
        return DCM_CANDIDATE_NXT_ANALOG;
    }

    if ((state | PIN_STATE_MASK_P1) == DCM_CANDIDATE_NXT_LIGHT) {
        return DCM_CANDIDATE_NXT_LIGHT;
    }

    // All other can be tested for equality.
    return (pbio_port_dcm_candidate_t)state;
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
        pbio_port_dcm_get_candidate(state),
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
    uint32_t count;
    bool connected;
    pbio_port_dcm_candidate_t candidate;
};

pbio_port_dcm_t dcm_state[PBIO_CONFIG_PORT_DCM_NUM_DEV];

#define DCM_LOOP_TIME_MS (10)
#define DCM_LOOP_STEADY_STATE_COUNT (20)
#define DCM_LOOP_DISCONNECT_COUNT (5)

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

    etimer_set(etimer, DCM_LOOP_TIME_MS);

    for (;;) {

        debug_pr("Start device scan\n");
        dcm->connected = false;

        // Wait for any device to be connected.
        for (dcm->count = 0; dcm->count < DCM_LOOP_STEADY_STATE_COUNT; dcm->count++) {
            pbio_port_dcm_pin_state_t state = pbio_port_dcm_get_state(pins);
            pbio_port_dcm_candidate_t candidate = pbio_port_dcm_get_candidate(state);
            if (candidate != dcm->candidate || candidate == DCM_CANDIDATE_NONE) {
                dcm->count = 0;
                dcm->candidate = candidate;
            }
            PT_WAIT_UNTIL(pt, etimer_expired(etimer));
            etimer_reset(etimer);
        }
        debug_pr("Device kind detected: %d\n", dcm->candidate);
        dcm->connected = true;

        // Now run processes for devices that require a process, and otherwise
        // wait for disconnection.

        if (dcm->candidate == DCM_CANDIDATE_LUMP) {
            debug_pr("Continue as LUMP process\n");
            // Exit EV3 device manager, letting LUMP manager take over.
            // That process runs until the device no longer reports data.
            PT_EXIT(pt);
        }

        if (dcm->candidate == DCM_CANDIDATE_NXT_TEMPERATURE) {
            // This device has no way to passively detect disconnection, so
            // we need to monitor I2C transactions to see if it is still there.
            debug_pr("Starting NXT temperature sensor process.\n");
            // TODO.
            debug_pr("Stopped NXT temperature sensor process.\n");
            // On disconnect, continue monitoring new connections.
            continue;
        }

        // Disconnection is detected by just one pin going high rather than all
        // pins going back to the none state. This is because other pins are
        // used for data transfer and may vary between high/low during normal
        // operation.
        for (dcm->count = 0; dcm->count < DCM_LOOP_DISCONNECT_COUNT; dcm->count++) {
            // Monitor P5 for EV3 analog, P2 for all NXT sensors.
            const pbdrv_gpio_t *gpio = dcm->candidate == DCM_CANDIDATE_EV3_ANALOG ? &pins->p5 : &pins->p2;
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

pbio_error_t pbio_port_dcm_assert_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t *type_id) {
    // TODO
    return PBIO_ERROR_NO_DEV;
}

#endif // PBIO_CONFIG_PORT_DCM_EV3
