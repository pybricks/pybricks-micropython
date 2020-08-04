// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbdrv/config.h>

#include <pbio/port.h>

#include "parameters/parameters.h"
#include "util_mp/pb_type_enum.h"

STATIC const mp_rom_map_elem_t pb_enum_Port_table[] = {
    #if PBDRV_CONFIG_HAS_PORT_A
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_A, PBIO_PORT_A),
    #endif
    #if PBDRV_CONFIG_HAS_PORT_B
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_B, PBIO_PORT_B),
    #endif
    #if PBDRV_CONFIG_HAS_PORT_C
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_C, PBIO_PORT_C),
    #endif
    #if PBDRV_CONFIG_HAS_PORT_D
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_D, PBIO_PORT_D),
    #endif
    #if PBDRV_CONFIG_HAS_PORT_E
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_E, PBIO_PORT_E),
    #endif
    #if PBDRV_CONFIG_HAS_PORT_F
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_F, PBIO_PORT_F),
    #endif
    #if PBDRV_CONFIG_HAS_PORT_1
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_S1, PBIO_PORT_1),
    #endif
    #if PBDRV_CONFIG_HAS_PORT_2
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_S2, PBIO_PORT_2),
    #endif
    #if PBDRV_CONFIG_HAS_PORT_3
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_S3, PBIO_PORT_3),
    #endif
    #if PBDRV_CONFIG_HAS_PORT_4
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_S4, PBIO_PORT_4),
    #endif
};
PB_DEFINE_ENUM(pb_enum_type_Port, MP_QSTR_Port, pb_enum_Port_table);

#endif // PYBRICKS_PY_PARAMETERS
