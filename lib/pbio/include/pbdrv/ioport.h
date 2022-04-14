// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

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

pbio_error_t pbdrv_ioport_get_iodev(pbio_port_id_t port, pbio_iodev_t **iodev);

/**
 * Gets the device type id of the attached motor or light, or lack thereof.
 * @param [in]  port    The port the motor is attached to.
 * @param [in]  type_id The type id of the device attached to the port.
 * @return              ::PBIO_SUCCESS if a motor, a light, or nothing is attached to this driver
 *                      ::PBIO_ERROR_INVALID_OP if something else is attached
 */
pbio_error_t pbdrv_ioport_get_motor_device_type_id(pbio_port_id_t port, pbio_iodev_type_id_t *type_id);

#else // PBDRV_CONFIG_IOPORT

static inline pbio_error_t pbdrv_ioport_get_iodev(pbio_port_id_t port, pbio_iodev_t **iodev) {
    *iodev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_ioport_get_motor_device_type_id(pbio_port_id_t port, pbio_iodev_type_id_t *type_id) {
    *type_id = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_IOPORT

#endif // _PBDRV_IOPORT_H_

/** @} */
