// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_EV3SENSORS_H_
#define _PBIO_EV3SENSORS_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

#include "py/obj.h"

typedef struct _pbdrv_ev3_sensor_t pbdrv_ev3_sensor_t;

pbio_error_t pbdrv_ev3_sensor_get(pbdrv_ev3_sensor_t **sensor, pbio_port_t port);

pbio_error_t pbdrv_ev3_sensor_assert_id(pbdrv_ev3_sensor_t *sensor, pbio_iodev_type_id_t valid_id);

pbio_error_t pbdrv_ev3_sensor_get_info(pbdrv_ev3_sensor_t *sensor, uint8_t *data_len, pbio_iodev_data_type_t *data_type);

pbio_error_t pbdrv_ev3_sensor_get_bin_data(pbdrv_ev3_sensor_t *sensor, char *bin_data);

pbio_error_t pbdrv_ev3_sensor_set_mode(pbdrv_ev3_sensor_t *sensor, uint8_t mode);

#endif // _PBIO_EV3SENSORS_H_
