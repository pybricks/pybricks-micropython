// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

#include "/usr/include/linux/i2c-dev.h"

#include <pbio/error.h>

#include "modsmbus.h"

#define MAXDEVPATH (16)
#define BUS_NUM_MIN (3)
#define BUS_NUM_MAX (6)

struct _smbus_t {
    int file;
    int address;
};

smbus_t buses[BUS_NUM_MAX-BUS_NUM_MIN+1];

static pbio_error_t smbus_set_address(smbus_t *bus, int address) {
    if (bus->address != address) {
        if (ioctl(bus->file, I2C_SLAVE, address) != 0) {
            return PBIO_ERROR_IO;
        }
        bus->address = address;
    }
    return PBIO_SUCCESS;
}

pbio_error_t smbus_get(smbus_t **_bus, int bus_num) {
    if (bus_num < BUS_NUM_MIN || bus_num > BUS_NUM_MAX) {
        return PBIO_ERROR_INVALID_PORT;
    }

    smbus_t *bus = &buses[bus_num-BUS_NUM_MIN];

    char devpath[MAXDEVPATH];

    if (snprintf(devpath, MAXDEVPATH, "/dev/i2c-%d", bus_num) >= MAXDEVPATH) {
        return PBIO_ERROR_IO;
    }

    bus->file = open(devpath, O_RDWR, 0);
    bus->address = -1;

    if (bus->file == -1) {
        return PBIO_ERROR_IO;
    }

    *_bus = bus;

    return PBIO_SUCCESS;
}

pbio_error_t smbus_read_bytes(smbus_t *bus, uint8_t address, uint8_t reg, uint8_t len, uint8_t *buf) {

    pbio_error_t err = smbus_set_address(bus, address);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int rclen = i2c_smbus_read_i2c_block_data(bus->file, reg, len, buf);
    if (rclen != len) {
        return PBIO_ERROR_IO;
    }
    return PBIO_SUCCESS;
}

pbio_error_t smbus_write_bytes(smbus_t *bus, uint8_t address, uint8_t reg, uint8_t len, uint8_t *buf) {

    pbio_error_t err = smbus_set_address(bus, address);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (i2c_smbus_write_i2c_block_data(bus->file, reg, len, buf) != 0) {
        return PBIO_ERROR_IO;
    }
    return PBIO_SUCCESS;
}
