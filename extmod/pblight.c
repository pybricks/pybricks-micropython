// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/light.h>
#include "pberror.h"

#include "py/mpconfig.h"

#include <stdio.h>

#include "pblight.h"

void pb_color_light_on(pbdevice_t *pbdev, pbio_light_color_t color) {
    if (!pbdev) {
        // No external device, so assume command is for the internal light
        pb_assert(pbio_light_on(PBIO_PORT_SELF, color));
        return;
    }

    pbdevice_color_light_on(pbdev, color);
}
