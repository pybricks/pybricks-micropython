// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_EV3DEV_STRETCH

#include <ev3dev_stretch/lego_motor.h>

#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

pbio_error_t pbdrv_ioport_get_iodev(pbio_port_id_t port, pbio_iodev_t **iodev) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbdrv_ioport_get_motor_device_type_id(pbio_port_id_t port, pbio_iodev_type_id_t *type_id) {
    return ev3dev_motor_get_id(port, type_id);
}

#endif // PBDRV_CONFIG_IOPORT_EV3DEV_STRETCH
