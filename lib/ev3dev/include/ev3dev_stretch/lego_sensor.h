// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_LEGO_SENSOR_H_
#define _PBIO_LEGO_SENSOR_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

typedef struct _lego_sensor_t lego_sensor_t;

pbio_error_t lego_sensor_get(lego_sensor_t **sensor, pbio_port_t port, pbio_iodev_type_id_t valid_id);

pbio_error_t lego_sensor_get_info(lego_sensor_t *sensor, uint8_t *data_len, pbio_iodev_data_type_t *data_type);

pbio_error_t lego_sensor_get_bin_data(lego_sensor_t *sensor, uint8_t **bin_data);

pbio_error_t lego_sensor_get_mode(lego_sensor_t *sensor, uint8_t *mode);

pbio_error_t lego_sensor_set_mode(lego_sensor_t *sensor, uint8_t mode);

#endif // _PBIO_LEGO_SENSOR_H_
