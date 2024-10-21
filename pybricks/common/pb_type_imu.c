// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include <pbio/drivebase.h>
#include <pbio/error.h>
#include <pbio/geometry.h>
#include <pbio/imu.h>

#include <pbsys/storage_settings.h>
#include <pbsys/program_stop.h>

#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/tools/pb_type_matrix.h>
#include <pybricks/parameters.h>
#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

typedef struct _pb_type_imu_obj_t {
    mp_obj_base_t base;
    mp_obj_t hub;
} pb_type_imu_obj_t;

// pybricks._common.IMU.up
static mp_obj_t pb_type_imu_up(mp_obj_t self_in) {
    switch (pbio_imu_get_up_side()) {
        default:
        case PBIO_GEOMETRY_SIDE_FRONT:
            return MP_OBJ_FROM_PTR(&pb_Side_FRONT_obj);
        case PBIO_GEOMETRY_SIDE_LEFT:
            return MP_OBJ_FROM_PTR(&pb_Side_LEFT_obj);
        case PBIO_GEOMETRY_SIDE_TOP:
            return MP_OBJ_FROM_PTR(&pb_Side_TOP_obj);
        case PBIO_GEOMETRY_SIDE_BACK:
            return MP_OBJ_FROM_PTR(&pb_Side_BACK_obj);
        case PBIO_GEOMETRY_SIDE_RIGHT:
            return MP_OBJ_FROM_PTR(&pb_Side_RIGHT_obj);
        case PBIO_GEOMETRY_SIDE_BOTTOM:
            return MP_OBJ_FROM_PTR(&pb_Side_BOTTOM_obj);
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_imu_up_obj, pb_type_imu_up);

// pybricks._common.IMU.tilt
static mp_obj_t pb_type_imu_tilt(mp_obj_t self_in) {

    // Read acceleration in the user frame.
    pbio_geometry_xyz_t accl;
    pbio_imu_get_acceleration(&accl);

    mp_obj_t tilt[2];
    // Pitch
    float pitch = atan2f(-accl.x, sqrtf(accl.z * accl.z + accl.y * accl.y));
    tilt[0] = mp_obj_new_int_from_float(pitch * 57.296f);

    // Roll
    float roll = atan2f(accl.y, accl.z);
    tilt[1] = mp_obj_new_int_from_float(roll * 57.296f);
    return mp_obj_new_tuple(2, tilt);
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_imu_tilt_obj, pb_type_imu_tilt);

static void pb_type_imu_extract_axis(mp_obj_t obj_in, pbio_geometry_xyz_t *vector) {
    if (!mp_obj_is_type(obj_in, &pb_type_Matrix)) {
        mp_raise_TypeError(MP_ERROR_TEXT("Axis must be Matrix."));
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
static mp_obj_t pb_type_imu_acceleration(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_imu_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    (void)self;
    pbio_geometry_xyz_t acceleration;
    pbio_imu_get_acceleration(&acceleration);

    // If no axis is specified, return a vector of values.
    if (axis_in == mp_const_none) {
        return pb_type_Matrix_make_vector(3, acceleration.values, false);
    }

    // Otherwise convert user axis to pbio object and project vector onto it.
    pbio_geometry_xyz_t axis;
    pb_type_imu_extract_axis(axis_in, &axis);

    float projection;
    pb_assert(pbio_geometry_vector_project(&axis, &acceleration, &projection));
    return mp_obj_new_float_from_f(projection);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_imu_acceleration_obj, 1, pb_type_imu_acceleration);

// pybricks._common.IMU.angular_velocity
static mp_obj_t pb_type_imu_angular_velocity(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_imu_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    (void)self;
    pbio_geometry_xyz_t angular_velocity;
    pbio_imu_get_angular_velocity(&angular_velocity);

    // If no axis is specified, return a vector of values.
    if (axis_in == mp_const_none) {
        return pb_type_Matrix_make_vector(3, angular_velocity.values, false);
    }

    // Otherwise convert user axis to pbio object and project vector onto it.
    pbio_geometry_xyz_t axis;
    pb_type_imu_extract_axis(axis_in, &axis);

    float projection;
    pb_assert(pbio_geometry_vector_project(&axis, &angular_velocity, &projection));
    return mp_obj_new_float_from_f(projection);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_imu_angular_velocity_obj, 1, pb_type_imu_angular_velocity);

// pybricks._common.IMU.rotation
static mp_obj_t pb_type_imu_rotation(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_imu_obj_t, self,
        PB_ARG_DEFAULT_NONE(axis));

    (void)self;

    // Otherwise convert user axis to pbio object and project vector onto it.
    pbio_geometry_xyz_t axis;
    pb_type_imu_extract_axis(axis_in, &axis);

    float rotation_angle;
    pb_assert(pbio_imu_get_single_axis_rotation(&axis, &rotation_angle));
    return mp_obj_new_float_from_f(rotation_angle);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_imu_rotation_obj, 1, pb_type_imu_rotation);

// pybricks._common.IMU.ready
static mp_obj_t pb_type_imu_ready(mp_obj_t self_in) {
    return mp_obj_new_bool(pbio_imu_is_ready());
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_imu_ready_obj, pb_type_imu_ready);

// pybricks._common.IMU.stationary
static mp_obj_t pb_type_imu_stationary(mp_obj_t self_in) {
    return mp_obj_new_bool(pbio_imu_is_stationary());
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_imu_stationary_obj, pb_type_imu_stationary);

// pybricks._common.IMU.settings
static mp_obj_t pb_type_imu_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_imu_obj_t, self,
        PB_ARG_DEFAULT_NONE(angular_velocity_threshold),
        PB_ARG_DEFAULT_NONE(acceleration_threshold),
        PB_ARG_DEFAULT_NONE(heading_correction),
        PB_ARG_DEFAULT_NONE(acceleration_correction));

    (void)self;

    // Return current values if no arguments are given.
    if (angular_velocity_threshold_in == mp_const_none &&
        acceleration_threshold_in == mp_const_none &&
        heading_correction_in == mp_const_none &&
        acceleration_correction_in == mp_const_none) {

        // Raises if not set, so can safely dereference.
        pbio_imu_persistent_settings_t *get_settings;
        pb_assert(pbio_imu_get_settings(&get_settings));

        mp_obj_t acceleration_corrections[] = {
            mp_obj_new_float_from_f(get_settings->gravity_x_pos),
            mp_obj_new_float_from_f(get_settings->gravity_x_neg),
            mp_obj_new_float_from_f(get_settings->gravity_y_pos),
            mp_obj_new_float_from_f(get_settings->gravity_y_neg),
            mp_obj_new_float_from_f(get_settings->gravity_z_pos),
            mp_obj_new_float_from_f(get_settings->gravity_z_neg),
        };

        mp_obj_t ret[] = {
            mp_obj_new_float_from_f(get_settings->gyro_stationary_threshold),
            mp_obj_new_float_from_f(get_settings->accel_stationary_threshold),
            mp_obj_new_float_from_f(get_settings->heading_correction),
            mp_obj_new_tuple(MP_ARRAY_SIZE(acceleration_corrections), acceleration_corrections),
        };
        return mp_obj_new_tuple(MP_ARRAY_SIZE(ret), ret);
    }

    // Apply new settings, using flags to indicate which should be updated.
    pbio_imu_persistent_settings_t set_settings = { 0 };
    if (angular_velocity_threshold_in != mp_const_none) {
        set_settings.flags |= PBIO_IMU_SETTINGS_FLAGS_GYRO_STATIONARY_THRESHOLD_SET;
        set_settings.gyro_stationary_threshold = mp_obj_get_float(angular_velocity_threshold_in);
    }

    if (acceleration_threshold_in != mp_const_none) {
        set_settings.flags |= PBIO_IMU_SETTINGS_FLAGS_ACCEL_STATIONARY_THRESHOLD_SET;
        set_settings.accel_stationary_threshold = mp_obj_get_float(acceleration_threshold_in);
    }

    if (heading_correction_in != mp_const_none) {
        set_settings.flags |= PBIO_IMU_SETTINGS_FLAGS_GYRO_HEADING_CORRECTION_SET;
        set_settings.heading_correction = mp_obj_get_float(heading_correction_in);
    }

    if (acceleration_correction_in != mp_const_none) {
        mp_obj_t *gravity;
        size_t size;
        mp_obj_get_array(acceleration_correction_in, &size, &gravity);
        if (size != 6) {
            mp_raise_ValueError(MP_ERROR_TEXT("Acceleration correction must be a 6-element tuple."));
        }
        set_settings.flags |= PBIO_IMU_SETTINGS_FLAGS_ACCEL_CALIBRATED;
        set_settings.gravity_x_pos = mp_obj_get_float(gravity[0]);
        set_settings.gravity_x_neg = mp_obj_get_float(gravity[1]);
        set_settings.gravity_y_pos = mp_obj_get_float(gravity[2]);
        set_settings.gravity_y_neg = mp_obj_get_float(gravity[3]);
        set_settings.gravity_z_pos = mp_obj_get_float(gravity[4]);
        set_settings.gravity_z_neg = mp_obj_get_float(gravity[5]);
    }

    pb_assert(pbio_imu_set_settings(&set_settings));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_imu_settings_obj, 1, pb_type_imu_settings);

