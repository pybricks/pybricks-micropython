// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Device specific behavior of known LEGO UART devices.
//
// The LUMP protocol in protocol.c is generic, but what the data of each device
// means, and which mode it should be in, is not something the device tells us
// in a useful way. That is implemented here.

#include <pbio/config.h>

#if PBIO_CONFIG_PORT_LUMP

#include <string.h>

#include <pbio/color.h>
#include <pbio/int_math.h>
#include <pbio/port_lump.h>
#include <pbio/util.h>

#include <lego/device.h>

#include "lump_dev.h"

bool pbio_port_lump_is_relative_motor(pbio_port_lump_dev_t *lump_dev) {
    return (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_INTERACTIVE_MOTOR) &&
           (lump_dev->mode == LEGO_DEVICE_MODE_PUP_REL_MOTOR__POS);
}

bool pbio_port_lump_is_absolute_motor(pbio_port_lump_dev_t *lump_dev) {
    return (lump_dev->capabilities & LUMP_MODE_FLAGS0_MOTOR_ABS_POS) &&
           (lump_dev->mode == LEGO_DEVICE_MODE_PUP_ABS_MOTOR__CALIB);
}

/**
 * Gets the mode that a device should be in when nothing else is requested.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @return                  The default mode, or 0 if the device has none.
 */
static uint8_t pbio_port_lump_device_get_default_mode(pbio_port_lump_dev_t *lump_dev) {

    if (lump_dev->capabilities & LUMP_MODE_FLAGS0_MOTOR_ABS_POS) {
        return LEGO_DEVICE_MODE_PUP_ABS_MOTOR__CALIB;
    }

    switch (lump_dev->type_id) {
        case LEGO_DEVICE_TYPE_ID_INTERACTIVE_MOTOR:
            return LEGO_DEVICE_MODE_PUP_REL_MOTOR__POS;
        case LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR:
            return LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I;
        case LEGO_DEVICE_TYPE_ID_SPIKE_FORCE_SENSOR:
            return LEGO_DEVICE_MODE_PUP_FORCE_SENSOR__CALIB;
        case LEGO_DEVICE_TYPE_ID_SPIKE_COLOR_SENSOR:
            return LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__RGB_I;
        case LEGO_DEVICE_TYPE_ID_WEDO2_MOTION_SENSOR:
            return LEGO_DEVICE_MODE_PUP_WEDO2_MOTION_SENSOR__CAL;
        #if PBIO_CONFIG_PORT_LUMP_EV3
        case LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR:
            return LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW;
        case LEGO_DEVICE_TYPE_ID_EV3_GYRO_SENSOR:
            return LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__G_A;
        #endif
        default:
            return 0;
    }
}

/**
 * Prepares the device specific state once a device has synced up.
 *
 * Called by the sync thread when the device type and capabilities are known,
 * but before any data is accepted from it.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 */
void pbio_port_lump_device_init(pbio_port_lump_dev_t *lump_dev) {

    // Nothing of the device that was attached before may linger.
    memset(&lump_dev->device, 0, sizeof(lump_dev->device));

    #if PBIO_CONFIG_PORT_LUMP_EV3
    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR) {
        pbio_port_lump_ev3_color_load_calibration(lump_dev);
    }
    #endif

    uint8_t default_mode = pbio_port_lump_device_get_default_mode(lump_dev);
    if (default_mode) {
        pbio_port_lump_request_mode(lump_dev, default_mode);
    }
}

/**
 * Handles incoming data for devices whose data we know how to interpret.
 *
 * Called by the protocol parser for every data message received.
 *
 * @param [in] lump_dev The lump device with new data.
 */
