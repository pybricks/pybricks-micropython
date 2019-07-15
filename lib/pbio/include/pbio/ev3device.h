// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

#include "py/obj.h"

#include "ev3platform.h"

// TODO: Make structure like iodevice
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
     * ev3dev sysfs HAL?
     */
    ev3_platform_t platform;
} ev3_iodev_t;

pbio_error_t ev3device_get_device(ev3_iodev_t **iodev, pbio_port_t port);

pbio_error_t ev3device_get_values_at_mode(ev3_iodev_t *iodev, pbio_iodev_mode_id_t mode, void *values);
