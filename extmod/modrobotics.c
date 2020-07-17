// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdlib.h>
#include <inttypes.h>

#include <math.h>

#include <pbio/drivebase.h>
#include <pbio/motorpoll.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"

#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

#include "modparameters.h"
#include "modbuiltins.h"
#include "modmotor.h"
#include "modlogger.h"

#if PYBRICKS_PY_ROBOTICS

// pybricks.robotics.DriveBase class object
typedef struct _robotics_DriveBase_obj_t {
    mp_obj_base_t base;
    pbio_drivebase_t *db;
    mp_obj_t left;
    mp_obj_t right;
    mp_obj_t logger;
    mp_obj_t heading_control;
    mp_obj_t distance_control;
    int32_t straight_speed;
    int32_t straight_acceleration;
    int32_t turn_rate;
    int32_t turn_acceleration;
} robotics_DriveBase_obj_t;

// pybricks.robotics.DriveBase.__init__
STATIC mp_obj_t robotics_DriveBase_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(left_motor),
        PB_ARG_REQUIRED(right_motor),
        PB_ARG_REQUIRED(wheel_diameter),
        PB_ARG_REQUIRED(axle_track));

    robotics_DriveBase_obj_t *self = m_new_obj(robotics_DriveBase_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    // Pointer to the Python (not pbio) Motor objects
    self->left = left_motor;
    self->right = right_motor;

    // Pointers to servos
    pbio_servo_t *srv_left = ((motor_Motor_obj_t *)pb_obj_get_base_class_obj(self->left, &motor_Motor_type))->srv;
    pbio_servo_t *srv_right = ((motor_Motor_obj_t *)pb_obj_get_base_class_obj(self->right, &motor_Motor_type))->srv;

    // A DriveBase must have two distinct motors
    if (srv_left == srv_right) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Create drivebase
    pb_assert(pbio_motorpoll_get_drivebase(&self->db));
    pb_assert(pbio_drivebase_setup(self->db, srv_left, srv_right, pb_obj_get_fix16(wheel_diameter), pb_obj_get_fix16(axle_track)));
    pb_assert(pbio_motorpoll_set_drivebase_status(self->db, PBIO_ERROR_AGAIN));

    // Create an instance of the Logger class
    self->logger = logger_obj_make_new(&self->db->log);

    // Create instances of the Control class
    self->heading_control = builtins_Control_obj_make_new(&self->db->control_heading);
    self->distance_control = builtins_Control_obj_make_new(&self->db->control_distance);

    // Get defaults for drivebase as 1/3 of maximum for the underlying motors
    int32_t straight_speed_limit, straight_acceleration_limit, turn_rate_limit, turn_acceleration_limit, _;
    pbio_control_settings_get_limits(&self->db->control_distance.settings, &straight_speed_limit, &straight_acceleration_limit, &_);
    pbio_control_settings_get_limits(&self->db->control_heading.settings, &turn_rate_limit, &turn_acceleration_limit, &_);

    self->straight_speed = straight_speed_limit / 3;
    self->straight_acceleration = straight_acceleration_limit / 3;
    self->turn_rate = turn_rate_limit / 3;
    self->turn_acceleration = turn_acceleration_limit / 3;

    return MP_OBJ_FROM_PTR(self);
}

STATIC void wait_for_completion_drivebase(pbio_drivebase_t *db) {
    pbio_error_t err;
    while ((err = pbio_motorpoll_get_drivebase_status(db)) == PBIO_ERROR_AGAIN && (!pbio_control_is_done(&db->control_distance) || !pbio_control_is_done(&db->control_heading))) {
        mp_hal_delay_ms(5);
    }
    if (err != PBIO_ERROR_AGAIN) {
        pb_assert(err);
    }
}

// pybricks.robotics.DriveBase.straight
STATIC mp_obj_t robotics_DriveBase_straight(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(distance));

    int32_t distance_val = pb_obj_get_int(distance);
    pb_assert(pbio_drivebase_straight(self->db, distance_val, self->straight_speed, self->straight_acceleration));

    wait_for_completion_drivebase(self->db);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_straight_obj, 1, robotics_DriveBase_straight);

// pybricks.robotics.DriveBase.turn
STATIC mp_obj_t robotics_DriveBase_turn(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(angle));

    int32_t angle_val = pb_obj_get_int(angle);
    pb_assert(pbio_drivebase_turn(self->db, angle_val, self->turn_rate, self->turn_acceleration));

    wait_for_completion_drivebase(self->db);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_turn_obj, 1, robotics_DriveBase_turn);