void pbio_port_lump_device_handle_data(pbio_port_lump_dev_t *lump_dev) {

    // Set some initial colors for this device. Can only be done some time
    // after initially plugged in, so wait a few samples.
    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX && lump_dev->device.light_matrix_samples < 2) {
        if (lump_dev->device.light_matrix_samples++ == 1) {
            const uint8_t rainbow[] = {0xa9, 0xa8, 0xa7, 0xa1, 0xaa, 0xa6, 0xa2, 0xa3, 0xa4};
            pbio_port_lump_set_mode_with_data(lump_dev, LEGO_DEVICE_MODE_PUP_COLOR_LIGHT_MATRIX__PIX_O, (uint8_t *)rainbow, sizeof(rainbow));
        }
    }

    // Handles LUMP motors in a mode that reports an absolute angle in decidegrees (0--3600).
    if (pbio_port_lump_is_absolute_motor(lump_dev)) {

        int32_t abs_mdeg = ((int16_t)pbio_get_uint16_le(lump_dev->bin_data + 2)) * 100;

        // Store measured millidegree state value as-is but keep old value.
        int32_t abs_prev = lump_dev->device.angle.millidegrees;
        lump_dev->device.angle.millidegrees = abs_mdeg;

        // Update rotation counter as encoder passes through multiples of 360.
        if (abs_prev > 270000 && abs_mdeg < 90000) {
            lump_dev->device.angle.rotations += 1;
        }
        if (abs_prev < 90000 && abs_mdeg > 270000) {
            lump_dev->device.angle.rotations -= 1;
        }
    }

    // Handles only known LUMP motor that reports incremental angle in degrees.
    if (pbio_port_lump_is_relative_motor(lump_dev)) {
        int32_t degrees = pbio_get_uint32_le(lump_dev->bin_data);
        lump_dev->device.angle.millidegrees = (degrees % 360) * 1000;
        lump_dev->device.angle.rotations = degrees / 360;
    }

    // The force sensor reports its calibration constants in one mode, which we
    // cache so that we can use them to preprocess the values of another mode.
    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_SPIKE_FORCE_SENSOR &&
        lump_dev->mode == LEGO_DEVICE_MODE_PUP_FORCE_SENSOR__CALIB) {
        lump_dev->device.force.offset = (int16_t)pbio_get_uint16_le(lump_dev->bin_data + 2);
        lump_dev->device.force.released = (int16_t)pbio_get_uint16_le(lump_dev->bin_data + 4);
        lump_dev->device.force.end = (int16_t)pbio_get_uint16_le(lump_dev->bin_data + 12);
        pbio_port_lump_request_mode(lump_dev, LEGO_DEVICE_MODE_PUP_FORCE_SENSOR__FRAW);
    }

    #if PBIO_CONFIG_PORT_LUMP_EV3
    // Advances a calibration in progress, if any.
    pbio_port_lump_ev3_color_handle_data(lump_dev);
    #endif
}

/**
 * Gets the minimum time needed before stale data is discarded.
 *
 * This is empirically determined based on sensor experiments.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @return                  Required delay in milliseconds.
 */
uint32_t pbio_port_lump_device_stale_data_delay(pbio_port_lump_dev_t *lump_dev) {
    switch (lump_dev->type_id) {
        case LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR:
            return lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX ? 0 : 30;
        case LEGO_DEVICE_TYPE_ID_SPIKE_COLOR_SENSOR:
            return lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__LIGHT ? 0 : 30;
        case LEGO_DEVICE_TYPE_ID_SPIKE_ULTRASONIC_SENSOR:
            return lump_dev->mode == LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__LIGHT ? 0 : 50;
        #if PBIO_CONFIG_PORT_LUMP_EV3
        case LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR:
            return 30;
        case LEGO_DEVICE_TYPE_ID_EV3_IR_SENSOR:
            return 1100;
        #endif
        default:
            // Default delay for other sensors and modes.
            return 0;
    }
}

/**
 * Gets the minimum time needed for the device to handle written data.
 *
 * This is empirically determined based on sensor experiments.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @return                  Required delay in milliseconds.
 */
uint32_t pbio_port_lump_device_data_set_delay(pbio_port_lump_dev_t *lump_dev) {
    // The Boost Color Distance Sensor requires a long delay or successive
    // writes are ignored.
    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR &&
        lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX) {
        return 250;
    }

    // Default delay for setting data. In practice, this is the delay for setting
    // the light on the color sensor and ultrasonic sensor.
    return 10;
}

/**
 * Gets the angle of a LUMP motor.
 *
 * @param [in]  lump_dev        The LEGO UART device instance.
 * @param [out] angle           The angle of the motor.
 * @param [in]  get_abs_angle   Whether to get the absolute angle.
 * @return                      ::PBIO_SUCCESS on success.
 *                              ::PBIO_ERROR_NO_DEV if this device is not a motor.
 *                              ::PBIO_ERROR_NOT_SUPPORTED if it has no absolute angle.
 *                              Otherwise see ::pbio_port_lump_is_ready.
 */