// pybricks._common.IMU.heading
static mp_obj_t pb_type_imu_heading(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_float(pbio_imu_get_heading());
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_imu_heading_obj, pb_type_imu_heading);

// pybricks._common.IMU.reset_heading
static mp_obj_t pb_type_imu_reset_heading(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_imu_obj_t, self,
        PB_ARG_REQUIRED(angle));

    if (pbio_drivebase_any_uses_gyro()) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Can't reset heading while gyro in use. Stop driving first."));
    }

    // Set the new angle
    (void)self;
    pbio_imu_set_heading(mp_obj_get_float(angle_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_imu_reset_heading_obj, 1, pb_type_imu_reset_heading);

// pybricks._common.IMU.update_heading_correction
static mp_obj_t pb_type_imu_update_heading_correction(mp_obj_t self_in) {
    pb_type_imu_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_module_tools_assert_blocking();

    // Disable stop button and cache original setting to restore later.
    pbio_button_flags_t stop_button = pbsys_program_stop_get_buttons();

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t func = pb_function_import_helper(MP_QSTR__hub_extra, MP_QSTR_imu_update_heading_correction);
        mp_call_function_1(func, self->hub);
        pbsys_program_stop_set_buttons(stop_button);
        nlr_pop();
    } else {
        pbsys_program_stop_set_buttons(stop_button);
        nlr_jump(nlr.ret_val);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_imu_update_heading_correction_obj, pb_type_imu_update_heading_correction);

// dir(pybricks.common.IMU)
static const mp_rom_map_elem_t pb_type_imu_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acceleration),     MP_ROM_PTR(&pb_type_imu_acceleration_obj)    },
    { MP_ROM_QSTR(MP_QSTR_angular_velocity), MP_ROM_PTR(&pb_type_imu_angular_velocity_obj)},
    { MP_ROM_QSTR(MP_QSTR_heading),          MP_ROM_PTR(&pb_type_imu_heading_obj)         },
    { MP_ROM_QSTR(MP_QSTR_ready),            MP_ROM_PTR(&pb_type_imu_ready_obj)           },
    { MP_ROM_QSTR(MP_QSTR_reset_heading),    MP_ROM_PTR(&pb_type_imu_reset_heading_obj)   },
    { MP_ROM_QSTR(MP_QSTR_rotation),         MP_ROM_PTR(&pb_type_imu_rotation_obj)        },
    { MP_ROM_QSTR(MP_QSTR_settings),         MP_ROM_PTR(&pb_type_imu_settings_obj)        },
    { MP_ROM_QSTR(MP_QSTR_stationary),       MP_ROM_PTR(&pb_type_imu_stationary_obj)      },
    { MP_ROM_QSTR(MP_QSTR_tilt),             MP_ROM_PTR(&pb_type_imu_tilt_obj)            },
    { MP_ROM_QSTR(MP_QSTR_up),               MP_ROM_PTR(&pb_type_imu_up_obj)              },
    { MP_ROM_QSTR(MP_QSTR_update_heading_correction), MP_ROM_PTR(&pb_type_imu_update_heading_correction_obj)},
};
static MP_DEFINE_CONST_DICT(pb_type_imu_locals_dict, pb_type_imu_locals_dict_table);

// type(pybricks.common.IMU)
static MP_DEFINE_CONST_OBJ_TYPE(pb_type_IMU,
    MP_QSTR_IMU,
    MP_TYPE_FLAG_NONE,
    locals_dict, &pb_type_imu_locals_dict);

static pb_type_imu_obj_t singleton_imu_obj = {
    .base.type = &pb_type_IMU,
};

// pybricks._common.IMU.__init__
mp_obj_t pb_type_IMU_obj_new(mp_obj_t hub_in, mp_obj_t top_side_axis_in, mp_obj_t front_side_axis_in) {

    // Set user base orientation.
    pbio_geometry_xyz_t front_side_axis;
    pb_type_imu_extract_axis(front_side_axis_in, &front_side_axis);

    pbio_geometry_xyz_t top_side_axis;
    pb_type_imu_extract_axis(top_side_axis_in, &top_side_axis);

    pbio_imu_set_base_orientation(&front_side_axis, &top_side_axis);

    // Return singleton instance.
    singleton_imu_obj.hub = hub_in;
    return MP_OBJ_FROM_PTR(&singleton_imu_obj);
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU
