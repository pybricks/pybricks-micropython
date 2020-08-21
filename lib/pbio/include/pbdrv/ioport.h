// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup IOPortDriver Driver: I/O Port
 * @{
 */

#ifndef _PBDRV_IOPORT_H_
#define _PBDRV_IOPORT_H_

#include <stddef.h>

#include <pbdrv/config.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

#if PBDRV_CONFIG_IOPORT

pbio_error_t pbdrv_ioport_get_iodev(pbio_port_t port, pbio_iodev_t **iodev);

#else // PBDRV_CONFIG_IOPORT

static inline pbio_error_t pbdrv_ioport_get_iodev(pbio_port_t port, pbio_iodev_t **iodev) {
    *iodev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_IOPORT

#endif // _PBDRV_IOPORT_H_

/** @}*/
