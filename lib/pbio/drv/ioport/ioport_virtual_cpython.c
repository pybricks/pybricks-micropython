// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_VIRTUAL_CPYTHON

#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

#include "../virtual.h"

pbio_error_t pbdrv_ioport_get_iodev(pbio_port_id_t port, pbio_iodev_t **iodev) {
    return pbdrv_virtual_get_ctype_pointer("ioport", port, "iodev", (void **)iodev);
}

pbio_error_t pbdrv_ioport_get_motor_device_type_id(pbio_port_id_t port, pbio_iodev_type_id_t *type_id) {
    return pbdrv_virtual_get_u32("ioport", port, "motor_type_id", type_id);
}

#endif // PBDRV_CONFIG_IOPORT_VIRTUAL_CPYTHON
