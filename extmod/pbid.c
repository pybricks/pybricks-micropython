// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "pbid.h"
#include "mpconfigbrick.h"

pbio_iodev_type_id_t get_id_from_qstr(qstr q){
    switch(q) {
        #if defined(PYBRICKS_BRICK_MOVEHUB)
        case MP_QSTR_ColorDistanceSensor:
            return PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR;
        #endif //PYBRICKS_BRICK_MOVEHUB
        #if defined(PYBRICKS_BRICK_EV3)
        #endif //PYBRICKS_BRICK_EV3
        default:
            return PBIO_IODEV_TYPE_ID_NONE;
    }
}
