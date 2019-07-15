// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

#include "py/obj.h"

typedef struct _ev3_platform_t {
    int n_sensor;
    FILE *f_mode;
    FILE *f_driver_name;
    FILE *f_bin_data;
} ev3_platform_t;

pbio_error_t ev3_sensor_init(ev3_platform_t *platform, pbio_port_t port);

pbio_error_t ev3_sensor_get_id(ev3_platform_t *platform, pbio_iodev_type_id_t *id);

pbio_error_t ev3_sensor_get_bin_data(ev3_platform_t *platform, char *bin_data);
