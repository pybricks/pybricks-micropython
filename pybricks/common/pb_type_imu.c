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
    float q[4];
    pbdrv_imu_quaternion_read(self->imu_dev,q);
    
    mp_obj_t tilt[2];

    // Pitch
    float pitch = ((float) -asinf(2.0 * (q[1] * q[3] - q[0] * q[2]))) * 57.296f;
    tilt[0] = mp_obj_new_float_from_f(pitch);

    // Roll
    float roll =(float) atan2f( 2.0f * (float) (q[0] * q[1] + q[2] * q[3]),q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3]) * 57.296f;
    tilt[1] = mp_obj_new_float_from_f(roll);
    return mp_obj_new_tuple(2, tilt);
}
MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_tilt_obj, common_IMU_tilt);

// pybricks._common.IMU.heading
STATIC mp_obj_t common_IMU_heading(mp_obj_t self_in) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);
    float q[4];
    pbdrv_imu_quaternion_read(self->imu_dev,q);


    float heading = ((float) atan2f(2.0 * (q[1] * q[2] + q[0] * q[3]),q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3])) * 57.296f;
    return mp_obj_new_float_from_f(heading);
}
MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_heading_obj, common_IMU_heading);

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

// pybricks._common.IMU.quaternion
STATIC mp_obj_t common_IMU_quaternion(mp_obj_t self_in) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);

    float values[4];
    pbdrv_imu_quaternion_read(self->imu_dev, values);

    return pb_type_Matrix_make_vector(4, values, false);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_quaternion_obj, common_IMU_quaternion);

// pybricks._common.IMU.set_mahony_gains
STATIC mp_obj_t common_IMU_start_gyro_calibration(mp_obj_t self_in) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbdrv_imu_start_gyro_calibration(self->imu_dev);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_start_gyro_calibration_obj, common_IMU_start_gyro_calibration);


// pybricks._common.IMU.quaternion
STATIC mp_obj_t common_IMU_stop_gyro_calibration(mp_obj_t self_in) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);

    float values[3];
    pbdrv_imu_stop_gyro_calibration(self->imu_dev, values);

    return pb_type_Matrix_make_vector(3, values, false);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_stop_gyro_calibration_obj, common_IMU_stop_gyro_calibration);

// pybricks._common.IMU.set_mahony_gains
STATIC mp_obj_t common_IMU_set_gyro_bias(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_IMU_obj_t, self,
        PB_ARG_REQUIRED(X),PB_ARG_REQUIRED(Y),PB_ARG_REQUIRED(Z));
    mp_float_t X = mp_obj_get_float(X_in);
    mp_float_t Y = mp_obj_get_float(Y_in);
    mp_float_t Z = mp_obj_get_float(Z_in);
    
    pbdrv_imu_set_gyro_bias(self->imu_dev,X,Y,Z);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_IMU_set_gyro_bias_obj,4, common_IMU_set_gyro_bias);

// pybricks._common.IMU.reset_heading
STATIC mp_obj_t common_IMU_reset_heading(mp_obj_t self_in) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbdrv_imu_reset_heading(self->imu_dev);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_reset_heading_obj, common_IMU_reset_heading);

// pybricks._common.IMU.set_mahony_gains
STATIC mp_obj_t common_IMU_set_mahony_gains(mp_obj_t self_in,mp_obj_t MP_Kp,mp_obj_t MP_Ki) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_float_t Kp = mp_obj_get_float(MP_Kp);
    mp_float_t Ki = mp_obj_get_float(MP_Ki);

    pbdrv_imu_set_mahony_gains(self->imu_dev,Kp,Ki);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(common_IMU_set_mahony_gains_obj, common_IMU_set_mahony_gains);




// HACK: this is for testing and will be removed
STATIC mp_obj_t common_IMU_temp(mp_obj_t self_in) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float_from_f(pbdrv_imu_temperature_read(self->imu_dev));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_temp_obj, common_IMU_temp);

// dir(pybricks.common.IMU)
STATIC const mp_rom_map_elem_t common_IMU_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_up),                      MP_ROM_PTR(&common_IMU_up_obj)                      },
    { MP_ROM_QSTR(MP_QSTR_tilt),                    MP_ROM_PTR(&common_IMU_tilt_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_heading),                 MP_ROM_PTR(&common_IMU_heading_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_acceleration),            MP_ROM_PTR(&common_IMU_acceleration_obj)            },
    { MP_ROM_QSTR(MP_QSTR_angular_velocity),        MP_ROM_PTR(&common_IMU_angular_velocity_obj)        },
    { MP_ROM_QSTR(MP_QSTR_quaternion),              MP_ROM_PTR(&common_IMU_quaternion_obj)              },
    { MP_ROM_QSTR(MP_QSTR_reset_heading),           MP_ROM_PTR(&common_IMU_reset_heading_obj)           },
    { MP_ROM_QSTR(MP_QSTR_set_gyro_bias),           MP_ROM_PTR(&common_IMU_set_gyro_bias_obj)           },
    { MP_ROM_QSTR(MP_QSTR_stop_gyro_calibration),   MP_ROM_PTR(&common_IMU_stop_gyro_calibration_obj)   },
    { MP_ROM_QSTR(MP_QSTR_set_mahony_gains),          MP_ROM_PTR(&common_IMU_set_mahony_gains_obj)          },
    { MP_ROM_QSTR(MP_QSTR_start_gyro_calibration),  MP_ROM_PTR(&common_IMU_start_gyro_calibration_obj)  },
    // HACK: this is for testing and will be removed
    { MP_ROM_QSTR(MP_QSTR_temp), MP_ROM_PTR(&common_IMU_temp_obj)},
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
        pbdrv_imu_dev_t *imu_dev;
        pb_assert(pbdrv_imu_get_imu(&imu_dev));
        self->base.type = &pb_type_IMU;
        self->imu_dev = imu_dev;
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
