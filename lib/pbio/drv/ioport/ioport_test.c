// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_TEST

#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

pbio_error_t pbdrv_ioport_get_iodev(pbio_port_id_t port, pbio_iodev_t **iodev) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbdrv_ioport_get_motor_device_type_id(pbio_port_id_t id, pbio_iodev_type_id_t *type_id) {
    *type_id = PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR;
    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_IOPORT_TEST