// pybricks.robotics.DriveBase.drive
STATIC mp_obj_t robotics_DriveBase_drive(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(turn_rate));

    // Get wheel diameter and axle track dimensions
    int32_t speed_val = pb_obj_get_int(speed);
    int32_t turn_rate_val = pb_obj_get_int(turn_rate);

    pb_assert(pbio_drivebase_drive(self->db, speed_val, turn_rate_val));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_drive_obj, 1, robotics_DriveBase_drive);

// pybricks.builtins.DriveBase.stop
STATIC mp_obj_t robotics_DriveBase_stop(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_drivebase_stop(self->db, PBIO_ACTUATION_COAST));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_stop_obj, robotics_DriveBase_stop);

// pybricks.builtins.DriveBase.distance
STATIC mp_obj_t robotics_DriveBase_distance(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, angle, turn_rate;
    pb_assert(pbio_drivebase_get_state(self->db, &distance, &drive_speed, &angle, &turn_rate));

    return mp_obj_new_int(distance);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_distance_obj, robotics_DriveBase_distance);

// pybricks.builtins.DriveBase.angle
STATIC mp_obj_t robotics_DriveBase_angle(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, angle, turn_rate;
    pb_assert(pbio_drivebase_get_state(self->db, &distance, &drive_speed, &angle, &turn_rate));

    return mp_obj_new_int(angle);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_angle_obj, robotics_DriveBase_angle);

// pybricks.builtins.DriveBase.state
STATIC mp_obj_t robotics_DriveBase_state(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, angle, turn_rate;
    pb_assert(pbio_drivebase_get_state(self->db, &distance, &drive_speed, &angle, &turn_rate));

    mp_obj_t ret[4];
    ret[0] = mp_obj_new_int(distance);
    ret[1] = mp_obj_new_int(drive_speed);
    ret[2] = mp_obj_new_int(angle);
    ret[3] = mp_obj_new_int(turn_rate);

    return mp_obj_new_tuple(4, ret);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_state_obj, robotics_DriveBase_state);


// pybricks.builtins.DriveBase.reset
STATIC mp_obj_t robotics_DriveBase_reset(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_assert(pbio_drivebase_reset_state(self->db));

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_reset_obj, robotics_DriveBase_reset);

// pybricks.robotics.DriveBase.settings
STATIC mp_obj_t robotics_DriveBase_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_DEFAULT_NONE(straight_speed),
        PB_ARG_DEFAULT_NONE(straight_acceleration),
        PB_ARG_DEFAULT_NONE(turn_rate),
        PB_ARG_DEFAULT_NONE(turn_acceleration));

    // If all given values are none, return current values
    if (straight_speed == mp_const_none &&
        straight_acceleration == mp_const_none &&
        turn_rate == mp_const_none &&
        turn_acceleration == mp_const_none
        ) {
        mp_obj_t ret[4];
        ret[0] = mp_obj_new_int(self->straight_speed);
        ret[1] = mp_obj_new_int(self->straight_acceleration);
        ret[2] = mp_obj_new_int(self->turn_rate);
        ret[3] = mp_obj_new_int(self->turn_acceleration);
        return mp_obj_new_tuple(4, ret);
    }

    if (self->db->control_distance.type != PBIO_CONTROL_NONE || self->db->control_heading.type != PBIO_CONTROL_NONE) {
        pb_assert(PBIO_ERROR_INVALID_OP);
    }

    // If some values are given, set them, bound by the control limits
    int32_t straight_speed_limit, straight_acceleration_limit, turn_rate_limit, turn_acceleration_limit, _;
    pbio_control_settings_get_limits(&self->db->control_distance.settings, &straight_speed_limit, &straight_acceleration_limit, &_);
    pbio_control_settings_get_limits(&self->db->control_heading.settings, &turn_rate_limit, &turn_acceleration_limit, &_);

    self->straight_speed = min(straight_speed_limit, abs(pb_obj_get_default_int(straight_speed, self->straight_speed)));
    self->straight_acceleration = min(straight_acceleration_limit, abs(pb_obj_get_default_int(straight_acceleration, self->straight_acceleration)));
    self->turn_rate = min(turn_rate_limit, abs(pb_obj_get_default_int(turn_rate, self->turn_rate)));
    self->turn_acceleration = min(turn_acceleration_limit, abs(pb_obj_get_default_int(turn_acceleration, self->turn_acceleration)));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_settings_obj, 1, robotics_DriveBase_settings);

// dir(pybricks.robotics.DriveBase)
STATIC const mp_rom_map_elem_t robotics_DriveBase_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_straight),         MP_ROM_PTR(&robotics_DriveBase_straight_obj) },
    { MP_ROM_QSTR(MP_QSTR_turn),             MP_ROM_PTR(&robotics_DriveBase_turn_obj)     },
    { MP_ROM_QSTR(MP_QSTR_drive),            MP_ROM_PTR(&robotics_DriveBase_drive_obj)    },
    { MP_ROM_QSTR(MP_QSTR_stop),             MP_ROM_PTR(&robotics_DriveBase_stop_obj)     },
    { MP_ROM_QSTR(MP_QSTR_distance),         MP_ROM_PTR(&robotics_DriveBase_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_angle),            MP_ROM_PTR(&robotics_DriveBase_angle_obj)    },
    { MP_ROM_QSTR(MP_QSTR_state),            MP_ROM_PTR(&robotics_DriveBase_state_obj)    },
    { MP_ROM_QSTR(MP_QSTR_reset),            MP_ROM_PTR(&robotics_DriveBase_reset_obj)    },
    { MP_ROM_QSTR(MP_QSTR_settings),         MP_ROM_PTR(&robotics_DriveBase_settings_obj) },
    { MP_ROM_QSTR(MP_QSTR_left),             MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, left)            },
    { MP_ROM_QSTR(MP_QSTR_right),            MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, right)           },
    { MP_ROM_QSTR(MP_QSTR_log),              MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, logger)          },
    { MP_ROM_QSTR(MP_QSTR_heading_control),  MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, heading_control) },
    { MP_ROM_QSTR(MP_QSTR_distance_control), MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, distance_control)},
};
STATIC MP_DEFINE_CONST_DICT(robotics_DriveBase_locals_dict, robotics_DriveBase_locals_dict_table);

// type(pybricks.robotics.DriveBase)
STATIC const mp_obj_type_t robotics_DriveBase_type = {
    { &mp_type_type },
    .name = MP_QSTR_DriveBase,
    .make_new = robotics_DriveBase_make_new,
    .locals_dict = (mp_obj_dict_t *)&robotics_DriveBase_locals_dict,
};

#if MICROPY_PY_BUILTINS_FLOAT

// Class structure for Matrix
typedef struct _robotics_Matrix_obj_t {
    mp_obj_base_t base;
    float_t *data;
    float_t scale;
    size_t m;
    size_t n;
    bool transposed;
} robotics_Matrix_obj_t;


