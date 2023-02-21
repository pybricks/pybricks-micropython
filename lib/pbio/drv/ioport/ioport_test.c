// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_TEST

#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

#include "ioport_test.h"

pbio_error_t pbdrv_ioport_get_iodev(pbio_port_id_t port, pbio_iodev_t **iodev) {
    *iodev = (pbio_iodev_t *)&pbdrv_ioport_test_platform_data[port - PBDRV_CONFIG_IOPORT_TEST_FIRST_PORT].iodev;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_ioport_get_motor_device_type_id(pbio_port_id_t id, pbio_iodev_type_id_t *type_id) {
    if (id < PBDRV_CONFIG_IOPORT_TEST_FIRST_PORT || id > PBDRV_CONFIG_IOPORT_TEST_LAST_PORT) {
        return PBIO_ERROR_INVALID_ARG;
    }
    *type_id = pbdrv_ioport_test_platform_data[id - PBDRV_CONFIG_IOPORT_TEST_FIRST_PORT].iodev.info->type_id;
    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_IOPORT_TEST
