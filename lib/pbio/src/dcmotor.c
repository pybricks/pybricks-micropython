// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

#include <pbio/config.h>

#if PBIO_CONFIG_DCMOTOR

#include <stdbool.h>
#include <inttypes.h>

#include <pbdrv/config.h>
#include <pbdrv/motor_driver.h>

#include <pbio/battery.h>
#include <pbio/dcmotor.h>
#include <pbio/int_math.h>
#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/servo.h>

#if PBIO_BATTERY_MAX_DUTY != PBDRV_MOTOR_DRIVER_MAX_DUTY
#error "this file is written with the assumption that we can pass battery duty to motor driver without scaling"
#endif

static pbio_dcmotor_t dcmotors[PBIO_CONFIG_DCMOTOR_NUM_DEV];

/**
 * Initializes DC motor state structure.
 *
 * @param [in]  index       The index of the DC motor.
 * @param [in]  motor_driver The motor driver instance.
 */
pbio_dcmotor_t *pbio_dcmotor_init_instance(uint8_t index, pbdrv_motor_driver_dev_t *motor_driver) {
    if (index >= PBIO_CONFIG_DCMOTOR_NUM_DEV) {
        return NULL;
    }
    pbio_dcmotor_t *dcmotor = &dcmotors[index];
    dcmotor->motor_driver = motor_driver;
    return dcmotor;
}

/**
 * Stops a motor and all higher level parent objects that use it.
 *
 * @param [in]  dcmotor       The DC motor instance.
 * @param [in]  clear_parent  Whether to not only stop the parent object
 *                            physically, but also clear the dcmotor's
 *                            knowledge about that object. Choosing true frees
 *                            up all motors to be used again by new parent
 *                            objects.
 */
void pbio_dcmotor_reset(pbio_dcmotor_t *dcmotor, bool clear_parent) {
    // Coast the motor and let errors pass.
    pbio_dcmotor_coast(dcmotor);

    // Stop its parents and let errors pass. Optionally resets parent
    // objects to free up this motor for use in new objects.
    pbio_parent_stop(&dcmotor->parent, clear_parent);
}

/**
 * Tests if all dc motors are coasting.
 *
 * @return                  @c true if all motors are coasting, @c false otherwise.
 */
bool pbio_dcmotor_all_coasting(void) {
    for (uint8_t i = 0; i < PBIO_CONFIG_DCMOTOR_NUM_DEV; i++) {
        if (dcmotors[i].actuation_now != PBIO_DCMOTOR_ACTUATION_COAST) {
            return false;
        }
    }
    return true;
}

/**
 * Stops and closes DC motor instance so it can be used in another application.
 *
 * @param [in]  dcmotor     The DC motor instance.
 * @return                  Error code.
 */
pbio_error_t pbio_dcmotor_close(pbio_dcmotor_t *dcmotor) {

    // Coast the motor and remember error.
    pbio_error_t stop_err = pbio_dcmotor_coast(dcmotor);

    // Stop its parents and reset parent objects to free up this motor
    // for use in new objects, even if stopping has failed.
    pbio_error_t parent_err = pbio_parent_stop(&dcmotor->parent, true);

    // Return original error corresponding to stopping the motor.
    if (stop_err != PBIO_SUCCESS) {
        return stop_err;
    }

    // Return error of trying to stop parent structure.
    return parent_err;
}

/**
 * Sets up the DC motor instance to be used in an application.
 *
 * @param [in]  dcmotor     The DC motor instance.
 * @param [in]  type        The type of motor.
 * @param [in]  direction   The direction of positive rotation.
 */
pbio_error_t pbio_dcmotor_setup(pbio_dcmotor_t *dcmotor, lego_device_type_id_t type, pbio_direction_t direction) {

    // If the device already has a parent, we shouldn't allow this device
    // to be used as a new object.
    if (pbio_parent_exists(&dcmotor->parent)) {
        return PBIO_ERROR_BUSY;
    }

    // Coast the device.
    pbio_error_t err = pbio_dcmotor_coast(dcmotor);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Load settings for this motor
    dcmotor->max_voltage = pbio_servo_get_max_voltage(type);
    dcmotor->max_voltage_hardware = dcmotor->max_voltage;

    // Set direction and state
    dcmotor->direction = direction;

    return PBIO_SUCCESS;
}

