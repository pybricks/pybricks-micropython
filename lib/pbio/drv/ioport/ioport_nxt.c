// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_NXT

#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

pbio_error_t pbdrv_ioport_get_iodev(pbio_port_id_t port, pbio_iodev_t **iodev) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbdrv_ioport_get_motor_device_type_id(pbio_port_id_t port, pbio_iodev_type_id_t *type_id) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // no detection capabilites, so assume NXT motor (which is almost the same
    // as the EV3 large motor).
    *type_id = PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR;

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_IOPORT_NXT
