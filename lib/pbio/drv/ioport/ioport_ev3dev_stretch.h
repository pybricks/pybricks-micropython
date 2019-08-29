// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

// ev3dev-stretch I/O port

#ifndef _PBDRV_IOPORT_EV3DEV_STRETCH_H_
#define _PBDRV_IOPORT_EV3DEV_STRETCH_H_

#include <pbio/error.h>
#include <pbio/port.h>

pbio_error_t pbdrv_ioport_ev3dev_get_syspath(pbio_port_t port, const char **syspath);

#endif // _PBDRV_IOPORT_EV3DEV_STRETCH_H_
