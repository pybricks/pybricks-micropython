// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_DCMOTOR

#include <inttypes.h>

#include <fixmath.h>

#include <pbdrv/config.h>
#include <pbdrv/motor.h>

#include <pbio/battery.h>
#include <pbio/dcmotor.h>

static pbio_dcmotor_t dcmotors[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_dcmotor_setup(pbio_dcmotor_t *dcmotor, pbio_direction_t direction) {

    pbio_error_t err;

    // Get device ID to ensure we are dealing with a supported device.
    err = pbdrv_motor_get_id(dcmotor->port, &dcmotor->id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Coast the device and stop any parent device using the dcmotor.
    err = pbio_dcmotor_coast(dcmotor);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Load settings for this motor
    dcmotor->max_voltage = pbio_dcmotor_get_max_voltage(dcmotor->id);

    // Set direction and state
    dcmotor->direction = direction;

    // Clear parent for this device
    pbio_parent_clear(&dcmotor->parent);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_dcmotor_get_dcmotor(pbio_port_id_t port, pbio_dcmotor_t **dcmotor) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Get pointer to dcmotor
    *dcmotor = &dcmotors[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    (*dcmotor)->port = port;

    return PBIO_SUCCESS;
}

void pbio_dcmotor_get_state(pbio_dcmotor_t *dcmotor, bool *is_coasting, int32_t *voltage_now) {
    *is_coasting = dcmotor->is_coasting;
    *voltage_now = dcmotor->voltage_now;
}

pbio_error_t pbio_dcmotor_stop(pbio_dcmotor_t *dcmotor) {
    dcmotor->is_coasting = true;
    return pbdrv_motor_coast(dcmotor->port);
}

pbio_error_t pbio_dcmotor_coast(pbio_dcmotor_t *dcmotor) {
    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&dcmotor->parent);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Stop the motor.
    return pbio_dcmotor_stop(dcmotor);
}

pbio_error_t pbio_dcmotor_set_voltage(pbio_dcmotor_t *dcmotor, int32_t voltage) {

    // Cap voltage at the configured limit.
    if (voltage > dcmotor->max_voltage) {
        voltage = dcmotor->max_voltage;
    } else if (voltage < -dcmotor->max_voltage) {
        voltage = -dcmotor->max_voltage;
    }

    // Cache value so we can read it back without touching hardware again.
    dcmotor->voltage_now = voltage;
    dcmotor->is_coasting = false;

    // Convert voltage to duty cycle.
    int32_t duty_cycle = pbio_battery_get_duty_from_voltage(voltage);

    // Flip sign if motor is inverted.
    if (dcmotor->direction == PBIO_DIRECTION_COUNTERCLOCKWISE) {
        duty_cycle = -duty_cycle;
    }

    // Apply the duty cycle.
    pbio_error_t err = pbdrv_motor_set_duty_cycle(dcmotor->port, duty_cycle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_dcmotor_set_voltage_passive(pbio_dcmotor_t *dcmotor, int32_t voltage) {

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&dcmotor->parent);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Call voltage setter that is also used for system purposes.
    return pbio_dcmotor_set_voltage(dcmotor, voltage);
}

void pbio_dcmotor_get_settings(pbio_dcmotor_t *dcmotor, int32_t *max_voltage) {
    *max_voltage = dcmotor->max_voltage;
}

pbio_error_t pbio_dcmotor_set_settings(pbio_dcmotor_t *dcmotor, int32_t max_voltage) {
    // New maximum voltage must be positive and at or below hardware limit.
    if (max_voltage < 0 || max_voltage > pbio_dcmotor_get_max_voltage(dcmotor->id)) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Set the new value.
    dcmotor->max_voltage = max_voltage;
    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_DCMOTOR
