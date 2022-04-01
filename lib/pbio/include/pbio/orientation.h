// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup Orientation Orientation functions
 *
 * Provides functions for accessing hub orientation.
 * @{
 */

#ifndef _PBIO_ORIENTATION_H_
#define _PBIO_ORIENTATION_H_

#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>

/**
 * Identifier for one side of a rectangle (e.g. screen) or box (e.g. a hub).
 */
typedef enum {
    PBIO_ORIENTATION_SIDE_TOP,       /**< The top side of a rectangular box or screen */
    PBIO_ORIENTATION_SIDE_LEFT,      /**< The left side of a rectangular box or screen */
    PBIO_ORIENTATION_SIDE_BOTTOM,    /**< The bottom side of a rectangular box or screen */
    PBIO_ORIENTATION_SIDE_RIGHT,     /**< The right side of a rectangular box or screen */
    PBIO_ORIENTATION_SIDE_FRONT,     /**< The front side of a rectangular box */
    PBIO_ORIENTATION_SIDE_BACK,      /**< The back side of a rectangular box */
} pbio_orientation_side_t;

#if PBIO_CONFIG_ORIENTATION

#else // PBIO_CONFIG_ORIENTATION

#endif // PBIO_CONFIG_ORIENTATION

#endif // _PBIO_ORIENTATION_H_

/** @} */
