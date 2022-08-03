// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2022 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#include <math.h>
#include <stdbool.h>

#include <pbdrv/imu.h>
#include <pbio/error.h>

#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/geometry.h>
#include <pybricks/parameters.h>
#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

typedef struct _common_IMU_obj_t {
    mp_obj_base_t base;
    pbdrv_imu_dev_t *imu_dev;
    float hub_x[3];
    float hub_y[3];
    float hub_z[3];
    bool use_default_placement;
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

    if (!mp_obj_is_type(axis_in, &pb_type_Matrix)) {
        mp_raise_TypeError(MP_ERROR_TEXT("axis must be Matrix or None"));
    }
    pb_type_Matrix_obj_t *axis = MP_OBJ_TO_PTR(axis_in);

    if (axis->m * axis->n != 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("axis must be 1x3 or 3x1 matrix"));
    }

    // Project data onto user specified axis and scale user axis to unit length
    float scalar = (axis->data[0] * values[0] + axis->data[1] * values[1] + axis->data[2] * values[2]) * axis->scale;
    return mp_obj_new_float_from_f(scalar / sqrtf(axis->data[0] * axis->data[0] + axis->data[1] * axis->data[1] + axis->data[2] * axis->data[2]));
}

STATIC void common_IMU_rotate_3d_axis(common_IMU_obj_t *self, float *values) {

    // If we use default placement, don't do anything.
    if (self->use_default_placement) {
        return;
    }

    // Make a copy of the input before we override it.
    float v[] = {values[0], values[1], values[2]};

    // Evaluate the rotation.
    values[0] = self->hub_x[0] * v[0] + self->hub_y[0] * v[1] + self->hub_z[0] * v[2];
    values[1] = self->hub_x[1] * v[0] + self->hub_y[1] * v[1] + self->hub_z[1] * v[2];
    values[2] = self->hub_x[2] * v[0] + self->hub_y[2] * v[1] + self->hub_z[2] * v[2];
}

// pybricks._common.IMU.up
STATIC mp_obj_t common_IMU_up(mp_obj_t self_in) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Up is which side of a unit box intersects the +Z vector first.
    // So read +Z vector of the inertial frame, in the body frame.
    // For now, this is the gravity vector. In the future, we can make this
    // slightly more accurate by using the full IMU orientation.
    float values[3];
    pbdrv_imu_accel_read(self->imu_dev, values);

    // Find index and sign of maximum component
    float abs_max = 0;
    uint8_t axis = 0;
    bool positive = false;
    for (uint8_t i = 0; i < 3; i++) {
        if (values[i] > abs_max) {
            abs_max = values[i];
            positive = true;
            axis = i;
        } else if (-values[i] > abs_max) {
            abs_max = -values[i];
            positive = false;
            axis = i;
        }
    }

    // The maximum component dictates which side of a unit box gets intersected
    // first. So, simply look at axis and sign to give the side.
    if (axis == 0 && positive) {
        return MP_OBJ_FROM_PTR(&pb_Side_FRONT_obj);
    }
    if (axis == 0 && !positive) {
        return MP_OBJ_FROM_PTR(&pb_Side_BACK_obj);
    }
    if (axis == 1 && positive) {
        return MP_OBJ_FROM_PTR(&pb_Side_LEFT_obj);
    }
    if (axis == 1 && !positive) {
        return MP_OBJ_FROM_PTR(&pb_Side_RIGHT_obj);
    }
    if (axis == 2 && positive) {
        return MP_OBJ_FROM_PTR(&pb_Side_TOP_obj);
    } else {
        return MP_OBJ_FROM_PTR(&pb_Side_BOTTOM_obj);
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_up_obj, common_IMU_up);

// pybricks._common.IMU.tilt
STATIC mp_obj_t common_IMU_tilt(mp_obj_t self_in) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Read acceleration in the user frame. In the future, we can make this
    // more accurate by using the full IMU orientation.
    float accl[3];
    pbdrv_imu_accel_read(self->imu_dev, accl);
    common_IMU_rotate_3d_axis(self, accl);

    mp_obj_t tilt[2];
    // Pitch
    float pitch = atan2f(-accl[0], sqrtf(accl[2] * accl[2] + accl[1] * accl[1]));
    tilt[0] = mp_obj_new_int_from_float(pitch * 57.296f);

    // Roll
    float roll = atan2f(accl[1], accl[2]);
    tilt[1] = mp_obj_new_int_from_float(roll * 57.296f);
    return mp_obj_new_tuple(2, tilt);
}
MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_tilt_obj, common_IMU_tilt);

