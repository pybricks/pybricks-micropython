// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/iodev.h>

#include "py/obj.h"

void pb_iodevice_assert_type_id(pbio_iodev_t *iodev, pbio_iodev_type_id_t type_id);
pbio_error_t pb_iodevice_get_type_id(pbio_iodev_t *iodev, pbio_iodev_type_id_t *id);
pbio_error_t pb_iodevice_get_mode(pbio_iodev_t *iodev, uint8_t *current_mode);
pbio_error_t pb_iodevice_set_mode(pbio_iodev_t *iodev, uint8_t new_mode);
mp_obj_t pb_iodevice_get_values(pbio_iodev_t *iodev);
mp_obj_t pb_iodevice_set_values(pbio_iodev_t *iodev, mp_obj_t values);
