// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbdrv/config.h>

#include <pbio/port.h>

#include <pybricks/parameters.h>
#include <pybricks/util_mp/pb_type_enum.h>

#if PBDRV_CONFIG_HAS_PORT_A
const pb_obj_enum_member_t pb_Port_A_obj = {
    {&pb_enum_type_Port},
    .name = MP_QSTR_A,
    .value = PBIO_PORT_A
};
#endif

#if PBDRV_CONFIG_HAS_PORT_B
const pb_obj_enum_member_t pb_Port_B_obj = {
    {&pb_enum_type_Port},
    .name = MP_QSTR_B,
    .value = PBIO_PORT_B
};
#endif

#if PBDRV_CONFIG_HAS_PORT_C
const pb_obj_enum_member_t pb_Port_C_obj = {
    {&pb_enum_type_Port},
    .name = MP_QSTR_C,
    .value = PBIO_PORT_C
};
#endif

#if PBDRV_CONFIG_HAS_PORT_D
const pb_obj_enum_member_t pb_Port_D_obj = {
    {&pb_enum_type_Port},
    .name = MP_QSTR_D,
    .value = PBIO_PORT_D
};
#endif

#if PBDRV_CONFIG_HAS_PORT_E
const pb_obj_enum_member_t pb_Port_E_obj = {
    {&pb_enum_type_Port},
    .name = MP_QSTR_E,
    .value = PBIO_PORT_E
};
#endif

#if PBDRV_CONFIG_HAS_PORT_F
const pb_obj_enum_member_t pb_Port_F_obj = {
    {&pb_enum_type_Port},
    .name = MP_QSTR_F,
    .value = PBIO_PORT_F
};
#endif

#if PBDRV_CONFIG_HAS_PORT_1
const pb_obj_enum_member_t pb_Port_1_obj = {
    {&pb_enum_type_Port},
    .name = MP_QSTR_S1,
    .value = PBIO_PORT_1
};
#endif

#if PBDRV_CONFIG_HAS_PORT_2
const pb_obj_enum_member_t pb_Port_2_obj = {
    {&pb_enum_type_Port},
    .name = MP_QSTR_S2,
    .value = PBIO_PORT_2
};
#endif

#if PBDRV_CONFIG_HAS_PORT_3
const pb_obj_enum_member_t pb_Port_3_obj = {
    {&pb_enum_type_Port},
    .name = MP_QSTR_S3,
    .value = PBIO_PORT_3
};
#endif

#if PBDRV_CONFIG_HAS_PORT_4
const pb_obj_enum_member_t pb_Port_4_obj = {
    {&pb_enum_type_Port},
    .name = MP_QSTR_S4,
    .value = PBIO_PORT_4
};
#endif

STATIC const mp_rom_map_elem_t pb_enum_Port_table[] = {
    #if PBDRV_CONFIG_HAS_PORT_A
    { MP_ROM_QSTR(MP_QSTR_A),  MP_ROM_PTR(&pb_Port_A_obj)},
    #endif
    #if PBDRV_CONFIG_HAS_PORT_B
    { MP_ROM_QSTR(MP_QSTR_B),  MP_ROM_PTR(&pb_Port_B_obj)},
    #endif
    #if PBDRV_CONFIG_HAS_PORT_C
    { MP_ROM_QSTR(MP_QSTR_C),  MP_ROM_PTR(&pb_Port_C_obj)},
    #endif
    #if PBDRV_CONFIG_HAS_PORT_D
    { MP_ROM_QSTR(MP_QSTR_D),  MP_ROM_PTR(&pb_Port_D_obj)},
    #endif
    #if PBDRV_CONFIG_HAS_PORT_E
    { MP_ROM_QSTR(MP_QSTR_E),  MP_ROM_PTR(&pb_Port_E_obj)},
    #endif
    #if PBDRV_CONFIG_HAS_PORT_F
    { MP_ROM_QSTR(MP_QSTR_F),  MP_ROM_PTR(&pb_Port_F_obj)},
    #endif
    #if PBDRV_CONFIG_HAS_PORT_1
    { MP_ROM_QSTR(MP_QSTR_S1),  MP_ROM_PTR(&pb_Port_1_obj)},
    #endif
    #if PBDRV_CONFIG_HAS_PORT_2
    { MP_ROM_QSTR(MP_QSTR_S2),  MP_ROM_PTR(&pb_Port_2_obj)},
    #endif
    #if PBDRV_CONFIG_HAS_PORT_3
    { MP_ROM_QSTR(MP_QSTR_S3),  MP_ROM_PTR(&pb_Port_3_obj)},
    #endif
    #if PBDRV_CONFIG_HAS_PORT_4
    { MP_ROM_QSTR(MP_QSTR_S4),  MP_ROM_PTR(&pb_Port_4_obj)},
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_enum_type_Port_locals_dict, pb_enum_Port_table);

const mp_obj_type_t pb_enum_type_Port = {
    { &mp_type_type },
    .name = MP_QSTR_Port,
    .print = pb_type_enum_print,
    .unary_op = mp_generic_unary_op,
    .locals_dict = (mp_obj_dict_t *)&(pb_enum_type_Port_locals_dict),
};

#endif // PYBRICKS_PY_PARAMETERS
