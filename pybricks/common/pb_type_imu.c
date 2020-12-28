// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#include "py/obj.h"

#include <pybricks/common.h>

// pybricks._common.IMU class object
typedef struct _common_IMU_obj_t {
    mp_obj_base_t base;
} common_IMU_obj_t;

// dir(pybricks.common.IMU)
STATIC const mp_rom_map_elem_t common_IMU_locals_dict_table[] = {
};
STATIC MP_DEFINE_CONST_DICT(common_IMU_locals_dict, common_IMU_locals_dict_table);

// type(pybricks.common.IMU)
STATIC const mp_obj_type_t pb_type_IMU = {
    { &mp_type_type },
    .name = MP_QSTR_IMU,
    .locals_dict = (mp_obj_dict_t *)&common_IMU_locals_dict,
};

// pybricks._common.IMU.__init__
mp_obj_t pb_type_IMU_obj_new(void) {
    common_IMU_obj_t *self = m_new_obj(common_IMU_obj_t);
    self->base.type = &pb_type_IMU;
    return self;
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU
