// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

#include "py/obj.h"

#include <pbdrv/ev3sensor.h>

// TODO: Make structure like iodevice
typedef struct _pbio_ev3iodev_t {
    /**
     * The device ID
     */
    pbio_iodev_type_id_t type_id;
    /**
     * The port the device is attached to.
     */
    pbio_port_t port;
    /**
     * The current active mode.
     */
    pbio_iodev_mode_id_t mode;
    /**
     * The current active mode.
     */
    uint8_t data_len;
    /**
     * The current active mode.
     */
    pbio_iodev_data_type_t data_type;
    /**
     * Platform specific low-level device abstraction
     */
    pbdrv_ev3_sensor_t *sensor;
} pbio_ev3iodev_t;

pbio_error_t ev3device_get_device(pbio_ev3iodev_t **iodev, pbio_port_t port);

pbio_error_t ev3device_get_values_at_mode(pbio_ev3iodev_t *iodev, pbio_iodev_mode_id_t mode, void *values);
