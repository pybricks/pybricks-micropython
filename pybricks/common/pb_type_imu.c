// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2022 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include <pbdrv/imu.h>
#include <pbio/error.h>
#include <pbio/geometry.h>
#include <pbio/orientation.h>

#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/geometry.h>
#include <pybricks/parameters.h>
#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

typedef struct _common_IMU_obj_t {
    mp_obj_base_t base;
    pbdrv_imu_dev_t *imu_dev;
} common_IMU_obj_t;

// pybricks._common.IMU.up
STATIC mp_obj_t common_IMU_up(mp_obj_t self_in) {
    switch (pbio_orientation_imu_get_up_side()) {
        case PBIO_ORIENTATION_SIDE_FRONT:
            return MP_OBJ_FROM_PTR(&pb_Side_FRONT_obj);
        case PBIO_ORIENTATION_SIDE_LEFT:
            return MP_OBJ_FROM_PTR(&pb_Side_LEFT_obj);
        case PBIO_ORIENTATION_SIDE_TOP:
            return MP_OBJ_FROM_PTR(&pb_Side_TOP_obj);
        case PBIO_ORIENTATION_SIDE_BACK:
            return MP_OBJ_FROM_PTR(&pb_Side_BACK_obj);
        case PBIO_ORIENTATION_SIDE_RIGHT:
            return MP_OBJ_FROM_PTR(&pb_Side_RIGHT_obj);
        case PBIO_ORIENTATION_SIDE_BOTTOM:
        // fallthrough
        default:
            return MP_OBJ_FROM_PTR(&pb_Side_BOTTOM_obj);
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_up_obj, common_IMU_up);

// pybricks._common.IMU.tilt
STATIC mp_obj_t common_IMU_tilt(mp_obj_t self_in) {

    // Read acceleration in the user frame.
    pbio_geometry_xyz_t accl;
    pbio_orientation_imu_get_acceleration(&accl);

    mp_obj_t tilt[2];
    // Pitch
    float pitch = atan2f(-accl.x, sqrtf(accl.z * accl.z + accl.y * accl.y));
    tilt[0] = mp_obj_new_int_from_float(pitch * 57.296f);

    // Roll
    float roll = atan2f(accl.y, accl.z);
    tilt[1] = mp_obj_new_int_from_float(roll * 57.296f);
    return mp_obj_new_tuple(2, tilt);
}
MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_tilt_obj, common_IMU_tilt);

STATIC void pb_type_imu_extract_axis(mp_obj_t obj_in, pbio_geometry_xyz_t *vector) {
    if (!mp_obj_is_type(obj_in, &pb_type_Matrix)) {
        mp_raise_TypeError(MP_ERROR_TEXT("Axis must be Matrix or None."));
    }
    pb_type_Matrix_obj_t *vector_obj = MP_OBJ_TO_PTR(obj_in);
    if (vector_obj->m * vector_obj->n != 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("Axis must be 1x3 or 3x1 matrix."));
    }
    for (uint8_t i = 0; i < MP_ARRAY_SIZE(vector->values); i++) {
        vector->values[i] = vector_obj->data[i] * vector_obj->scale;
    }
}

// pybricks._common.IMU.acceleration
STATIC mp_obj_t common_IMU_acceleration(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_IMU_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    (void)self;
    pbio_geometry_xyz_t acceleration;
    pbio_orientation_imu_get_acceleration(&acceleration);

    // If no axis is specified, return a vector of values.
    if (axis_in == mp_const_none) {
        return pb_type_Matrix_make_vector(3, acceleration.values, false);
    }

    // Otherwise convert user axis to pbio object and project vector onto it.
    pbio_geometry_xyz_t axis;
    pb_type_imu_extract_axis(axis_in, &axis);
    return mp_obj_new_float_from_f(pbio_geometry_vector_project(&axis, &acceleration));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_IMU_acceleration_obj, 1, common_IMU_acceleration);