// pybricks._common.IMU.acceleration
STATIC mp_obj_t common_IMU_acceleration(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_IMU_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    float values[3];
    pbdrv_imu_accel_read(self->imu_dev, values);
    common_IMU_rotate_3d_axis(self, values);

    return common_IMU_project_3d_axis(axis_in, values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_IMU_acceleration_obj, 1, common_IMU_acceleration);

// pybricks._common.IMU.gyro
STATIC mp_obj_t common_IMU_angular_velocity(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_IMU_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    float values[3];
    pbdrv_imu_gyro_read(self->imu_dev, values);
    common_IMU_rotate_3d_axis(self, values);

    return common_IMU_project_3d_axis(axis_in, values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_IMU_angular_velocity_obj, 1, common_IMU_angular_velocity);

// dir(pybricks.common.IMU)
STATIC const mp_rom_map_elem_t common_IMU_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_up),               MP_ROM_PTR(&common_IMU_up_obj)              },
    { MP_ROM_QSTR(MP_QSTR_tilt),             MP_ROM_PTR(&common_IMU_tilt_obj)            },
    { MP_ROM_QSTR(MP_QSTR_acceleration),     MP_ROM_PTR(&common_IMU_acceleration_obj)    },
    { MP_ROM_QSTR(MP_QSTR_angular_velocity), MP_ROM_PTR(&common_IMU_angular_velocity_obj)},
};
STATIC MP_DEFINE_CONST_DICT(common_IMU_locals_dict, common_IMU_locals_dict_table);

// type(pybricks.common.IMU)
STATIC const mp_obj_type_t pb_type_IMU = {
    { &mp_type_type },
    .name = MP_QSTR_IMU,
    .locals_dict = (mp_obj_dict_t *)&common_IMU_locals_dict,
};

// Extracts a (scaled) 3D vector object to a plain, normalized float array
static void get_normal_axis(pb_type_Matrix_obj_t *vector, float *dest) {

    // Assert we have a vector with 3 entries
    if (vector->m * vector->n != 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("axis must be 1x3 or 3x1 matrix"));
    }

    // compute norm
    float magnitude = sqrtf(
        vector->data[0] * vector->data[0] +
        vector->data[1] * vector->data[1] +
        vector->data[2] * vector->data[2]);

    // Assert we have a vector with nonzero length
    if (magnitude < 0.001f) {
        mp_raise_ValueError(MP_ERROR_TEXT("axis must have nonzero length"));
    }

    // Scale and sign magnitude by matrix scale
    magnitude *= vector->scale;

    // Set destination values
    dest[0] = vector->data[0] / magnitude;
    dest[1] = vector->data[1] / magnitude;
    dest[2] = vector->data[2] / magnitude;
}

STATIC common_IMU_obj_t singleton_imu_obj;

// pybricks._common.IMU.__init__
mp_obj_t pb_type_IMU_obj_new(mp_obj_t top_side_axis, mp_obj_t front_side_axis) {

    // Get singleton instance
    common_IMU_obj_t *self = &singleton_imu_obj;

    // Initialized if not done so already
    if (!self->imu_dev) {
        pb_assert(pbdrv_imu_get_imu(&self->imu_dev));
        self->base.type = &pb_type_IMU;
    }

    // Check if we use the default orientation.
    if (MP_OBJ_TO_PTR(top_side_axis) == &pb_Axis_Z_obj && MP_OBJ_TO_PTR(front_side_axis) == &pb_Axis_X_obj) {
        // If so, we can avoid math on every read.
        self->use_default_placement = true;
    } else {
        // If not, compute the orientation and use it for reading data from now on.
        self->use_default_placement = false;

        // Extract the body X axis
        get_normal_axis(front_side_axis, self->hub_x);

        // Extract the body Z axis
        get_normal_axis(top_side_axis, self->hub_z);

        // Assert that X and Z are orthogonal
        float inner = self->hub_x[0] * self->hub_z[0] + self->hub_x[1] * self->hub_z[1] + self->hub_x[2] * self->hub_z[2];
        if (inner > 0.001f || inner < -0.001f) {
            mp_raise_ValueError(MP_ERROR_TEXT("X axis must be orthogonal to Z axis"));
        }

        // Make the body Y axis as Y = cross(Z, X)
        self->hub_y[0] = self->hub_z[1] * self->hub_x[2] - self->hub_z[2] * self->hub_x[1];
        self->hub_y[1] = self->hub_z[2] * self->hub_x[0] - self->hub_z[0] * self->hub_x[2];
        self->hub_y[2] = self->hub_z[0] * self->hub_x[1] - self->hub_z[1] * self->hub_x[0];
    }

    return self;
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU
