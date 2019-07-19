// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

#include "py/obj.h"

typedef struct _ev3_platform_t ev3_platform_t;

pbio_error_t ev3_sensor_get_platform(ev3_platform_t **pplatform, pbio_port_t port);

pbio_error_t ev3_sensor_get_id(ev3_platform_t *platform, pbio_iodev_type_id_t *id);

pbio_error_t ev3_sensor_get_info(ev3_platform_t *platform, uint8_t *data_len, pbio_iodev_data_type_t *data_type);

pbio_error_t ev3_sensor_get_bin_data(ev3_platform_t *platform, char *bin_data);

pbio_error_t ev3_sensor_set_mode(ev3_platform_t *platform, pbio_iodev_mode_id_t mode);