// pybricks._common.IMU.angular_velocity
STATIC mp_obj_t common_IMU_angular_velocity(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_IMU_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    (void)self;
    pbio_geometry_xyz_t angular_velocity;
    pbio_orientation_imu_get_angular_velocity(&angular_velocity);

    // If no axis is specified, return a vector of values.
    if (axis_in == mp_const_none) {
        return pb_type_Matrix_make_vector(3, angular_velocity.values, false);
    }

    // Otherwise convert user axis to pbio object and project vector onto it.
    pbio_geometry_xyz_t axis;
    pb_type_imu_extract_axis(axis_in, &axis);
    return mp_obj_new_float_from_f(pbio_geometry_vector_project(&axis, &angular_velocity));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_IMU_angular_velocity_obj, 1, common_IMU_angular_velocity);

// pybricks._common.IMU.stationary
STATIC mp_obj_t common_IMU_stationary(mp_obj_t self_in) {
    return mp_obj_new_bool(pbio_orientation_imu_is_stationary());
}
MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_stationary_obj, common_IMU_stationary);

// pybricks._common.IMU.heading
STATIC mp_obj_t common_IMU_heading(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_float(pbio_orientation_imu_get_heading());
}
MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_heading_obj, common_IMU_heading);

// pybricks._common.IMU.reset_heading
STATIC mp_obj_t common_IMU_reset_heading(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_IMU_obj_t, self,
        PB_ARG_REQUIRED(angle));

    // Set the new angle
    (void)self;
    pbio_orientation_imu_set_heading(mp_obj_get_float(angle_in));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_IMU_reset_heading_obj, 1, common_IMU_reset_heading);

// dir(pybricks.common.IMU)
STATIC const mp_rom_map_elem_t common_IMU_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_up),               MP_ROM_PTR(&common_IMU_up_obj)              },
    { MP_ROM_QSTR(MP_QSTR_tilt),             MP_ROM_PTR(&common_IMU_tilt_obj)            },
    { MP_ROM_QSTR(MP_QSTR_acceleration),     MP_ROM_PTR(&common_IMU_acceleration_obj)    },
    { MP_ROM_QSTR(MP_QSTR_angular_velocity), MP_ROM_PTR(&common_IMU_angular_velocity_obj)},
    { MP_ROM_QSTR(MP_QSTR_stationary),       MP_ROM_PTR(&common_IMU_stationary_obj)      },
    { MP_ROM_QSTR(MP_QSTR_heading),          MP_ROM_PTR(&common_IMU_heading_obj)         },
    { MP_ROM_QSTR(MP_QSTR_reset_heading),    MP_ROM_PTR(&common_IMU_reset_heading_obj)   },
};
STATIC MP_DEFINE_CONST_DICT(common_IMU_locals_dict, common_IMU_locals_dict_table);

// type(pybricks.common.IMU)
STATIC const mp_obj_type_t pb_type_IMU = {
    { &mp_type_type },
    .name = MP_QSTR_IMU,
    .locals_dict = (mp_obj_dict_t *)&common_IMU_locals_dict,
};

STATIC common_IMU_obj_t singleton_imu_obj;

// pybricks._common.IMU.__init__
mp_obj_t pb_type_IMU_obj_new(mp_obj_t top_side_axis, mp_obj_t front_side_axis) {

    // Get singleton instance
    common_IMU_obj_t *self = &singleton_imu_obj;

    // Initialized if not done so already
    if (!self->imu_dev) {
        pbdrv_imu_dev_t *imu_dev;
        pbdrv_imu_config_t *imu_config;
        pb_assert(pbdrv_imu_get_imu(&imu_dev, &imu_config));
        self->base.type = &pb_type_IMU;
        self->imu_dev = imu_dev;
    }
    pbio_orientation_imu_set_heading(0.0f);

    pb_assert_type(top_side_axis, &pb_type_Matrix);
    pb_assert_type(front_side_axis, &pb_type_Matrix);

    // REVISIT: Extract/align pbio and pybricks matrix type
    pbio_geometry_xyz_t hub_x;
    memcpy(hub_x.values, ((pb_type_Matrix_obj_t *)front_side_axis)->data, sizeof(hub_x.values));

    pbio_geometry_xyz_t hub_z;
    memcpy(hub_z.values, ((pb_type_Matrix_obj_t *)top_side_axis)->data, sizeof(hub_z.values));

    pbio_orientation_set_base_orientation(&hub_x, &hub_z);

    return MP_OBJ_FROM_PTR(self);
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU
