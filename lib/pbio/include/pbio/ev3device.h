// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

#include "py/obj.h"

#include "ev3platform.h"

typedef struct _ev3_iodev_t {
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
    uint8_t mode;
    /**
     * ev3dev sysfs HAL?
     */
    ev3_platform_t platform;
} ev3_iodev_t;

pbio_error_t ev3device_get_device(ev3_iodev_t **iodev, pbio_port_t port);

pbio_error_t ev3device_get_values_at_mode(ev3_iodev_t *iodev, pbio_iodev_type_id_t valid_id, uint8_t mode, void *values);
