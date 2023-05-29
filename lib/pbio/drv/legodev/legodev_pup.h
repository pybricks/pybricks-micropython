// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_LEGODEV_PUP_H_
#define _INTERNAL_PBDRV_LEGODEV_PUP_H_

#include <pbdrv/config.h>

#include "legodev_pup_uart.h"

#include <pbdrv/legodev.h>

#include <pbio/port.h>
#include <pbdrv/legodev.h>

typedef struct {
    pbio_port_id_t port_id;
    pbdrv_legodev_type_id_t type_id;
    uint8_t motor_driver_index;
    uint8_t quadrature_index;
} pbdrv_legodev_pup_int_platform_data_t;

typedef struct {
    pbio_port_id_t port_id;
    uint8_t ioport_index;
} pbdrv_legodev_pup_ext_platform_data_t;

#if PBDRV_CONFIG_LEGODEV_PUP

extern const pbdrv_legodev_pup_int_platform_data_t
    pbdrv_legodev_pup_int_platform_data[PBDRV_CONFIG_LEGODEV_PUP_NUM_INT_DEV];

extern const pbdrv_legodev_pup_ext_platform_data_t
    pbdrv_legodev_pup_ext_platform_data[PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV];

void pbdrv_legodev_pup_reset_device_detection(pbdrv_legodev_pup_uart_dev_t *dev);

#else

static inline void pbdrv_legodev_pup_reset_device_detection(pbdrv_legodev_pup_uart_dev_t *dev) {
}

#endif // PBDRV_CONFIG_LEGODEV_PUP

#endif // _INTERNAL_PBDRV_LEGODEV_PUP_H_
