// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_VIRTUAL

#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

#include "../virtual.h"

pbio_error_t pbdrv_ioport_get_iodev(pbio_port_id_t port, pbio_iodev_t **iodev) {
    return pbdrv_virtual_get_ctype_pointer("ioport", port, "iodev", (void **)iodev);
}

#endif // PBDRV_CONFIG_IOPORT_VIRTUAL