/**
 * Gets the actuation type that is currently being applied to the motor.
 *
 * @param [in]  dcmotor     The dcmotor instance.
 * @param [out] actuation   Actuation type, ::PBIO_DCMOTOR_ACTUATION_COAST
 *                          or ::PBIO_DCMOTOR_ACTUATION_VOLTAGE.
 * @param [out] voltage_now Voltage in mV if @ actuation is voltage. Else 0.
 */
void pbio_dcmotor_get_state(const pbio_dcmotor_t *dcmotor, pbio_dcmotor_actuation_t *actuation, int32_t *voltage_now) {
    *actuation = dcmotor->actuation_now;
    *voltage_now = dcmotor->voltage_now;
}

/**
 * Coasts the dc motor.
 *
 * This just sets the physical state and does not stop parent objects. It
 * should only be used by higher level objects. Users should use
 * ::pbio_dcmotor_user_command instead.
 *
 * @param [in] dcmotor      The motor instance.
 * @return                  Error code.
 */
pbio_error_t pbio_dcmotor_coast(pbio_dcmotor_t *dcmotor) {
    // Stop the motor and set the passivity state value for data logging.
    dcmotor->actuation_now = PBIO_DCMOTOR_ACTUATION_COAST;
    dcmotor->voltage_now = 0;
    return pbdrv_motor_driver_coast(dcmotor->motor_driver);
}

/**
 * Sets the dc motor voltage.
 *
 * A positive voltage will cause a rotation in the direction configured in
 * ::pbio_dcmotor_setup. A negative voltage makes it turn in the other way.
 *
 * This just sets the physical state and does not stop parent objects. It
 * should only be used by higher level objects. Users should use
 * ::pbio_dcmotor_user_command instead.
 *
 * @param [in] dcmotor      The motor instance.
 * @param [in] voltage      The voltage in mV.
 * @return                  Error code.
 */
pbio_error_t pbio_dcmotor_set_voltage(pbio_dcmotor_t *dcmotor, int32_t voltage) {
    // Cap voltage at the configured limit.
    voltage = pbio_int_math_clamp(voltage, dcmotor->max_voltage);

    // Cache value so we can read state without touching hardware again.
    dcmotor->voltage_now = voltage;
    dcmotor->actuation_now = PBIO_DCMOTOR_ACTUATION_VOLTAGE;

    // Convert voltage to duty cycle.
    int32_t duty_cycle = pbio_battery_get_duty_from_voltage(voltage);

    // Flip sign if motor is inverted.
    if (dcmotor->direction == PBIO_DIRECTION_COUNTERCLOCKWISE) {
        duty_cycle = -duty_cycle;
    }

    // Apply the duty cycle.
    pbio_error_t err = pbdrv_motor_driver_set_duty_cycle(dcmotor->motor_driver, (int16_t)duty_cycle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}

/**
 * Sets a voltage or coasts the motor, and stops higher level objects.
 *
 * A positive voltage will cause a rotation in the direction configured in
 * ::pbio_dcmotor_setup. A negative voltage makes it turn in the other way.
 *
 * @param [in] dcmotor      The motor instance.
 * @param [in] coast        If true, the motor coasts. If false, applies the
 *                          given voltage.
 * @param [in] voltage      The voltage in mV.
 * @return                  Error code.
 */
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

/**
 * Gets the settings for this motor.
 *
 * @param [in]  dcmotor       The motor instance.
 * @param [out] max_voltage   The configured maximum voltage for this motor.
 */
void pbio_dcmotor_get_settings(const pbio_dcmotor_t *dcmotor, int32_t *max_voltage) {
    *max_voltage = dcmotor->max_voltage;
}

/**
 * Sets the settings for this motor.
 *
 * @param [in] dcmotor      The motor instance.
 * @param [in] max_voltage  Maximum voltage (mV) for this motor.
 * @return                  Error code.
 */
pbio_error_t pbio_dcmotor_set_settings(pbio_dcmotor_t *dcmotor, int32_t max_voltage) {

    // New maximum voltage must be positive and at or below hardware limit.
    if (max_voltage < 0 || max_voltage > dcmotor->max_voltage_hardware) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Set the new value.
    dcmotor->max_voltage = max_voltage;
    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_DCMOTOR
