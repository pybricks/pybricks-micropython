// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

/**
 * @addtogroup Tacho pbio/tacho: Rotation sensor interface
 *
 * Extends a rotation sensor with a configurable positive direction and zero
 * point without resetting hardware.
 * @{
 */

#ifndef _PBIO_TACHO_H_
#define _PBIO_TACHO_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/counter.h>

#include <pbio/angle.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/dcmotor.h>
#include <pbio/port.h>

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
typedef struct {;
                pbio_direction_t direction; /**< Direction of tacho for increasing raw driver counts. */
                pbio_angle_t zero_angle; /**< Raw angle where tacho output angle reads zero. */
                pbio_port_t *port;
} pbio_tacho_t;

#if PBIO_CONFIG_TACHO

/** @name Initialization Functions */
/**@{*/
pbio_error_t pbio_tacho_setup(pbio_tacho_t *tacho, pbio_direction_t direction, bool reset_angle);
/**@}*/

/** @name Status Functions */
/**@{*/
pbio_error_t pbio_tacho_get_angle(pbio_tacho_t *tacho, pbio_angle_t *angle);
/**@}*/

/** @name Operation Functions */
/**@{*/
pbio_error_t pbio_tacho_reset_angle(pbio_tacho_t *tacho, pbio_angle_t *reset_angle, bool reset_to_abs);
/**@}*/

#else

static inline pbio_error_t pbio_tacho_setup(pbio_tacho_t *tacho, pbio_direction_t direction, bool reset_angle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_tacho_get_angle(pbio_tacho_t *tacho, pbio_angle_t *angle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_tacho_reset_angle(pbio_tacho_t *tacho, pbio_angle_t *reset_angle, bool reset_to_abs) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBIO_CONFIG_TACHO

#endif // _PBIO_TACHO_H_

/** @} */
