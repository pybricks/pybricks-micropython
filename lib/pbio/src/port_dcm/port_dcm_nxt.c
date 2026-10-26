// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Device connection manager for NXT sensor ports.
//
// Unlike EV3, the NXT has no pin 2 input, so devices are told apart by how
// they drive pins 5 and 6 and by the voltage on pin 1. On the NXT side, pins
// 5 and 6 go through 82K in series to the ARM pin, which has 1M to ground. So
// an empty line reads low, while a line that the device holds high or low
// stays there. Pin 5 and 6 of port 4 are also wired to an RS485 transceiver,
// which keeps an empty pin 6 at about half the supply voltage. All of these
// tests work on all ports despite this.

#include <pbio/config.h>

#include <pbio/int_math.h>
#include <pbio/port.h>
#include <pbio/port_dcm.h>

#include <pbdrv/adc.h>
#include <pbdrv/clock.h>
#include <pbdrv/gpio.h>

#if PBIO_CONFIG_PORT_DCM_NXT

#include "port_dcm_nxt_sensors.h"

#define DEBUG 0
#if DEBUG
#include <pbio/debug.h>
#define debug_pr pbio_debug
#else
#define debug_pr(...)
#endif

/**
 * Device category types that can be distinguished.
 */
typedef enum {
    /**
     * No device is connected, or a device that can't be detected. This
     * includes the NXT Touch Sensor while released.
     */
    DCM_CATEGORY_NONE,
    /**
     * Device holds pins 5 and 6 high, as I2C devices do with their pull-ups.
     */
    DCM_CATEGORY_NXT_I2C,
    /**
     * Device holds pin 5 high and pin 6 low, and pin 1 is low. This is the
     * NXT Color Sensor, confirmed by its calibration data checksum.
     */
    DCM_CATEGORY_NXT_COLOR,
    /**
     * Device holds pin 5 high and pin 6 low, and pin 1 is not low. This is the
     * NXT Sound Sensor.
     */
    DCM_CATEGORY_NXT_SOUND,
    /**
     * Device pulls pin 5 to ground through its LED transistor base. This is
     * the NXT Light Sensor.
     */
    DCM_CATEGORY_NXT_LIGHT,
    /**
     * Pin 1 is pulled down, with nothing on pins 5 and 6. This is a pressed
     * NXT Touch Sensor (both versions), or another passive analog device.
     */
    DCM_CATEGORY_NXT_TOUCH_PRESSED,
} pbio_port_dcm_category_t;

// Device connection manager state for each port
struct _pbio_port_dcm_t {
    pbio_os_state_t child;
    uint32_t count;
    bool connected;
    pbio_port_dcm_category_t category;
    /** Category of the current scan, kept across a yield. */
    pbio_port_dcm_category_t candidate;
    /** State of an NXT Light Sensor or NXT Color Sensor. */
    pbio_port_dcm_nxt_optical_t optical;
    /** State of an NXT Touch Sensor. */
    pbio_port_dcm_touch_t touch;
};

static pbio_port_dcm_t dcm_state[PBIO_CONFIG_PORT_DCM_NUM_DEV];

#define DCM_LOOP_TIME_MS (10)
#define DCM_LOOP_STEADY_STATE_COUNT (20)
#define DCM_LOOP_DISCONNECT_COUNT (5)

/**
 * Minimum pin 6 voltage for I2C devices.
 *
 * This is in the scale of ::pbio_port_dcm_get_mv, which assumes a 5V
 * reference. The ARM ADC on pin 6 uses 3.3V, so this is about 2.4V. I2C
 * devices read about 2.9V or more, while an empty port 4 reads about 1.75V.
 */
#define DCM_I2C_P6_MIN_MV (3600)

/**
 * The sound sensor is considered disconnected when pin 1 is at least this
 * voltage for ::DCM_SOUND_DISCONNECT_COUNT samples. Empty ports read 5000 mV,
 * while the sound sensor reads about 4800 mV or less, even in a quiet room.
 */
#define DCM_SOUND_DISCONNECT_MV (4950)
#define DCM_SOUND_DISCONNECT_COUNT (10)

/**
 * Pin 1 voltage range of the sound sensor for loudest and quietest sound,
 * matching the SoundSensor class.
 */
#define DCM_SOUND_MIN_MV (650)
#define DCM_SOUND_MAX_MV (4860)

/**
 * Pin 5 is considered pulled low by the device if it reads low within this
 * many reads after releasing it from high. With the NXT Light Sensor, it is
 * low by the second read. An empty port takes 4 reads on port 4 and about 13
 * reads on ports 1--3.
 */
#define DCM_P5_PULLED_LOW_READS (3)

/**
 * Releases pin 5 after it was driven high and tests whether the device pulls
 * it low.
 *
 * A device that pulls it to ground does so much faster than the 1M on an
 * empty port. How long pin 5 was high before this does not matter, as long as
 * the line had time to charge, so the caller may yield in between. Driving it
 * longer only charges an empty line more, which makes it fall slower.
 *
 * @param [in]  pins        The ioport pins.
 * @return                  True if the device pulls pin 5 low.
 */
static bool pbio_port_dcm_p5_release_is_pulled_low(const pbio_port_pins_t *pins) {
    // Each read also makes sure that the pin is an input, so the first one
    // releases the pin.
    for (uint32_t i = 0; i < DCM_P5_PULLED_LOW_READS; i++) {
        if (!pbdrv_gpio_input(&pins->p5)) {
            return true;
        }
    }
    return false;
}

/**
 * Gets the device category based on the passive state of the port.
 *
 * Pins 5 and 6 are read without driving them, so I2C devices are never
 * disturbed. The internal pull-ups must be disabled. This can't detect the
 * NXT Light Sensor, which needs the active pin 5 test.
 *
 * @param [in]  pins        The ioport pins.
 * @return                  The category type.
 */
static pbio_port_dcm_category_t pbio_port_dcm_get_category(const pbio_port_pins_t *pins) {

    uint32_t adc1 = pbio_port_dcm_get_mv(pins, 1);
    bool p5 = pbdrv_gpio_input(&pins->p5);
    bool p6 = pbdrv_gpio_input(&pins->p6);

    if (p5 && p6) {
        return DCM_CATEGORY_NXT_I2C;
    }

    if (p5) {
        return adc1 < 100 ? DCM_CATEGORY_NXT_COLOR : DCM_CATEGORY_NXT_SOUND;
    }

    if (adc1 >= 100 && adc1 < 3100) {
        return DCM_CATEGORY_NXT_TOUCH_PRESSED;
    }

    return DCM_CATEGORY_NONE;
}

/**
 * Thread that detects the device type and runs background processes for
 * devices that need it until they are disconnected.
 *
 * @param [in]  state       The protothread state.
 * @param [in]  timer       The timer to use for timing.
 * @param [in]  dcm         The device connection manager.
 * @param [in]  pins        The ioport pins.
 */
