// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_TACHO

#include <inttypes.h>

#include <pbdrv/counter.h>

#include <pbio/angle.h>
#include <pbio/math.h>
#include <pbio/port.h>
#include <pbio/tacho.h>

/**
 * The tacho module is a wrapper around a counter driver to provide rotation
 * sensing with a configurable positive direction and zero point, without
 * poking at hardware to reset it. The output angle is defined as:
 *
 *     angle = (raw_angle - zero_angle) * direction.
 *
 * Here direction is +1 if the tacho should increase for increasing raw angles
 * or -1 if it should decrease for increasing raw angles.
 */
struct _pbio_tacho_t {
    pbio_direction_t direction;  /**< Direction of tacho for increasing raw driver counts. */
    pbio_angle_t zero_angle;     /**< Raw angle where tacho output angle reads zero. */
    pbdrv_counter_dev_t *counter;
};

static pbio_tacho_t tachos[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_tacho_get_tacho(pbio_port_id_t port, pbio_tacho_t **tacho) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Get pointer to tacho.
    *tacho = &tachos[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

    // Get counter device
    return pbdrv_counter_get_dev(port - PBDRV_CONFIG_FIRST_MOTOR_PORT, &((*tacho)->counter));
}

pbio_error_t pbio_tacho_get_angle(pbio_tacho_t *tacho, pbio_angle_t *angle) {

    // First, get the raw angle from the driver.
    pbio_angle_t raw;
    pbio_error_t err = pbdrv_counter_get_angle(tacho->counter, &raw.rotations, &raw.millidegrees);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get angle offset by zero point.
    pbio_angle_diff(&raw, &tacho->zero_angle, angle);

    // Negate result depending on chosen positive direction.
    if (tacho->direction == PBIO_DIRECTION_COUNTERCLOCKWISE) {
        pbio_angle_neg(angle);
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbio_tacho_reset_angle(pbio_tacho_t *tacho, pbio_angle_t *angle, bool reset_to_abs) {

    // If we reset to the absolute angle, we override the input angle. This
    // then acts as an output, so the caller knows what it was reset to.
    if (reset_to_abs) {
        // Read the absolute angle.
        pbio_error_t err = pbdrv_counter_get_abs_angle(tacho->counter, &angle->millidegrees);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Negate the absolute value if requested.
        if (tacho->direction == PBIO_DIRECTION_COUNTERCLOCKWISE) {
            angle->millidegrees *= -1;
        }
        // Rotations is always zero for the absolute angle.
        angle->rotations = 0;
    }

    // Next, get the raw angle from the driver.
    pbio_angle_t raw;
    pbio_error_t err = pbdrv_counter_get_angle(tacho->counter, &raw.rotations, &raw.millidegrees);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // From the above definition, we can set the new zero point as:
    //
    //    zero_angle = raw_angle - angle / direction
    //
    if (tacho->direction == PBIO_DIRECTION_COUNTERCLOCKWISE) {
        // direction = -1, so as per above we should add.
        pbio_angle_sum(&raw, angle, &tacho->zero_angle);
    } else {
        // direction = +1, so as per above we should subtract.
        pbio_angle_diff(&raw, angle, &tacho->zero_angle);
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbio_tacho_setup(pbio_tacho_t *tacho, pbio_direction_t direction, bool reset_angle) {

    pbio_angle_t zero = {0};

    // Configure direction
    tacho->direction = direction;

    // If there's no need to reset the angle, we are done here.
    if (!reset_angle) {
        // We still do one test read to ensure a tacho exists.
        return pbio_tacho_get_angle(tacho, &zero);
    }

    // Reset counter if requested. Try absolute reset first.
    pbio_error_t err = pbio_tacho_reset_angle(tacho, &zero, true);
    if (err == PBIO_ERROR_NOT_SUPPORTED) {
        // If not available, set it to 0.
        err = pbio_tacho_reset_angle(tacho, &zero, false);
    }
    return err;
}

#endif // PBIO_CONFIG_TACHO
