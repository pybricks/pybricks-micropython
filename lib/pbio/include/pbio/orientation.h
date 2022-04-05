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
    PBIO_ORIENTATION_SIDE_FRONT =  (0 << 2) | 0,  /**< +X: The front side of a rectangular box */
    PBIO_ORIENTATION_SIDE_LEFT =   (0 << 2) | 1,  /**< +Y: The left side of a rectangular box or screen */
    PBIO_ORIENTATION_SIDE_TOP =    (0 << 2) | 2,  /**< +Z: The top side of a rectangular box or screen */
    PBIO_ORIENTATION_SIDE_BACK =   (1 << 2) | 0,  /**< -X: The back side of a rectangular box */
    PBIO_ORIENTATION_SIDE_RIGHT =  (1 << 2) | 1,  /**< -Y: The right side of a rectangular box or screen */
    PBIO_ORIENTATION_SIDE_BOTTOM = (1 << 2) | 2,  /**< -Z: The bottom side of a rectangular box or screen */
} pbio_orientation_side_t;

#if PBIO_CONFIG_ORIENTATION

void pbio_orientation_side_get_axis(pbio_orientation_side_t side, uint8_t *index, int8_t *sign);

void pbio_orientation_get_complementary_axis(uint8_t *index, int8_t *sign);

#else // PBIO_CONFIG_ORIENTATION

static inline void pbio_orientation_side_get_axis(pbio_orientation_side_t side, uint8_t *index, int8_t *sign) {
}

static inline void pbio_orientation_get_complementary_axis(uint8_t *index, int8_t *sign) {
}

#endif // PBIO_CONFIG_ORIENTATION

#endif // _PBIO_ORIENTATION_H_

/** @} */
