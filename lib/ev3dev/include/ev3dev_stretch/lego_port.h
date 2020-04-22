// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_LEGO_PORT_H_
#define _PBIO_LEGO_PORT_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

pbio_error_t ev3dev_lego_port_configure(pbio_port_t port, pbio_iodev_type_id_t id);

#endif // _PBIO_LEGO_PORT_H_
