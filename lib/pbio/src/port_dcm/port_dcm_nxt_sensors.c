// SPDX-License-Identifier: MIT
// Copyright (c) 2025-2026 The Pybricks Authors

// Support for NXT analog sensors, shared by the EV3 and NXT device connection
// managers.

#include <pbio/config.h>

#if PBIO_CONFIG_PORT_DCM_EV3 || PBIO_CONFIG_PORT_DCM_NXT

#include <stddef.h>

#include <pbio/int_math.h>
#include <pbio/util.h>

#include <pbdrv/adc.h>
#include <pbdrv/clock.h>
#include <pbdrv/gpio.h>

#include "port_dcm_nxt_sensors.h"

#define DEBUG 0
#if DEBUG
#include <pbio/debug.h>
#define debug_pr pbio_debug
#else
#define debug_pr(...)
#endif

/**
 * Converts a 10-bit ADC value to millivolts.
 *
 * @param [in]  adc_10bit   The 10-bit ADC value.
 * @return                  The voltage in mV.
 */
uint32_t pbio_port_dcm_adc_to_mv(uint32_t adc_10bit) {
    return adc_10bit * 4888 / 1000;
}

/**
 * Gets the voltage on a pin.
 *
 * @param [in]  pins        The ioport pins.
 * @param [in]  pin         The pin number (1 or 6). Everything else is invalid.
 * @return                  The voltage in mV.
 */
uint32_t pbio_port_dcm_get_mv(const pbdrv_ioport_pins_t *pins, uint8_t pin) {
    uint16_t adc;
    pbdrv_adc_get_ch(pin == 1 ? pins->adc_p1 : pins->adc_p6, &adc);
    // ADC returns a 10-bit value. Convert to 0--5000mV.
    return pbio_port_dcm_adc_to_mv(adc);
}

/**
 * Thread that reads one color sensor calibration data byte.
 *
 * This is slower than it needs to be since 1ms is the lowest timer resolution.
 * In total we read 54 bytes * 8 bits * 2 cycles = 864 cycles = 864ms.
 * Since this is only used during the initial sensor setup, this is not worth
 * optimizing with a dedicated timer.
 *
 * @param [in]  state         The thread state.
 * @param [in]  optical       The sensor state.
 * @param [in]  pins          The ioport pins.
 * @param [in]  timer         The timer to use for timing.
 * @param [out] msg           The message byte.
 */
static pbio_error_t pbio_port_dcm_nxt_color_rx_msg(pbio_os_state_t *state, pbio_port_dcm_nxt_optical_t *optical, const pbdrv_ioport_pins_t *pins, pbio_os_timer_t *timer, uint8_t *msg) {

    PBIO_OS_ASYNC_BEGIN(state);

    *msg = 0;
    pbdrv_gpio_input(&pins->p6);

    // Read 8 bits while toggling the "clock".
    for (optical->bit = 0; optical->bit < 8; optical->bit++) {

        // Clock high and wait for the data bit to settle.
        pbdrv_gpio_out_high(&pins->p5);
        PBIO_OS_AWAIT_MS(state, timer, 1);

        *msg |= pbdrv_gpio_input(&pins->p6) << optical->bit;

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
 * @param [in]  optical       The sensor state.
 * @param [in]  pins          The ioport pins.
 * @param [in]  timer         The timer to use for timing.
 * @param [in]  msg           The message byte.
 */
static pbio_error_t pbio_port_dcm_nxt_color_tx_msg(pbio_os_state_t *state, pbio_port_dcm_nxt_optical_t *optical, const pbdrv_ioport_pins_t *pins, pbio_os_timer_t *timer, uint8_t msg) {

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_gpio_out_low(&pins->p5);
    pbdrv_gpio_out_low(&pins->p6);

    // Send 8 bits while toggling the "clock".
    for (optical->bit = 0; optical->bit < 8; optical->bit++) {

        // Set the data bit.
        if (msg & (1 << optical->bit)) {
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

/**
 * Computes the checksum of the received color sensor calibration data.
 *
 * Uses CRC-16 with reflected polynomial 0xA001 and initial value 0x5AA5,
 * computed over all bytes except the checksum itself.
 *
 * Based on calcCRC() and readCalibration() in leJOS NXJ, lejos.nxt.SensorPort.
 *
 * @param [in]  data          The received calibration data.
 * @return                    The computed checksum, in received byte order.
 */
static uint16_t pbio_port_dcm_nxt_color_crc(const pbio_port_dcm_nxt_color_sensor_data_t *data) {
    const uint8_t *bytes = (const uint8_t *)data;
    const size_t len = offsetof(pbio_port_dcm_nxt_color_sensor_data_t, crc);
    uint16_t crc = 0x5AA5;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = bytes[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if ((crc ^ byte) & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
            byte >>= 1;
        }
    }
    // Sensor sends high byte first.
    return (crc << 8) | (crc >> 8);
}

/**
 * Thread that resets the NXT Color Sensor and reads its calibration data.
 *
 * @param [in]  state         The thread state.
 * @param [in]  optical       The sensor state.
 * @param [in]  pins          The ioport pins.
 * @param [in]  timer         The timer to use for timing.
 * @return                    ::PBIO_SUCCESS on success.
 *                            ::PBIO_ERROR_IO if the checksum did not match.
 */
pbio_error_t pbio_port_dcm_nxt_color_init(pbio_os_state_t *state, pbio_port_dcm_nxt_optical_t *optical, const pbdrv_ioport_pins_t *pins, pbio_os_timer_t *timer) {

    PBIO_OS_ASYNC_BEGIN(state);

    debug_pr("Initializing NXT Color Sensor.\n");
    optical->rgba.last_sample_time = 0;

    // The original firmware has a reset sequence where p6 is high and
    // then p5 is toggled twice. It also works with 8 toggles, we can
    // just use the send function with 0xff to achieve the same effect.
    PBIO_OS_AWAIT(state, &optical->child, pbio_port_dcm_nxt_color_tx_msg(&optical->child, optical, pins, timer, 0xff));
    PBIO_OS_AWAIT_MS(state, timer, 100);

    // Set to full color mode.
    PBIO_OS_AWAIT(state, &optical->child, pbio_port_dcm_nxt_color_tx_msg(&optical->child, optical, pins, timer, 13));

    // Receive all calibration info.
    for (optical->count = 0; optical->count < sizeof(pbio_port_dcm_nxt_color_sensor_data_t); optical->count++) {
        PBIO_OS_AWAIT(state, &optical->child,
            pbio_port_dcm_nxt_color_rx_msg(&optical->child, optical, pins, timer, (uint8_t *)&optical->data + optical->count));
    }

    // Test checksum, which the sensor sends high byte first.
    uint16_t crc = pbio_port_dcm_nxt_color_crc(&optical->data);
    if (crc != optical->data.crc) {
        debug_pr("NXT Color Sensor calibration checksum failed. Received: 0x%04x, computed: 0x%04x\n", optical->data.crc, crc);
        pbdrv_gpio_out_low(&pins->p5);
        return PBIO_ERROR_IO;
    }
    debug_pr("Finished initializing NXT Color Sensor.\n");

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Thread that takes one full sample of all colors with the NXT Color Sensor.
 *
 * Pin 6 must be configured for analog input.
 *
 * @param [in]  state         The thread state.
 * @param [in]  optical       The sensor state.
 * @param [in]  pins          The ioport pins.
 * @param [in]  timer         The timer to use for timing.
 */
pbio_error_t pbio_port_dcm_nxt_color_sample(pbio_os_state_t *state, pbio_port_dcm_nxt_optical_t *optical, const pbdrv_ioport_pins_t *pins, pbio_os_timer_t *timer) {

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_gpio_out_low(&pins->p5);
    PBIO_OS_AWAIT(state, &optical->child, pbdrv_adc_await_new_samples(&optical->child, &timer->start, pins->adc_p6, 200));
    optical->rgba.a = pbio_port_dcm_get_mv(pins, 6);

    pbdrv_gpio_out_high(&pins->p5);
    PBIO_OS_AWAIT(state, &optical->child, pbdrv_adc_await_new_samples(&optical->child, &timer->start, pins->adc_p6, 200));
    optical->rgba.r = pbio_port_dcm_get_mv(pins, 6);

    pbdrv_gpio_out_low(&pins->p5);
    PBIO_OS_AWAIT(state, &optical->child, pbdrv_adc_await_new_samples(&optical->child, &timer->start, pins->adc_p6, 200));
    optical->rgba.g = pbio_port_dcm_get_mv(pins, 6);

    pbdrv_gpio_out_high(&pins->p5);
    PBIO_OS_AWAIT(state, &optical->child, pbdrv_adc_await_new_samples(&optical->child, &timer->start, pins->adc_p6, 200));
    optical->rgba.b = pbio_port_dcm_get_mv(pins, 6);

    optical->rgba.last_sample_time = pbdrv_clock_get_ms();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Thread that samples the NXT Light Sensor until it is disconnected.
 *
 * @param [in]  state         The thread state.
 * @param [in]  optical       The sensor state.
 * @param [in]  pins          The ioport pins.
 * @param [in]  timer         The timer to use for timing.
 */
pbio_error_t pbio_port_dcm_nxt_light_run(pbio_os_state_t *state, pbio_port_dcm_nxt_optical_t *optical, const pbdrv_ioport_pins_t *pins, pbio_os_timer_t *timer) {

    PBIO_OS_ASYNC_BEGIN(state);

    optical->rgba.last_sample_time = 0;

    #if PBIO_CONFIG_PORT_DCM_EV3
    // Take a reflected and ambient sample each time
    // until pin 2 shows that the sensor is disconnected.
    while (!pbdrv_gpio_input(&pins->p2)) {
        pbdrv_gpio_out_high(&pins->p5);
        PBIO_OS_AWAIT(state, &optical->child, pbdrv_adc_await_new_samples(&optical->child, &timer->start, pins->adc_p1, 200));
        optical->rgba.r = pbio_port_dcm_get_mv(pins, 1);

        pbdrv_gpio_out_low(&pins->p5);
        PBIO_OS_AWAIT(state, &optical->child, pbdrv_adc_await_new_samples(&optical->child, &timer->start, pins->adc_p1, 200));
        optical->rgba.a = pbio_port_dcm_get_mv(pins, 1);
        optical->rgba.last_sample_time = pbdrv_clock_get_ms();
    }

    #else // PBIO_CONFIG_PORT_DCM_EV3

    // TODO, NXT version

    #endif // PBIO_CONFIG_PORT_DCM_EV3

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
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

/**
 * Gets calibrated samples of the NXT Light Sensor or NXT Color Sensor.
 *
 * @param [in]  optical     The sensor state.
 * @param [in]  type_id     The sensor that is running, if any.
 * @param [out] calibrated  The calibrated values, 0--1000.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if no light or color sensor is running.
 *                          ::PBIO_ERROR_AGAIN if no sample is available yet.
 */
static pbio_error_t pbio_port_dcm_nxt_get_calibrated_rgba(pbio_port_dcm_nxt_optical_t *optical, lego_device_type_id_t type_id, pbio_port_dcm_analog_rgba_t *calibrated) {

    pbio_port_dcm_analog_rgba_t *rgba = &optical->rgba;

    if (type_id != LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR && type_id != LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR) {
        return PBIO_ERROR_NO_DEV;
    }

    // No sample ready yet.
    if (!rgba->last_sample_time) {
        return PBIO_ERROR_AGAIN;
    }

    if (type_id == LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR) {

        // Intensity is inverted.
        uint32_t ambient = 5000 - rgba->a;
        uint32_t reflection = 5000 - rgba->r;
        uint32_t difference = reflection <= ambient ? 0 : reflection - ambient;

        // With higher ambient light, contrast is less pronounced due to the
        // nonlinearity of the sensor. Scale up the difference to compensate.
        // Also normalize to approximately 0--1000.
        uint32_t scale = ambient <= 1000 ? 0: (ambient - 1000) * 2;
        debug_pr("NXT Light Sensor: Ambient: %d Reflection: %d Diff: %d Scale: %d\n", ambient, reflection, difference, scale);
        calibrated->r = pbio_int_math_clamp(difference * scale / 1200, 1000);
        calibrated->g = 0;
        calibrated->b = 0;
        calibrated->a = ambient <= 1000 ? 0 : pbio_int_math_bind((ambient - 1000) / 4, 0, 1000);
        calibrated->last_sample_time = rgba->last_sample_time;
        return PBIO_SUCCESS;
    }

    pbio_port_dcm_nxt_color_sensor_data_t *data = &optical->data;

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
    calibrated->last_sample_time = rgba->last_sample_time;

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

    return PBIO_SUCCESS;
}

/**
 * Gets the color measured by the NXT Color Sensor.
 *
 * See ::pbio_port_dcm_get_color for the output arguments and return values.
 *
 * @param [in]  optical     The sensor state.
 * @param [in]  type_id     The sensor that is running, if any.
 */
pbio_error_t pbio_port_dcm_nxt_get_color(pbio_port_dcm_nxt_optical_t *optical, lego_device_type_id_t type_id, pbio_color_t *color_hsv, lego_device_type_id_t *id, uint8_t *mode) {

    if (type_id != LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR) {
        return PBIO_ERROR_NO_DEV;
    }

    pbio_port_dcm_analog_rgba_t rgba;
    pbio_error_t err = pbio_port_dcm_nxt_get_calibrated_rgba(optical, type_id, &rgba);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Values are capped between 0--1000, so scale to get a range of 0..255.
    const pbio_color_rgb_t rgb = {
        .r = rgba.r >> 2,
        .g = rgba.g >> 2,
        .b = rgba.b >> 2,
    };
    *color_hsv = pbio_color_from_rgb_with_hue_shift(&rgb);
    if (id) {
        *id = LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR;
    }
    if (mode) {
        // This sensor measures everything at once, so it has only one mode.
        // The ambient measurement is taken with all sensor lights off, so it
        // does not provide color information.
        *mode = 0;
    }
    return PBIO_SUCCESS;
}

/**
 * Gets the light intensity measured by the NXT Light Sensor or the NXT Color
 * Sensor.
 *
 * See ::pbio_port_dcm_get_light_intensity for the output arguments and return
 * values.
 *
 * @param [in]  optical     The sensor state.
 * @param [in]  type_id     The sensor that is running, if any.
 */
pbio_error_t pbio_port_dcm_nxt_get_light_intensity(pbio_port_dcm_nxt_optical_t *optical, lego_device_type_id_t type_id, uint32_t *reflected, uint32_t *ambient, lego_device_type_id_t *id, uint8_t *mode) {

    pbio_port_dcm_analog_rgba_t rgba;
    pbio_error_t err = pbio_port_dcm_nxt_get_calibrated_rgba(optical, type_id, &rgba);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (reflected) {
        // With the sensor light on, all three color channels contribute.
        *reflected = type_id == LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR ?
            (rgba.r + rgba.g + rgba.b) / 3 : rgba.r;
    }
    if (ambient) {
        *ambient = rgba.a;
    }
    if (id) {
        *id = type_id;
    }
    if (mode) {
        // These sensors measure reflection and ambient light at once, so they
        // have only one mode.
        *mode = 0;
    }
    return PBIO_SUCCESS;
}

// Contacts can briefly open while pressed, so release is only reported once
// readings have been past the release threshold for this long.
#define DCM_TOUCH_RELEASE_DEBOUNCE_MS (20)

/**
 * Updates the debounced touch sensor state. Press is reported immediately,
 * release only after it has been stable for ::DCM_TOUCH_RELEASE_DEBOUNCE_MS.
 *
 * @param [in]  touch         The touch sensor state.
 * @param [in]  past_press    Whether the reading is past the press threshold.
 * @param [in]  past_release  Whether the reading is past the release threshold.
 * @return                    The updated pressed state.
 */
bool pbio_port_dcm_update_touch(pbio_port_dcm_touch_t *touch, bool past_press, bool past_release) {

    if (!touch->pressed) {
        touch->pressed = past_press;
        touch->releasing = false;
        return touch->pressed;
    }

    // Still pressed, so cancel any pending release.
    if (!past_release) {
        touch->releasing = false;
        return true;
    }

    uint32_t now = pbdrv_clock_get_ms();
    if (!touch->releasing) {
        touch->releasing = true;
        touch->release_start = now;
    }
    if (now - touch->release_start >= DCM_TOUCH_RELEASE_DEBOUNCE_MS) {
        touch->pressed = false;
        touch->releasing = false;
    }
    return touch->pressed;
}

// NXT touch sensor thresholds (mV). Released is a clean signal near the supply
// rail, while pressed is noisy with occasional single-sample spikes towards
// released, so the release threshold is close to the released level.
#define DCM_TOUCH_NXT_PRESS_MV (3000)
#define DCM_TOUCH_NXT_RELEASE_MV (4700)

/**
 * Gets the debounced pressed state of an NXT touch sensor, which is low on
 * pin 1 when pressed.
 *
 * @param [in]  touch         The touch sensor state.
 * @param [in]  pins          The ioport pins.
 * @return                    The updated pressed state.
 */
bool pbio_port_dcm_nxt_touch_get_pressed(pbio_port_dcm_touch_t *touch, const pbdrv_ioport_pins_t *pins) {
    uint32_t mv = pbio_port_dcm_get_mv(pins, 1);
    return pbio_port_dcm_update_touch(touch, mv < DCM_TOUCH_NXT_PRESS_MV, mv > DCM_TOUCH_NXT_RELEASE_MV);
}

#endif // PBIO_CONFIG_PORT_DCM_EV3 || PBIO_CONFIG_PORT_DCM_NXT
