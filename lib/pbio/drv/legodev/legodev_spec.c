// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

// Known information about LEGO devices.
//
// Some specifications are determined empirically based on sensor experiments.
// Other specs are also reported by the device itself, but this information is
// not received on all platforms. This file should only be used to specify
// information required to run these devices. Optional information is omitted.

#include <pbdrv/config.h>

#include <pbdrv/legodev.h>

/**
 * Gets the minimum time needed before stale data is discarded.
 *
 * This is empirically determined based on sensor experiments.
 *
 * @param [in]  id          The device type ID.
 * @param [in]  mode        The device mode.
 * @return                  Required delay in milliseconds.
 */
uint32_t pbdrv_legodev_spec_stale_data_delay(pbdrv_legodev_type_id_t id, uint8_t mode) {
    switch (id) {
        case PBDRV_LEGODEV_TYPE_ID_COLOR_DIST_SENSOR:
            return mode == PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX ? 0 : 30;
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_COLOR_SENSOR:
            return mode == PBDRV_LEGODEV_MODE_PUP_COLOR_SENSOR__LIGHT ? 0 : 30;
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR:
            return mode == PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__LIGHT ? 0 : 50;
        #if PBDRV_CONFIG_LEGODEV_EV3DEV
        case PBDRV_LEGODEV_TYPE_ID_EV3_COLOR_SENSOR:
            return 30;
        case PBDRV_LEGODEV_TYPE_ID_EV3_IR_SENSOR:
            return 1100;
        case PBDRV_LEGODEV_TYPE_ID_NXT_LIGHT_SENSOR:
            return 20;
        case PBDRV_LEGODEV_TYPE_ID_NXT_SOUND_SENSOR:
            return 300;
        case PBDRV_LEGODEV_TYPE_ID_NXT_ENERGY_METER:
            return 200;
        #endif // PBDRV_CONFIG_LEGODEV_EV3DEV
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
 * @param [in]  id          The device type ID.
 * @param [in]  mode        The device mode.
 * @return                  Required delay in milliseconds.
 */
uint32_t pbdrv_legodev_spec_data_set_delay(pbdrv_legodev_type_id_t id, uint8_t mode) {
    // The Boost Color Distance Sensor requires a long delay or successive
    // writes are ignored.
    if (id == PBDRV_LEGODEV_TYPE_ID_COLOR_DIST_SENSOR && mode == PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX) {
        return 250;
    }

    // Default delay for setting data. In practice, this is the delay for setting
    // the light on the color sensor and ultrasonic sensor.
    return 10;
}

/**
 * Checks if the device type is a motor with an absolute encoder.
 *
 * @param [in]  id          The device type ID.
 * @return                  True if the device is an absolute encoded motor, false otherwise.
 */
static bool pbdrv_legodev_spec_device_is_abs_motor(pbdrv_legodev_type_id_t id) {
    switch (id) {
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_XL_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_M_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_L_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_S_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR:
            return true;
        default:
            return false;
    }
}

/**
 * Checks if the device type is a motor with an encoder (absolute or relative).
 *
 * @param [in]  id          The device type ID.
 * @return                  True if the device is an encoded motor, false otherwise.
 */
static bool pbdrv_legodev_spec_device_is_encoded_motor(pbdrv_legodev_type_id_t id) {

    // Absolute motors are also encoded motors.
    if (pbdrv_legodev_spec_device_is_abs_motor(id)) {
        return true;
    }

    // Relative encoded motors, including those that do not report specs.
    switch (id) {
        #if PBDRV_CONFIG_LEGODEV_EV3DEV
        case PBDRV_LEGODEV_TYPE_ID_EV3_LARGE_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_EV3_MEDIUM_MOTOR:
        #endif // PBDRV_CONFIG_LEGODEV_EV3DEV
        case PBDRV_LEGODEV_TYPE_ID_INTERACTIVE_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_MOVE_HUB_MOTOR:
            return true;
        default:
            return false;
    }
}

/**
 * Checks if the device type is a dc motor.
 *
 * @param [in]  id          The device type ID.
 * @return                  True if the device is a dc motor, false otherwise.
 */
static bool pbdrv_legodev_spec_device_is_dc_motor(pbdrv_legodev_type_id_t id) {
    switch (id) {
        case PBDRV_LEGODEV_TYPE_ID_LPF2_MMOTOR:
        case PBDRV_LEGODEV_TYPE_ID_LPF2_TRAIN:
        case PBDRV_LEGODEV_TYPE_ID_EV3DEV_DC_MOTOR:
            return true;
        default:
            return false;
    }
}

/**
 * Checks if the device type is a UART device.
 *
 * @param [in]  id          The device type ID.
 * @return                  True if the device is a LEGO UART device, false otherwise.
 */
static bool pbdrv_legodev_spec_device_is_uart_device(pbdrv_legodev_type_id_t id) {
    return id > PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART && id <= PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR;
}

/**
 * Checks if the device type is in a given category of devices.
 *
 * @param [in]  id          The device type ID.
 * @return                  True if the device is a motor, false otherwise.
 */
bool pbdrv_legodev_spec_device_category_match(pbdrv_legodev_type_id_t id, pbdrv_legodev_type_id_t category) {
    switch (category) {
        case PBDRV_LEGODEV_TYPE_ID_ANY_DC_MOTOR:
            return pbdrv_legodev_spec_device_is_dc_motor(id);
        case PBDRV_LEGODEV_TYPE_ID_ANY_ENCODED_MOTOR:
            return pbdrv_legodev_spec_device_is_encoded_motor(id);
        case PBDRV_LEGODEV_TYPE_ID_ANY_LUMP_UART:
            return pbdrv_legodev_spec_device_is_uart_device(id);
        default:
            return false;
    }
}

/**
 * Gets the desired default mode for a device.
 *
 * @param [in]  id          The device type ID.
 * @param [in]  mode        The device mode.
 * @return                  Required delay in milliseconds.
 */
uint8_t pbdrv_legodev_spec_default_mode(pbdrv_legodev_type_id_t id) {

    if (pbdrv_legodev_spec_device_is_abs_motor(id)) {
        return PBDRV_LEGODEV_MODE_PUP_ABS_MOTOR__CALIB;
    }

    switch (id) {
        case PBDRV_LEGODEV_TYPE_ID_COLOR_DIST_SENSOR:
            return PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I;
        case PBDRV_LEGODEV_TYPE_ID_INTERACTIVE_MOTOR:
            return PBDRV_LEGODEV_MODE_PUP_REL_MOTOR__POS;
        default:
            return 0;
    }
}

/**
 * Gets flags like power requirements for a given device type.
 *
 * Can be used if the device does not report its requirements or reporting is not enabled.
 *
 * @param [in]  id          The device type ID.
 * @return                  Power reqquirement capability flag.
 */
pbdrv_legodev_capability_flags_t pbdrv_legodev_spec_basic_flags(pbdrv_legodev_type_id_t id) {

    if (pbdrv_legodev_spec_device_is_abs_motor(id)) {
        return PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS;
    }

    switch (id) {
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_COLOR_SENSOR:
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR:
            return PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1;
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX:
            return PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2;
        case PBDRV_LEGODEV_TYPE_ID_INTERACTIVE_MOTOR:
            return PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS;
        default:
            return 0;
    }
}
