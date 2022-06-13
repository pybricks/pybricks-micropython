// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#ifndef _PBDEVICE_H_
#define _PBDEVICE_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

typedef struct _pb_device_t pb_device_t;

pb_device_t *pb_device_get_device(pbio_port_id_t port, pbio_iodev_type_id_t valid_id);

void pb_device_get_values(pb_device_t *pbdev, uint8_t mode, int32_t *values);

void pb_device_set_values(pb_device_t *pbdev, uint8_t mode, int32_t *values, uint8_t num_values);

void pb_device_set_power_supply(pb_device_t *pbdev, int32_t duty);

pbio_iodev_type_id_t pb_device_get_id(pb_device_t *pbdev);

uint8_t pb_device_get_mode(pb_device_t *pbdev);

uint8_t pb_device_get_num_values(pb_device_t *pbdev);

int8_t pb_device_get_mode_id_from_str(pb_device_t *pbdev, const char *mode_str);

/**
 * Sets up the motor/port to get it ready to be used.
 *
 * Raises MicroPython exception on error.
 *
 * @param [in]  port        The port the motor is attached to.
 * @param [in]  is_servo    True if the motor is to be used with position feedback, otherwise false.
 */
void pb_device_setup_motor(pbio_port_id_t port, bool is_servo);

#endif // _PBDEVICE_H_
