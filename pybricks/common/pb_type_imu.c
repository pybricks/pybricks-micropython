// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#include "py/obj.h"

#include <lsm6ds3tr_c_reg.h>

#include <pybricks/common.h>
#include <pybricks/geometry.h>

#include <pybricks/util_pb/pb_imu.h>
#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

typedef struct _common_IMU_obj_t {
    mp_obj_base_t base;
    pb_imu_dev_t *imu_dev;
} common_IMU_obj_t;


STATIC mp_obj_t common_IMU_project_3d_axis(mp_obj_t axis_in, float *values) {

    // If no axis is specified, return a vector of values
    if (axis_in == mp_const_none) {
        return pb_type_Matrix_make_vector(3, values, false);
    }

    // If X, Y, or Z is specified, return value directly for efficiency in most cases
    if (axis_in == &pb_Axis_X_obj) {
        return mp_obj_new_float_from_f(values[0]);
    }
    if (axis_in == &pb_Axis_Y_obj) {
        return mp_obj_new_float_from_f(values[1]);
    }
    if (axis_in == &pb_Axis_Z_obj) {
        return mp_obj_new_float_from_f(values[2]);
    }

    // Argument must be a matrix
    if (!mp_obj_is_type(axis_in, &pb_type_Matrix)) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    pb_type_Matrix_obj_t *axis = MP_OBJ_TO_PTR(axis_in);

    // Axis must be 1x3 or 3x1
    if (axis->m * axis->n != 3) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Project data onto user specified axis and scale user axis to unit length
    float scalar = (axis->data[0] * values[0] + axis->data[1] * values[1] + axis->data[2] * values[2]) * axis->scale;
    return mp_obj_new_float_from_f(scalar / sqrtf(axis->data[0] * axis->data[0] + axis->data[1] * axis->data[1] + axis->data[2] * axis->data[2]));
}

// pybricks._common.IMU.acceleration
STATIC mp_obj_t common_IMU_acceleration(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_IMU_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    float_t values[3];
    pb_imu_accel_read(self->imu_dev, values);

    return common_IMU_project_3d_axis(axis_in, values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_IMU_acceleration_obj, 1, common_IMU_acceleration);

// pybricks._common.IMU.gyro
STATIC mp_obj_t common_IMU_gyro(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_IMU_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    float_t values[3];
    pb_imu_gyro_read(self->imu_dev, values);

    return common_IMU_project_3d_axis(axis_in, values);
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
mp_obj_t pb_type_IMU_obj_new(mp_obj_t top_side_axis, mp_obj_t front_side_axis) {

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
