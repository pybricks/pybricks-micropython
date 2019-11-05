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

pbio_error_t nxtcolor_get_values_at_mode(pbio_port_t port, uint8_t mode, void *values) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}