pbio_error_t pbio_port_lump_get_angle(pbio_port_lump_dev_t *lump_dev, pbio_angle_t *angle, bool get_abs_angle) {

    // Need to be up and running so we don't return stale data.
    pbio_error_t err = pbio_port_lump_is_ready(lump_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Only motors have angles.
    if (!pbio_port_lump_is_relative_motor(lump_dev) && !pbio_port_lump_is_absolute_motor(lump_dev)) {
        return PBIO_ERROR_NO_DEV;
    }

    // Handle request for absolute angle.
    if (get_abs_angle) {
        if (!pbio_port_lump_is_absolute_motor(lump_dev)) {
            return PBIO_ERROR_NOT_SUPPORTED;
        }

        // Mod the angle to be in the range [-180, 180).
        angle->rotations = 0;
        angle->millidegrees = lump_dev->device.angle.millidegrees;
        if (angle->millidegrees >= 180000) {
            angle->millidegrees -= 360000;
        }
        return PBIO_SUCCESS;
    }

    // Otherwise return angle as-is.
    *angle = lump_dev->device.angle;
    return PBIO_SUCCESS;
}

/**
 * Gets the force and displacement measured by the force sensor.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @param [out] force       The measured force in millinewtons.
 * @param [out] distance    The measured displacement in micrometers.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if this is not a force sensor.
 *                          ::PBIO_ERROR_FAILED on bad calibration data.
 *                          Otherwise see ::pbio_port_lump_is_ready.
 */
pbio_error_t pbio_port_lump_get_force(pbio_port_lump_dev_t *lump_dev, int32_t *force, int32_t *distance) {
    // Need to be up and running so we don't return stale data.
    pbio_error_t err = pbio_port_lump_is_ready(lump_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Only available on force sensor.
    if (lump_dev->type_id != LEGO_DEVICE_TYPE_ID_SPIKE_FORCE_SENSOR ||
        lump_dev->mode != LEGO_DEVICE_MODE_PUP_FORCE_SENSOR__FRAW) {
        return PBIO_ERROR_NO_DEV;
    }

    int16_t offset = lump_dev->device.force.offset;
    int16_t released = lump_dev->device.force.released;
    int16_t end = lump_dev->device.force.end;

    // Fail on bad calibration data (released >= end)
    if (released >= end) {
        return PBIO_ERROR_FAILED;
    }

    int16_t raw = (int16_t)pbio_get_uint16_le(lump_dev->bin_data);

    // micrometers
    *distance = (6670 * (raw - released)) / (end - released);

    // millinewtons, counting from calibrated offset.
    *force = (10000 * (raw - released - offset)) / (end - released);
    if (*force < 0) {
        *force = 0;
    }
    return PBIO_SUCCESS;
}

/**
 * Gets the color measured by a LEGO UART color sensor, in a device independent
 * HSV format that is comparable across sensors.
 *
 * This does not change modes. Whatever the sensor measures in the mode that is
 * currently active is what gets returned, and @p mode says which measurement
 * that is.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @param [out] color_hsv   The measured color.
 * @param [out] id          The type of this device, or NULL to skip.
 * @param [out] mode        The Pybricks measurement mode, or NULL to skip.
 *                          These are abstract capabilities rather than LEGO
 *                          modes, since not all LEGO modes are functional or
 *                          meaningfully different. 0 measures the surface lit
 *                          by the sensor light, 1 measures ambient light.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if this device does not measure
 *                          color in the mode that is currently active.
 *                          Otherwise see ::pbio_port_lump_is_ready.
 */
pbio_error_t pbio_port_lump_get_color(pbio_port_lump_dev_t *lump_dev, pbio_color_t *color_hsv, lego_device_type_id_t *id, uint8_t *mode) {

    // Need to be up and running so we don't return stale data.
    pbio_error_t err = pbio_port_lump_is_ready(lump_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    const int16_t *data = (const int16_t *)lump_dev->bin_data;

    pbio_color_t hsv;
    uint8_t measurement;

    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_SPIKE_COLOR_SENSOR &&
        lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__SHSV) {

        // Saturation and value are 0--10000, scaled to match the 0--100
        // range that is typical in applications.
        hsv = PBIO_COLOR_ENCODE(data[0],
            pbio_int_math_min(data[1] / 10, 100),
            pbio_int_math_min(data[2] / 10, 100));
        measurement = 1;

    } else if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_SPIKE_COLOR_SENSOR &&
               lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__RGB_I) {

        const pbio_color_rgb_t rgb = {
            .r = data[0] == 1024 ? 255 : data[0] >> 2,
            .g = data[1] == 1024 ? 255 : data[1] >> 2,
            .b = data[2] == 1024 ? 255 : data[2] >> 2,
        };
        pbio_color_t raw = pbio_color_from_rgb_with_hue_shift(&rgb);
        uint8_t raw_s = pbio_color_get_s(raw);
        int8_t raw_v = pbio_color_get_v(raw);
        hsv = PBIO_COLOR_ENCODE(pbio_color_get_h(raw),
            // Approximately double saturation for low values to get similar
            // results as other sensors.
            raw_s * (200 - raw_s) / 100,
            // Approximately +50% low values to get similar results as with
            // other sensors.
            raw_v * (150 - raw_v / 2) / 100);
        measurement = 0;

    } else if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR &&
               lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I) {

        // Max observed value is ~440 so we scale to get a range of 0..255.
        const pbio_color_rgb_t rgb = {
            .r = 1187 * data[0] / 2048,
            .g = 1187 * data[1] / 2048,
            .b = 1187 * data[2] / 2048,
        };
        pbio_color_t raw = pbio_color_from_rgb_with_hue_shift(&rgb);
        int8_t raw_v = pbio_color_get_v(raw);
        // Approximately double low values to get similar results as with other
        // sensors.
        hsv = PBIO_COLOR_ENCODE(pbio_color_get_h(raw), pbio_color_get_s(raw), raw_v * (200 - raw_v) / 100);
        measurement = 0;

    }
    #if PBIO_CONFIG_PORT_LUMP_EV3
    else if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR &&
             lump_dev->mode == LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW &&
             !pbio_port_lump_ev3_color_calibration_busy(lump_dev)) {

        pbio_color_rgb_t rgb;
        pbio_port_lump_ev3_color_get_rgb(lump_dev, &rgb);
        hsv = pbio_color_from_rgb_with_hue_shift(&rgb);
        measurement = 0;

    }
    #endif
    else {
        // This device does not measure color in the way that is active now.
        return PBIO_ERROR_NO_DEV;
    }

    *color_hsv = hsv;
    if (id) {
        *id = lump_dev->type_id;
    }
    if (mode) {
        *mode = measurement;
    }
    return PBIO_SUCCESS;
}

