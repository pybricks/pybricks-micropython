// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBSMBUS_H_
#define _PBSMBUS_H_

#include <stdint.h>
#if PB_HAVE_LIBI2C
#include <i2c/smbus.h>
#else
#include <linux/i2c-dev.h>
#endif
#include <pbio/error.h>

#define PB_SMBUS_BLOCK_MAX I2C_SMBUS_BLOCK_MAX

typedef struct _smbus_t smbus_t;

pbio_error_t pb_smbus_get(smbus_t **_bus, int bus_num);

pbio_error_t pb_smbus_read_bytes(smbus_t *bus, uint8_t address, uint8_t reg, uint8_t len, uint8_t *buf);

pbio_error_t pb_smbus_write_bytes(smbus_t *bus, uint8_t address, uint8_t reg, uint8_t len, uint8_t *buf);

pbio_error_t pb_smbus_read_no_reg(smbus_t *bus, uint8_t address, uint8_t *buf);

pbio_error_t pb_smbus_write_no_reg(smbus_t *bus, uint8_t address, uint8_t buf);

pbio_error_t pb_smbus_read_quick(smbus_t *bus, uint8_t address);

pbio_error_t pb_smbus_write_quick(smbus_t *bus, uint8_t address);

#endif /* _PBSMBUS_H_ */
