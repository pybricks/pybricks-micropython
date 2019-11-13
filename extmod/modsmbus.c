// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include <pbio/error.h>

#define MAXDEVPATH (16)
#define BUS_NUM_MIN (3)
#define BUS_NUM_MAX (6)

typedef struct _smbus_t {
    int file;
    int address;
} smbus_t;

smbus_t buses[BUS_NUM_MAX-BUS_NUM_MIN+1];

pbio_error_t smbus_get(smbus_t **_bus, int bus_num)
{
    if (bus_num < BUS_NUM_MIN || bus_num > BUS_NUM_MAX) {
        return PBIO_ERROR_INVALID_PORT;
    }

    smbus_t *bus = &buses[bus_num-BUS_NUM_MIN];

    char devpath[MAXDEVPATH];

    if (snprintf(devpath, MAXDEVPATH, "/dev/i2c-%d", bus_num) >= MAXDEVPATH) {
        return PBIO_ERROR_IO;
    }

    bus->file = open(devpath, O_RDWR, 0);

    if (bus->file == -1) {
        return PBIO_ERROR_IO;
    }

    *_bus = bus;

    return PBIO_SUCCESS;
}
