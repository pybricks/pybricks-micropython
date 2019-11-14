// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbio/error.h>

#define I2C_MAX_LEN (32)

typedef struct _smbus_t smbus_t;

pbio_error_t smbus_get(smbus_t **_bus, int bus_num);

pbio_error_t smbus_read_bytes(smbus_t *bus, uint8_t address, uint8_t reg, uint8_t len, uint8_t *buf);

pbio_error_t smbus_write_bytes(smbus_t *bus, uint8_t address, uint8_t reg, uint8_t len, uint8_t *buf);

pbio_error_t smbus_read_no_reg(smbus_t *bus, uint8_t address, uint8_t *buf);

pbio_error_t smbus_write_no_reg(smbus_t *bus, uint8_t address, uint8_t buf);

pbio_error_t smbus_read_quick(smbus_t *bus, uint8_t address);

pbio_error_t smbus_write_quick(smbus_t *bus, uint8_t address);
