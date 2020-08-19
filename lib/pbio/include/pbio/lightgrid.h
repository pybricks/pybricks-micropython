// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * \addtogroup Lightgrid functions
 * @{
 */

#ifndef _PBIO_LIGHTGRID_H_
#define _PBIO_LIGHTGRID_H_

#include <pbio/config.h>
#include <pbio/error.h>

typedef struct _pbio_lightgrid_t pbio_lightgrid_t;

#if PBIO_CONFIG_LIGHTGRID

/** Platform-specific data for light grid devices. */
typedef struct {
    /** PWM device id. */
    uint8_t id;
    /** The grid has size * size pixels. */
    uint8_t size;
    /** PWM channel ordered by pixel index r0c0, r0c1, .... */
    uint8_t channels[25];
} pbdrv_lightgrid_platform_data_t;

const pbdrv_lightgrid_platform_data_t pbdrv_lightgrid_platform_data;

/**
 * Get the lightgrid device
 * @param [out] lightgrid   The lightgrid object
 * @return                  ::PBIO_SUCCESS on success or
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the PWM driver is disabled.
 */
pbio_error_t pbio_lightgrid_get_dev(pbio_lightgrid_t **lightgrid);

/**
 * Get the size of the lightgrid
 * @param [in]  lightgrid   The lightgrid object
 * @return                  ::grid size
 */
uint8_t pbio_lightgrid_get_size(pbio_lightgrid_t *lightgrid);

/**
 * Sets the pixels of all rows bitwise
 * @param [in]  lightgrid   The lightgrid object
 * @param [in]  rows        Rows of bytes. Each byte is one row, LSB right.
 * @return                  ::PBIO_SUCCESS on success or
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the PWM driver is disabled.
 */
pbio_error_t pbio_lightgrid_set_rows(pbio_lightgrid_t *lightgrid, const uint8_t *rows);

/**
 * Sets the pixel to a given brightness.
 * @param [in]  lightgrid   The lightgrid object
 * @param [in]  row         Row index (0--m)
 * @param [in]  row         Column index (0--n)
 * @param [in]  brightness  Brightness (0--100)
 * @return                  ::PBIO_SUCCESS on success or
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the PWM driver is disabled.
 */
pbio_error_t pbio_lightgrid_set_pixel(pbio_lightgrid_t *lightgrid, uint8_t row, uint8_t col, uint8_t brightness);

/**
 * Sets the pixel to a given brightness.
 * @param [in]  lightgrid   The lightgrid object
 * @param [in]  image       Buffer of brightness values (0--100)
 * @return                  ::PBIO_SUCCESS on success or
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the PWM driver is disabled.
 */
pbio_error_t pbio_lightgrid_set_image(pbio_lightgrid_t *lightgrid, uint8_t *image);

#else

static inline pbio_error_t pbio_lightgrid_get_dev(pbio_lightgrid_t **lightgrid) {
    return PBIO_SUCCESS;
}
static inline uint8_t pbio_lightgrid_get_size(pbio_lightgrid_t *lightgrid) {
    return 0;
}
static inline pbio_error_t pbio_lightgrid_set_rows(pbio_lightgrid_t *lightgrid, const uint8_t *rows) {
    return PBIO_SUCCESS;
}
static inline pbio_error_t pbio_lightgrid_set_pixel(pbio_lightgrid_t *lightgrid, uint8_t row, uint8_t col, int32_t brightness) {
    return PBIO_SUCCESS;
}
static inline pbio_error_t pbio_lightgrid_set_image(pbio_lightgrid_t *lightgrid, uint8_t *image) {
    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_LIGHTGRID

#endif // _PBIO_LIGHTGRID_H_

/** @}*/
