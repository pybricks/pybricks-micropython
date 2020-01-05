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

    // FIXME: move light ops to pbdevice. For now do it by ID here.
    pbio_port_t port;
    pbio_iodev_type_id_t id;
    uint8_t mode;
    pbio_iodev_data_type_t data_type;
    uint8_t num_values;
    
    pbdevice_get_info(pbdev, &port, &id, &mode, &data_type, &num_values);

#if PYBRICKS_PY_EV3DEVICES
    if (id == PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR) {
        pbdevice_get_values(pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP, &color);
    }
#endif
#if PYBRICKS_PY_PUPDEVICES
    if (id == PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR) {
        uint8_t mode;
        switch (color) {
            case PBIO_LIGHT_COLOR_GREEN:
                mode = PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX;
                break;
            case PBIO_LIGHT_COLOR_RED:
                mode = PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__REFLT;
                break;
            case PBIO_LIGHT_COLOR_BLUE:
                mode = PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI;
                break;
            default:
                mode = PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__SPEC1;
                break;
        }
        uint32_t *data;
        pbdevice_get_values(pbdev, mode, &data);
    }
#endif
}
