// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_LEGO_SENSOR_H_
#define _PBIO_LEGO_SENSOR_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

/**
 * Data types used by ev3dev sensors.
 */
typedef enum {
    /**
     * Signed 8-bit integer.
     */
    LEGO_SENSOR_DATA_TYPE_INT8 = LUMP_DATA_TYPE_DATA8,
    /**
     * Little-endian, signed 16-bit integer.
     */
    LEGO_SENSOR_DATA_TYPE_INT16 = LUMP_DATA_TYPE_DATA16,
    /**
     * Little-endian, signed 32-bit integer.
     */
    LEGO_SENSOR_DATA_TYPE_INT32 = LUMP_DATA_TYPE_DATA32,
    /**
     * Little endian 32-bit floating point.
     */
    LEGO_SENSOR_DATA_TYPE_FLOAT = LUMP_DATA_TYPE_DATAF,
    /**
     * Unsigned 8-bit integer.
     */
    LEGO_SENSOR_DATA_TYPE_UINT8,
    /**
     * Unsigned 16-bit integer.
     */
    LEGO_SENSOR_DATA_TYPE_UINT16,
    /**
     * Unsigned 32-bit integer.
     */
    LEGO_SENSOR_DATA_TYPE_UINT32,
    /**
     * Big-endian, signed 16-bit integer.
     */
    LEGO_SENSOR_DATA_TYPE_INT16_BE,
} lego_sensor_data_type_t;

typedef struct _lego_sensor_t lego_sensor_t;

pbio_error_t lego_sensor_get(lego_sensor_t **sensor, pbio_port_id_t port, pbio_iodev_type_id_t valid_id);

pbio_error_t lego_sensor_get_info(lego_sensor_t *sensor, uint8_t *data_len, lego_sensor_data_type_t *data_type);

pbio_error_t lego_sensor_get_bin_data(lego_sensor_t *sensor, uint8_t **bin_data);

pbio_error_t lego_sensor_get_mode_id_from_str(lego_sensor_t *sensor, const char *mode_str, uint8_t *mode);

pbio_error_t lego_sensor_get_mode(lego_sensor_t *sensor, uint8_t *mode);

pbio_error_t lego_sensor_set_mode(lego_sensor_t *sensor, uint8_t mode);

#endif // _PBIO_LEGO_SENSOR_H_