/**
 * Gets the light intensity measured by a LEGO UART sensor, in permille.
 *
 * This does not change modes. Whatever the sensor measures in the mode that is
 * currently active is what gets returned. Values that the sensor does not
 * measure that way are set to ::PBIO_LIGHT_INTENSITY_NOT_AVAILABLE.
 *
 * @param [in]  lump_dev    The LEGO UART device instance.
 * @param [out] reflected   The measured reflection, 0--1000, or NULL to skip.
 * @param [out] ambient     The measured ambient light, 0--1000, or NULL to skip.
 * @param [out] id          The type of this device, or NULL to skip.
 * @param [out] mode        The Pybricks measurement mode, or NULL to skip.
 *                          See ::pbio_port_lump_get_color.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if this device measures neither
 *                          value in the mode that is currently active.
 *                          Otherwise see ::pbio_port_lump_is_ready.
 */
pbio_error_t pbio_port_lump_get_light_intensity(pbio_port_lump_dev_t *lump_dev, uint32_t *reflected, uint32_t *ambient, lego_device_type_id_t *id, uint8_t *mode) {

    // Need to be up and running so we don't return stale data.
    pbio_error_t err = pbio_port_lump_is_ready(lump_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    const int16_t *data16 = (const int16_t *)lump_dev->bin_data;
    const int8_t *data8 = (const int8_t *)lump_dev->bin_data;

    uint32_t ref = PBIO_LIGHT_INTENSITY_NOT_AVAILABLE;
    uint32_t amb = PBIO_LIGHT_INTENSITY_NOT_AVAILABLE;
    uint8_t measurement;

    if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_SPIKE_COLOR_SENSOR &&
        lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__SHSV) {

        // The value component of the ambient color is 0--10000.
        amb = data16[2] / 10;
        measurement = 1;

    } else if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_SPIKE_COLOR_SENSOR &&
               lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__RGB_I) {

        // Average of the RGB reflections, which each range from 0 to 1024.
        ref = (data16[0] + data16[1] + data16[2]) * 1000 / 3072;
        measurement = 0;

    } else if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR &&
               lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI) {

        amb = data8[0] * 10;
        measurement = 1;

    } else if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR &&
               lump_dev->mode == LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I) {

        // The channels can each read higher than the 400 that this scale
        // assumes, so cap rather than rescale to keep existing values intact.
        ref = pbio_int_math_min((data16[0] + data16[1] + data16[2]) * 10 / 12, 1000);
        measurement = 0;

    }
    #if PBIO_CONFIG_PORT_LUMP_EV3
    else if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR &&
             lump_dev->mode == LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__AMBIENT) {

        amb = data8[0] * 10;
        measurement = 1;

    } else if (lump_dev->type_id == LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR &&
               lump_dev->mode == LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW &&
               !pbio_port_lump_ev3_color_calibration_busy(lump_dev)) {

        // Reflection is derived from the raw color channels rather than the
        // dedicated reflection mode, so that color and reflection can be read
        // without changing modes, just like on other color sensors.
        pbio_color_rgb_t rgb;
        pbio_port_lump_ev3_color_get_rgb(lump_dev, &rgb);
        ref = (rgb.r + rgb.g + rgb.b) * 1000 / 765;
        measurement = 0;
    }
    #endif
    else {
        // This device does not measure light in the way that is active now.
        return PBIO_ERROR_NO_DEV;
    }

    if (reflected) {
        *reflected = ref;
    }
    if (ambient) {
        *ambient = amb;
    }
    if (id) {
        *id = lump_dev->type_id;
    }
    if (mode) {
        *mode = measurement;
    }
    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_PORT_LUMP
