// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#include "py/obj.h"

#include <lsm6ds3tr_c_reg.h>

#include <pybricks/common.h>
#include <pybricks/geometry.h>

#include <pybricks/util_pb/pb_imu.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

typedef struct _common_IMU_obj_t {
    mp_obj_base_t base;
    pb_imu_dev_t *imu_dev;
} common_IMU_obj_t;

// pybricks._common.IMU.acceleration
STATIC mp_obj_t common_IMU_acceleration(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_IMU_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    float_t values[3];
    pb_imu_accel_read(self->imu_dev, values);

    if (axis_in == &pb_Axis_X_obj) {
        return mp_obj_new_float_from_f(values[0]);
    }
    if (axis_in == &pb_Axis_Y_obj) {
        return mp_obj_new_float_from_f(values[1]);
    }
    if (axis_in == &pb_Axis_Z_obj) {
        return mp_obj_new_float_from_f(values[2]);
    }

    // TODO: Handle other axes via projection and scale accordingly

    return pb_type_Matrix_make_vector(3, values, false);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_IMU_acceleration_obj, 1, common_IMU_acceleration);

// pybricks._common.IMU.gyro
STATIC mp_obj_t common_IMU_gyro(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_IMU_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    float_t values[3];
    pb_imu_gyro_read(self->imu_dev, values);

    if (axis_in == &pb_Axis_X_obj) {
        return mp_obj_new_float_from_f(values[0]);
    }
    if (axis_in == &pb_Axis_Y_obj) {
        return mp_obj_new_float_from_f(values[1]);
    }
    if (axis_in == &pb_Axis_Z_obj) {
        return mp_obj_new_float_from_f(values[2]);
    }

    // TODO: Handle other axes via projection and scale accordingly

    return pb_type_Matrix_make_vector(3, values, false);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_IMU_gyro_obj, 1, common_IMU_gyro);

// dir(pybricks.common.IMU)
STATIC const mp_rom_map_elem_t common_IMU_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acceleration), MP_ROM_PTR(&common_IMU_acceleration_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyro),         MP_ROM_PTR(&common_IMU_gyro_obj)         },
};
STATIC MP_DEFINE_CONST_DICT(common_IMU_locals_dict, common_IMU_locals_dict_table);

// type(pybricks.common.IMU)
STATIC const mp_obj_type_t pb_type_IMU = {
    { &mp_type_type },
    .name = MP_QSTR_IMU,
    .locals_dict = (mp_obj_dict_t *)&common_IMU_locals_dict,
};

STATIC common_IMU_obj_t singleton_obj;

// pybricks._common.IMU.__init__
mp_obj_t pb_type_IMU_obj_new(void) {

    // Get singleton instance
    common_IMU_obj_t *self = &singleton_obj;

    // Return if already initialized
    if (self->imu_dev) {
        return self;
    }

    self->base.type = &pb_type_IMU;

    pb_imu_get_imu(&self->imu_dev);

    // Initialize IMU
    pb_imu_init(self->imu_dev);

    return self;
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU
