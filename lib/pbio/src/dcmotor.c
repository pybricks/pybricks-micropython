// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_DCMOTOR

#include <stdbool.h>
#include <inttypes.h>

#include <fixmath.h>

#include <pbdrv/config.h>
#include <pbdrv/ioport.h>
#include <pbdrv/motor_driver.h>
#include <pbio/battery.h>
#include <pbio/dcmotor.h>
#include <pbio/error.h>
#include <pbio/port.h>

#if PBIO_BATTERY_MAX_DUTY != PBDRV_MOTOR_DRIVER_MAX_DUTY
#error "this file is written with the assumption that we can pass battery duty to motor driver without scaling"
#endif

static pbio_dcmotor_t dcmotors[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

// Stop all motors and their parent objects.
void pbio_dcmotor_stop_all(bool clear_parents) {

    // Go through all ports.
    for (pbio_port_id_t port = PBDRV_CONFIG_FIRST_MOTOR_PORT; port <= PBDRV_CONFIG_LAST_MOTOR_PORT; port++) {

        // Get device from port index.
        pbio_dcmotor_t *dcmotor;
        pbio_error_t err = pbio_dcmotor_get_dcmotor(port, &dcmotor);
        if (err != PBIO_SUCCESS) {
            continue;
        }

        // Get the device type ID to ensure we are dealing with a motor.
        pbio_iodev_type_id_t type_id;
        err = pbdrv_ioport_get_motor_device_type_id(port, &type_id);
        if (err != PBIO_SUCCESS) {
            // It's something other than a motor, so don't touch it.
            continue;
        }

        // Coast the motor and let errors pass.
        pbio_dcmotor_coast(dcmotor);

        // Stop its parents and let errors pass. Optionally resets parent
        // objects to free up this motor for use in new objects.
        pbio_parent_stop(&dcmotor->parent, clear_parents);
    }
}

/**
 * Sets up the DC motor instance to be used in an application.
 *
 * @param [in]  dcmotor     The DC motor instance.
 * @param [in]  direction   The direction of positive rotation.
 */
pbio_error_t pbio_dcmotor_setup(pbio_dcmotor_t *dcmotor, pbio_direction_t direction) {

    // Get the device type ID to ensure we are dealing with a motor.
    pbio_iodev_type_id_t type_id;
    pbio_error_t err = pbdrv_ioport_get_motor_device_type_id(dcmotor->port, &type_id);
    if (err != PBIO_SUCCESS || type_id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }

    // If the device already has a parent, we shouldn't allow this device
    // to be used as a new object.
    if (pbio_parent_exists(&dcmotor->parent)) {
        return PBIO_ERROR_BUSY;
    }

    // Coast the device.
    err = pbio_dcmotor_coast(dcmotor);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Load settings for this motor
    dcmotor->max_voltage = pbio_dcmotor_get_max_voltage(type_id);

    // Set direction and state
    dcmotor->direction = direction;

    return PBIO_SUCCESS;
}

/**
 * Gets the DC motor instance for the specified port.
 *
 * @param [in]  port        The port the motor is connected to.
 * @param [out] dcmotor     The motor instance.
 * @return                  Error code.
 */
pbio_error_t pbio_dcmotor_get_dcmotor(pbio_port_id_t port, pbio_dcmotor_t **dcmotor) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    *dcmotor = &dcmotors[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

    // if this is the first time getting the device, we need to get the motor
    // driver instance
    if ((*dcmotor)->motor_driver == NULL) {
        // since there is no dcmotor module init, we init the port here
        (*dcmotor)->port = port;
        // REVISIT: this assumes that the motor driver id cooresponds to the port
        pbio_error_t err = pbdrv_motor_driver_get_dev(port - PBDRV_CONFIG_FIRST_MOTOR_PORT, &(*dcmotor)->motor_driver);

        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    return PBIO_SUCCESS;
}

void pbio_dcmotor_get_state(pbio_dcmotor_t *dcmotor, pbio_dcmotor_actuation_t *actuation, int32_t *voltage_now) {
    *actuation = dcmotor->actuation_now;
    *voltage_now = dcmotor->voltage_now;
}

int32_t pbio_dcmotor_get_max_voltage(pbio_iodev_type_id_t id) {
    if (id == PBIO_IODEV_TYPE_ID_SPIKE_S_MOTOR) {
        return 6000;
    }
    return 9000;
}

pbio_error_t pbio_dcmotor_coast(pbio_dcmotor_t *dcmotor) {
    // Stop the motor and set the passivity state value for data logging.
    dcmotor->actuation_now = PBIO_DCMOTOR_ACTUATION_COAST;
    dcmotor->voltage_now = 0;
    return pbdrv_motor_driver_coast(dcmotor->motor_driver);
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
    dcmotor->actuation_now = PBIO_DCMOTOR_ACTUATION_VOLTAGE;

    // Convert voltage to duty cycle.
    int32_t duty_cycle = pbio_battery_get_duty_from_voltage(voltage);

    // Flip sign if motor is inverted.
    if (dcmotor->direction == PBIO_DIRECTION_COUNTERCLOCKWISE) {
        duty_cycle = -duty_cycle;
    }

    // Apply the duty cycle.
    pbio_error_t err = pbdrv_motor_driver_set_duty_cycle(dcmotor->motor_driver, duty_cycle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_dcmotor_user_command(pbio_dcmotor_t *dcmotor, bool coast, int32_t voltage) {
    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&dcmotor->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Coast if this command was given.
    if (coast) {
        return pbio_dcmotor_coast(dcmotor);
    }
    // Otherwise set a voltage.
    return pbio_dcmotor_set_voltage(dcmotor, voltage);
}


void pbio_dcmotor_get_settings(pbio_dcmotor_t *dcmotor, int32_t *max_voltage) {
    *max_voltage = dcmotor->max_voltage;
}

pbio_error_t pbio_dcmotor_set_settings(pbio_dcmotor_t *dcmotor, int32_t max_voltage) {

    // Get the device type.
    pbio_iodev_type_id_t type_id;
    pbio_error_t err = pbdrv_ioport_get_motor_device_type_id(dcmotor->port, &type_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // New maximum voltage must be positive and at or below hardware limit.
    if (max_voltage < 0 || max_voltage > pbio_dcmotor_get_max_voltage(type_id)) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Set the new value.
    dcmotor->max_voltage = max_voltage;
    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_DCMOTOR
