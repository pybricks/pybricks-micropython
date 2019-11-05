// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/ev3sensor.h>
#include <pbio/ev3device.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

typedef struct _pbdrv_nxtcolor_t {
    bool initialized;
} pbdrv_nxtcolor_t;

pbdrv_nxtcolor_t nxtcolorsensors[4];

static pbio_error_t nxtcolor_init(pbdrv_nxtcolor_t *nxtcolor, pbio_port_t port) {
    printf("init sensor on port %c\n", port);
    nxtcolor->initialized = true;
    return PBIO_SUCCESS;
}

pbio_error_t nxtcolor_get_values_at_mode(pbio_port_t port, uint8_t mode, void *values) {

    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbdrv_nxtcolor_t *nxtcolor = &nxtcolorsensors[port-PBIO_PORT_1];

    if (!nxtcolor->initialized) {
        nxtcolor_init(nxtcolor, port);
    }

    return PBIO_ERROR_NOT_IMPLEMENTED;
}
