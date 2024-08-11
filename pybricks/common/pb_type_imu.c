// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#include <math.h>
#include <stdbool.h>
#include <string.h>

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
        PB_ARG_DEFAULT_NONE(heading_correction));

    (void)self;

    // Return current values if no arguments are given.
    if (angular_velocity_threshold_in == mp_const_none &&
        acceleration_threshold_in == mp_const_none &&
        heading_correction_in == mp_const_none) {
        float angular_velocity;
        float acceleration;
        float heading_correction;
        pbio_imu_get_settings(&angular_velocity, &acceleration, &heading_correction);
        mp_obj_t ret[] = {
            mp_obj_new_float_from_f(angular_velocity),
            mp_obj_new_float_from_f(acceleration),
            mp_obj_new_float_from_f(heading_correction),
        };
        return mp_obj_new_tuple(MP_ARRAY_SIZE(ret), ret);
    }

    // Otherwise set new values, only if given.
    pb_assert(pbio_imu_set_settings(
        angular_velocity_threshold_in == mp_const_none ? NAN : mp_obj_get_float(angular_velocity_threshold_in),
        acceleration_threshold_in == mp_const_none ? NAN : mp_obj_get_float(acceleration_threshold_in),
        heading_correction_in == mp_const_none ? NAN : mp_obj_get_float(heading_correction_in)
        ));

    // Request that changed settings are saved on shutdown.
    pbsys_storage_settings_save_imu_settings();
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
