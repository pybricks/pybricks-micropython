// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

#include <pbio/config.h>

#if PBIO_CONFIG_TACHO

#include <inttypes.h>


#include <pbio/angle.h>
#include <pbio/int_math.h>
#include <pbio/port.h>
#include <pbio/tacho.h>
#include <pbio/port_interface.h>

/**
 * Gets the tacho angle.
 *
 * @param [in]  tacho       The tacho instance.
 * @param [out] angle       Angle in millidegrees.
 * @return                  Error code.
 */
pbio_error_t pbio_tacho_get_angle(pbio_tacho_t *tacho, pbio_angle_t *angle) {

    // First, get the raw angle from the driver.
    pbio_angle_t raw;
    pbio_error_t err = pbio_port_get_angle(tacho->port, &raw);
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

/**
 * Resets the tacho angle to a given value.
 *
 * If @p reset_to_abs is @c true, the value will be reset to the absolute angle
 * marked on the shaft if supported. In this case, @p angle serves as an output
 * so the caller knows which value it was reset to.
 *
 * @param [in]      tacho           The tacho instance.
 * @param [in,out]  angle           Angle that tacho should now report in
 *                                  millidegrees if @p reset_to_abs is @c false
 *                                  or an uninitialized value to hold the
 *                                  result if @p reset_to_abs is
 *                                  @c true.
 * @param [in]      reset_to_abs    If @c true, ignores @p angle and resets to
 *                                  absolute angle marked on shaft instead.
 * @return                          Error code.
 */
pbio_error_t pbio_tacho_reset_angle(pbio_tacho_t *tacho, pbio_angle_t *angle, bool reset_to_abs) {

    // If we reset to the absolute angle, we override the input angle. This
    // then acts as an output, so the caller knows what it was reset to.
    if (reset_to_abs) {
        // Read the absolute angle.
        pbio_error_t err = pbio_port_get_abs_angle(tacho->port, angle);
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
    pbio_error_t err = pbio_port_get_angle(tacho->port, &raw);
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

/**
 * Sets up the tacho instance to be used in an application.
 *
 * @param [in]  tacho       The tacho instance.
 * @param [in]  direction   The direction of positive rotation.
 * @param [in]  reset_angle If true, reset the current angle to the current
 *                          absolute position if supported or 0. Otherwise it
 *                          maintains its current value.
 * @return                  Error code.
 */
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