pbio_error_t pbio_port_dcm_thread(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins) {

    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    debug_pr("Start device scan\n");
    dcm->category = DCM_CATEGORY_NONE;
    dcm->connected = false;
    dcm->touch = (pbio_port_dcm_touch_t) {0};

    // Pin 6 of ports 1--3 works as GPIO only while its ADC channel is
    // disabled. Detection and device setup need the GPIO, so disable it until
    // a device needs analog values.
    pbdrv_adc_ch_enable(pins->adc_p6, false);

    // Wait for any device to be connected.
    for (dcm->count = 0; dcm->count < DCM_LOOP_STEADY_STATE_COUNT; dcm->count++) {
        dcm->candidate = pbio_port_dcm_get_category(pins);

        // On port 4, the RS485 bias charges pin 6 of the sound sensor, so it
        // can look like I2C. Only I2C pull-ups take pin 6 near the supply
        // rail, so measure it. This does not drive the pin.
        if (dcm->candidate == DCM_CATEGORY_NXT_I2C) {
            pbdrv_adc_ch_enable(pins->adc_p6, true);
            PBIO_OS_AWAIT(state, &dcm->child, pbdrv_adc_await_new_samples(&dcm->child, &timer->start, pins->adc_p6, 200));
            if (pbio_port_dcm_get_mv(pins, 6) < DCM_I2C_P6_MIN_MV) {
                dcm->candidate = DCM_CATEGORY_NXT_SOUND;
            }
            pbdrv_adc_ch_enable(pins->adc_p6, false);
        }

        // The light sensor connects pin 5 to its LED transistor base, which
        // pulls it low quickly after releasing it from high. Only test this
        // if nothing holds pin 5 high, so I2C devices are never driven. How
        // long it stays high does not matter, so yield in between.
        if (dcm->candidate == DCM_CATEGORY_NONE || dcm->candidate == DCM_CATEGORY_NXT_TOUCH_PRESSED) {
            pbdrv_gpio_out_high(&pins->p5);
            PBIO_OS_AWAIT_ONCE(state);
            if (pbio_port_dcm_p5_release_is_pulled_low(pins)) {
                dcm->candidate = DCM_CATEGORY_NXT_LIGHT;
            }
        }

        if (dcm->candidate != dcm->category || dcm->candidate == DCM_CATEGORY_NONE) {
            dcm->count = 0;
            dcm->category = dcm->candidate;
        }
        PBIO_OS_AWAIT_MS(state, timer, DCM_LOOP_TIME_MS);
    }
    dcm->connected = true;
    debug_pr("Device kind detected: %d\n", dcm->category);

    if (dcm->category == DCM_CATEGORY_NXT_I2C) {
        debug_pr("Continue as I2C process\n");
        // Exit NXT device manager, letting the I2C process take over.
        return PBIO_SUCCESS;
    }

    if (dcm->category == DCM_CATEGORY_NXT_LIGHT) {
        debug_pr("Reading NXT Light Sensor until disconnected.\n");
        PBIO_OS_AWAIT(state, &dcm->child, pbio_port_dcm_nxt_light_run(&dcm->child, &dcm->optical, pins, timer));
        debug_pr("Light Sensor disconnected.\n");
        return PBIO_SUCCESS;
    }

    if (dcm->category == DCM_CATEGORY_NXT_COLOR) {
        // This also confirms that it is a color sensor. On failure, exit so
        // that detection starts over and we try again.
        PBIO_OS_AWAIT(state, &dcm->child, err = pbio_port_dcm_nxt_color_init(&dcm->child, &dcm->optical, pins, timer));
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // Pin 6 is analog from here on. The first sample may be corrupted,
        // but each reading below awaits new samples anyway.
        pbdrv_adc_ch_enable(pins->adc_p6, true);

        // While plugged in, toggle through available colors and measure intensity.
        while (pbio_port_dcm_get_mv(pins, 1) < 100) {
            PBIO_OS_AWAIT(state, &dcm->child, pbio_port_dcm_nxt_color_sample(&dcm->child, &dcm->optical, pins, timer));
        }
        pbdrv_gpio_out_low(&pins->p5);
        debug_pr("Color Sensor disconnected.\n");
        return PBIO_SUCCESS;
    }

    if (dcm->category == DCM_CATEGORY_NXT_SOUND) {
        // Select audible sounds only (dBA), which is also the default of the
        // SoundSensor class. The sensor class may still change it.
        pbdrv_gpio_out_high(&pins->p5);

        // Pin 5 selects the sound sensor mode, so it can't be used to test for
        // disconnection. Pin 6 has a large capacitance that holds any level it
        // is driven to, and on port 4 it is charged by the RS485 bias, so it
        // can't be used either. Instead, wait for pin 1 to go to the supply
        // rail, which the sound sensor does not reach even when quiet.
        for (dcm->count = 0; dcm->count < DCM_SOUND_DISCONNECT_COUNT; dcm->count++) {
            if (pbio_port_dcm_get_mv(pins, 1) < DCM_SOUND_DISCONNECT_MV) {
                dcm->count = 0;
            }
            PBIO_OS_AWAIT_MS(state, timer, DCM_LOOP_TIME_MS);
        }
        debug_pr("Sound Sensor disconnected.\n");
        return PBIO_SUCCESS;
    }

    if (dcm->category == DCM_CATEGORY_NXT_TOUCH_PRESSED) {
        // Released looks the same as disconnected, so just wait for release,
        // which is debounced by the pressed state checker.
        PBIO_OS_AWAIT_WHILE(state, pbio_port_dcm_get_pressed(dcm, pins));
        debug_pr("NXT Touch Sensor released or disconnected\n");
        return PBIO_SUCCESS;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbio_port_dcm_i2c_exited(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins, pbdrv_i2c_dev_t *i2c_dev) {

    PBIO_OS_ASYNC_BEGIN(state);

    // While the bus is idle, the device keeps the clock line high, which goes
    // low on an empty port. Reading the pin makes it an input, which releases
    // the line just like the open drain driver does when idle. So skip samples
    // while user code runs a transaction on this bus.
    for (dcm->count = 0; dcm->count < DCM_LOOP_DISCONNECT_COUNT;) {
        PBIO_OS_AWAIT_MS(state, timer, DCM_LOOP_TIME_MS);
        if (pbdrv_i2c_is_busy(i2c_dev)) {
            continue;
        }
        dcm->count = pbdrv_gpio_input(&pins->p5) ? 0 : dcm->count + 1;
    }
    debug_pr("I2C device disconnected\n");

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

    // NXT Touch sensors can't be detected while released, so allow passing
    // none state also. Raise only if something *else* is definitively connected.
    if (*expected_type_id == LEGO_DEVICE_TYPE_ID_NXT_TOUCH_SENSOR) {
        // Still scanning, so category may be a transient state.
        if (!dcm->connected) {
            return PBIO_SUCCESS;
        }
        return (dcm->category == DCM_CATEGORY_NONE || dcm->category == DCM_CATEGORY_NXT_TOUCH_PRESSED) ?
               PBIO_SUCCESS : PBIO_ERROR_NO_DEV;
    }

    // Generic analog devices usually have no way to be detected, so allow
    // anything that does not use pins 5 and 6 for something else.
    if (*expected_type_id == LEGO_DEVICE_TYPE_ID_NXT_ANALOG) {
        return (dcm->category == DCM_CATEGORY_NXT_I2C || dcm->category == DCM_CATEGORY_NXT_COLOR) ?
               PBIO_ERROR_NO_DEV : PBIO_SUCCESS;
    }

    // Still busy detecting.
    if (!dcm->connected || dcm->category == DCM_CATEGORY_NONE) {
        return PBIO_ERROR_NO_DEV;
    }

    switch (*expected_type_id) {
        case LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR:
            return matches_category(dcm, DCM_CATEGORY_NXT_COLOR);
        case LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR:
            return matches_category(dcm, DCM_CATEGORY_NXT_LIGHT);
        case LEGO_DEVICE_TYPE_ID_NXT_SOUND_SENSOR:
            return matches_category(dcm, DCM_CATEGORY_NXT_SOUND);
        case LEGO_DEVICE_TYPE_ID_NXT_I2C:
            // Known I2C devices are told apart by the I2C process.
            return matches_category(dcm, DCM_CATEGORY_NXT_I2C);
        default:
            return PBIO_ERROR_NO_DEV;
    }
}

uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins, bool active) {

    // Some NXT sensors have an active mode by setting P5 high.
    if (active) {
        pbdrv_gpio_out_high(&pins->p5);
    } else {
        pbdrv_gpio_out_low(&pins->p5);
    }

    return pbio_port_dcm_get_mv(pins, 1);
}

bool pbio_port_dcm_get_pressed(pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins) {
    return pbio_port_dcm_nxt_touch_get_pressed(&dcm->touch, pins);
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

pbsys_telemetry_error_t pbio_port_dcm_get_telemetry(pbio_port_dcm_t *dcm, const pbio_port_pins_t *pins, pbsys_telemetry_packet_t *tel, uint32_t *size) {

    // Touch sensors only show while pressed, so show pressed state.
    if (dcm->category == DCM_CATEGORY_NXT_TOUCH_PRESSED) {
        if (*size < sizeof(uint8_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        tel->id = LEGO_DEVICE_TYPE_ID_NXT_TOUCH_SENSOR;
        tel->mode = 0;
        tel->payload[0] = pbio_port_dcm_get_pressed(dcm, pins);
        *size = sizeof(uint8_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    // Sound intensity in percent, scaled the same as the SoundSensor class.
    // There is only one mode, measured in whichever way pin 5 currently
    // selects, which is audible sounds only by default.
    if (dcm->category == DCM_CATEGORY_NXT_SOUND) {
        if (*size < sizeof(uint8_t)) {
            return PBSYS_TELEMETRY_ERROR_NO_ROOM;
        }
        int32_t mv = pbio_port_dcm_get_mv(pins, 1);
        tel->id = LEGO_DEVICE_TYPE_ID_NXT_SOUND_SENSOR;
        tel->mode = 0;
        tel->payload[0] = 100 - pbio_int_math_bind(100 * (mv - DCM_SOUND_MIN_MV) / (DCM_SOUND_MAX_MV - DCM_SOUND_MIN_MV), 0, 100);
        *size = sizeof(uint8_t);
        return PBSYS_TELEMETRY_SUCCESS;
    }

    // Uknown device.
    tel->id = LEGO_DEVICE_TYPE_ID_NONE;
    tel->mode = 0;
    *size = 0;
    return PBSYS_TELEMETRY_SUCCESS;
}

#endif // PBIO_CONFIG_PORT_DCM_NXT
