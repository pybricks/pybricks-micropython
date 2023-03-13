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

#include <pbio/angle.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/dcmotor.h>
#include <pbio/port.h>

/** A rotation sensor object. */
typedef struct _pbio_tacho_t pbio_tacho_t;

#if PBIO_CONFIG_TACHO

/** @name Initialization Functions */
/**@{*/
pbio_error_t pbio_tacho_get_tacho(pbio_port_id_t port, pbio_tacho_t **tacho);
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

static inline pbio_error_t pbio_tacho_get_tacho(pbio_port_id_t port, pbio_tacho_t **tacho) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

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
