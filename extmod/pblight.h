// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PYBRICKS_LIGHT_H_
#define _PYBRICKS_LIGHT_H_

#include "pbdevice.h"

#include <pbio/light.h>

pbio_error_t pb_light_on(pbdevice_t *pbdev);

pbio_error_t pb_color_light_on(pbdevice_t *pbdev, pbio_light_color_t color);

#endif // _PYBRICKS_LIGHT_H_
