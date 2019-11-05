// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_NXTCOLOR_H_
#define _PBIO_NXTCOLOR_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

#include "py/obj.h"

pbio_error_t nxtcolor_get_values_at_mode(pbio_port_t port, uint8_t mode, void *values);

#endif // _PBIO_NXTCOLOR_H_