// pybricks.robotics.Matrix.__init__
STATIC mp_obj_t robotics_Matrix_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(rows));

    robotics_Matrix_obj_t *self = m_new_obj(robotics_Matrix_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    // Unpack the main list of rows
    mp_obj_t *row_objs, *scalar_objs;
    mp_obj_get_array(rows, &self->m, &row_objs);

    if (self->m == 0) {
        // TODO: raise dimension error, m >= 1
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Iterate through each of the rows to get the scalars
    for (size_t r = 0; r < self->m; r++) {

        size_t n;
        mp_obj_get_array(row_objs[r], &n, &scalar_objs);

        if (r == 0) {
            if (n == 0) {
                // TODO: raise dimension error, n >= 1
                pb_assert(PBIO_ERROR_INVALID_ARG);
            }

            self->n = n;
            self->data = m_new(float_t, self->m * self->n);
        } else { // other rows
            if (n != self->n) {
                // TODO: raise dimension error
                pb_assert(PBIO_ERROR_INVALID_ARG);
            }
        }

        // Unpack the scalars
        for (size_t c = 0; c < self->n; c++) {
            self->data[r * self->n + c] = mp_obj_get_float_to_f(scalar_objs[c]);
        }
    }

    // Modifiers that allow basic modifications without moving data around
    self->scale = 1;
    self->transposed = false;

    return MP_OBJ_FROM_PTR(self);
}

const mp_obj_type_t robotics_Matrix_type;


// Get string representation of the form -123.456
static void print_float(char *buf, float_t x) {

    // Convert to positive int
    int32_t val = (int32_t)(x * 1000);
    bool negative = val < 0;
    if (negative) {
        val = -val;
    }

    // Limit to 999.999
    if (val >= 999999) {
        val = 999999;
    }

    // Get number of printed digits so we know where to add minus sign later
    int32_t c = snprintf(NULL, 0, "%" PRId32, val);

    // Print the integer
    snprintf(buf, 8, "%7" PRId32, val);

    // If the number was negative, add minus sign in the right place
    if (negative) {
        buf[6 - c <= 2 ? 6 - c : 2] = '-';
    }

    // If there is nothing before the decimal, ensure there is a zero
    buf[3] = buf[3] == ' ' ? '0' : buf[3];

    // Shift last 3 digits to make room for decimal point and insert 0 if empty
    buf[7] = buf[6] == ' ' ? '0' : buf[6];
    buf[6] = buf[5] == ' ' ? '0' : buf[5];
    buf[5] = buf[4] == ' ' ? '0' : buf[4];

    // Terminate and add decimal separator
    buf[4] = '.';
}

// pybricks.robotics.Matrix.__repr__
void robotics_Matrix_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {

    robotics_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Each "-123.456" is 8. Each digit is appended with ", " or "]\n", so 10 per digit
    // Also each row starts with "[[" or " [" so we add 2*rows. Finally we add \0, so +1.
    size_t len = self->m * self->n * 10 + self->m * 2 + 1;

    // Allocate the buffer
    char *buf = m_new(char, len);

    // Loop through the rows
    for (size_t r = 0; r < self->m; r++) {

        // Character starting index for this row
        size_t row_start = r * (self->n * 10 + 2);

        // Prepend "[[" on first row, else " ["
        buf[row_start] = r == 0 ? '[' : ' ';
        buf[row_start + 1] = '[';

        // Loop through the colums, so the scalars in each row
        for (size_t c = 0; c < self->n; c++) {

            // Starting character index for this column
            size_t col_start = row_start + c * 10 + 2;

            // Get data index of the scalar we will print. Transposed attribute
            // tells us whether data is stored row by row or column by column.
            // (i, j) -> index:
            // regular:    i * self->n + j
            // transposed: j * self->m + i
            size_t idx = self->transposed ? c * self->m + r : r * self->n + c;

            // Get character representation of said value
            print_float(buf + col_start, self->data[idx] * self->scale);

            // Append ", " or "]\n" after the last value
            if (c < self->n - 1) {
                buf[col_start + 8] = ',';
                buf[col_start + 9] = ' ';
            } else {
                buf[col_start + 8] = ']';
                buf[col_start + 9] = '\n';
            }
        }
    }
    // Close the group of rows with "]"
    buf[len - 2 ] = ']';
    buf[len - 1 ] = '\0';

    // Send the bufer to MicroPython
    mp_print_str(print, buf);
}

// pybricks.robotics.Matrix._mul
STATIC mp_obj_t robotics_Matrix__mul(mp_obj_t lhs_obj, mp_obj_t rhs_obj) {

    // Get left and right matrices
    robotics_Matrix_obj_t *lhs = MP_OBJ_TO_PTR(lhs_obj);
    robotics_Matrix_obj_t *rhs = MP_OBJ_TO_PTR(rhs_obj);

    // Verify matching dimensions else raise error

    // Result has as many rows as left hand side and as many columns as right hand side.
    robotics_Matrix_obj_t *ret = m_new_obj(robotics_Matrix_obj_t);
    ret->base.type = &robotics_Matrix_type;
    ret->m = lhs->m;
    ret->n = rhs->n;
    ret->data = m_new(float_t, ret->m * ret->n);

    // Scale is commutative, so we can do it separately
    ret->scale = lhs->scale * rhs->scale;
    ret->transposed = false;

    // Multiply the matrices by looping over rows and columns
    for (size_t r = 0; r < ret->m; r++) {
        for (size_t c = 0; c < ret->m; c++) {
            // This entry is obtained as the sum of the products of the entries
            // of the r'th row of lhs and the c'th column of rhs, so size lhs->n.
            float_t sum = 0;
            for (size_t k = 0; k < lhs->n; k++) {
                // k'th entry on the r'th row (i = r, j = k)
                size_t lhs_idx = lhs->transposed ? k * lhs->m + r : r * lhs->n + k;
                // k'th entry on the c'th column  (i = k, j = c)
                size_t rhs_idx = rhs->transposed ? c * rhs->m + k : k * rhs->n + c;
                sum += lhs->data[lhs_idx] * rhs->data[rhs_idx];
            }
            ret->data[ret->n * r + c] = sum;
        }
    }

    return MP_OBJ_FROM_PTR(ret);
}

// pybricks.robotics.Matrix._scale
STATIC mp_obj_t robotics_Matrix__scale(mp_obj_t self_in, float_t scale) {
    robotics_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    robotics_Matrix_obj_t *copy = m_new_obj(robotics_Matrix_obj_t);
    copy->base.type = &robotics_Matrix_type;

    // Point to the same data instead of copying
    copy->data = self->data;
    copy->n = self->n;
    copy->m = self->m;
    copy->scale = self->scale * scale;
    copy->transposed = self->transposed;

    return MP_OBJ_FROM_PTR(copy);
}

// pybricks.robotics.Matrix.T
STATIC mp_obj_t robotics_Matrix_T(mp_obj_t self_in) {
    robotics_Matrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    robotics_Matrix_obj_t *copy = m_new_obj(robotics_Matrix_obj_t);
    copy->base.type = &robotics_Matrix_type;

    // Point to the same data instead of copying
    copy->data = self->data;
    copy->n = self->m;
    copy->m = self->n;
    copy->scale = self->scale;
    copy->transposed = !self->transposed;

    return MP_OBJ_FROM_PTR(copy);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(robotics_Matrix_T_obj, robotics_Matrix_T);


STATIC mp_obj_t robotics_Matrix_unary_op(mp_unary_op_t op, mp_obj_t o_in) {

    robotics_Matrix_obj_t *self = MP_OBJ_TO_PTR(o_in);

    switch (op) {
        // len returns the size of the matrix: (m, n)
        case MP_UNARY_OP_LEN: {
            mp_obj_t shape[2];
            shape[0] = MP_OBJ_NEW_SMALL_INT(self->m);
            shape[1] = MP_OBJ_NEW_SMALL_INT(self->n);
            return mp_obj_new_tuple(2, shape);
        }
        // Hash is the same as in generic unary op
        case MP_UNARY_OP_HASH:
            return MP_OBJ_NEW_SMALL_INT((mp_uint_t)o_in);
        // Positive just returns the original object
        case MP_UNARY_OP_POSITIVE:
            return o_in;
        // Negative returns a scaled copy
        case MP_UNARY_OP_NEGATIVE:
            return robotics_Matrix__scale(o_in, -1);
        // Get absolute vale (magnitude)
        case MP_UNARY_OP_ABS: {
            // For vectors, this is the norm
            if (self->m == 1 || self->n == 1) {
                size_t len = self->m * self->n;
                float squares = 0;
                for (size_t i = 0; i < len; i++) {
                    squares += self->data[i] * self->data[i];
                }
                return mp_obj_new_float(sqrtf(squares));
            }
            // Determinant not implemented
            return MP_OBJ_NULL;
        }
        default:
            // Other operations are not supported
            return MP_OBJ_NULL;
    }
}

STATIC mp_obj_t robotics_Matrix_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {

    switch (op) {
        case MP_BINARY_OP_MULTIPLY:
        case MP_BINARY_OP_INPLACE_MULTIPLY:
            return robotics_Matrix__mul(lhs_in, rhs_in);
        default:
            // Other operations not supported
            return MP_OBJ_NULL;
    }
}

// dir(pybricks.robotics.Matrix)
STATIC const mp_rom_map_elem_t robotics_Matrix_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_T),     MP_ROM_PTR(&robotics_Matrix_T_obj)              },
};
STATIC MP_DEFINE_CONST_DICT(robotics_Matrix_locals_dict, robotics_Matrix_locals_dict_table);

// type(pybricks.robotics.Matrix)
const mp_obj_type_t robotics_Matrix_type = {
    { &mp_type_type },
    .name = MP_QSTR_Matrix,
    .print = robotics_Matrix_print,
    .make_new = robotics_Matrix_make_new,
    .unary_op = robotics_Matrix_unary_op,
    .binary_op = robotics_Matrix_binary_op,
    .locals_dict = (mp_obj_dict_t *)&robotics_Matrix_locals_dict,
};


#endif

// dir(pybricks.robotics)
STATIC const mp_rom_map_elem_t robotics_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_robotics)         },
    { MP_ROM_QSTR(MP_QSTR_DriveBase),   MP_ROM_PTR(&robotics_DriveBase_type)  },
    #if MICROPY_PY_BUILTINS_FLOAT
    { MP_ROM_QSTR(MP_QSTR_Matrix),   MP_ROM_PTR(&robotics_Matrix_type)  },
    #endif // MICROPY_PY_BUILTINS_FLOAT
};
STATIC MP_DEFINE_CONST_DICT(pb_module_robotics_globals, robotics_globals_table);

const mp_obj_module_t pb_module_robotics = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_robotics_globals,
};

#endif // PYBRICKS_PY_ROBOTICS
