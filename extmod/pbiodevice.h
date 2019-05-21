// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <stdint.h>

#include <pbdrv/ioport.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

#include "py/obj.h"

pbio_error_t pb_iodevice_get_type_id(pbio_port_t port, pbio_iodev_type_id_t *id);
pbio_error_t pb_iodevice_get_mode(pbio_port_t port, uint8_t *current_mode);
pbio_error_t pb_iodevice_set_mode(pbio_port_t port, uint8_t new_mode);
mp_obj_t pb_iodevice_get_values(pbio_port_t port);
mp_obj_t pb_iodevice_set_values(pbio_port_t port, mp_obj_t values);
