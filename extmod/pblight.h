// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PYBRICKS_LIGHT_H_
#define _PYBRICKS_LIGHT_H_

#include "pbdevice.h"

#include <pbio/light.h>

// TODO: Ultimately, lights should be redesigned and moved to pbio
struct _pbio_lightdev_t {
    pbio_iodev_type_id_t id;
    pbio_iodev_t *pupiodev;
    pbdevice_t *ev3iodev;
};

typedef struct _pbio_lightdev_t pbio_lightdev_t;

pbio_error_t pb_light_on(pbio_lightdev_t *dev);

pbio_error_t pb_color_light_on(pbio_lightdev_t *dev, pbio_light_color_t color);

#endif // _PYBRICKS_LIGHT_H_
