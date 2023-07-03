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
 * Gets the desired default mode for a device.
 *
 * @param [in]  id          The device type ID.
 * @param [in]  mode        The device mode.
 * @return                  Required delay in milliseconds.
 */
uint8_t pbdrv_legodev_spec_default_mode(pbdrv_legodev_type_id_t id) {
    switch (id) {
        case PBDRV_LEGODEV_TYPE_ID_COLOR_DIST_SENSOR:
            return PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I;
        case PBDRV_LEGODEV_TYPE_ID_INTERACTIVE_MOTOR:
            return PBDRV_LEGODEV_MODE_PUP_REL_MOTOR__POS;
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_M_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_L_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_S_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_XL_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR:
            return PBDRV_LEGODEV_MODE_PUP_ABS_MOTOR__CALIB;
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
    switch (id) {
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_COLOR_SENSOR:
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR:
            return PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1;
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX:
            return PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2;
        case PBDRV_LEGODEV_TYPE_ID_INTERACTIVE_MOTOR:
            return PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS;
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_M_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_L_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_SPIKE_S_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_XL_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR:
        case PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR:
            return PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS;
        default:
            return 0;
    }
}
