#include "modmotor.h"

/*
DCMotor
class DCMotor():
    """Class for motors without encoders."""
*/

/*
DCMotor
    def __init__(self, port, direction):
        """Initialize the motor.
        Arguments:
            port {const} -- Port to which the device is connected: PORT_A, PORT_B, etc.
            direction {const} -- DIR_NORMAL or DIR_INVERTED (default: {DIR_NORMAL})
        """
*/
mp_obj_t motor_DCMotor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    motor_DCMotor_obj_t *self = m_new_obj(motor_DCMotor_obj_t);
    self->base.type = &motor_DCMotor_type;
    self->port = mp_obj_get_int(args[0]);
    int8_t direction = (n_args > 1) ? mp_obj_get_int(args[1]) : PBIO_MOTOR_DIR_NORMAL;
    pbio_dcmotor_set_constant_settings(self->port, direction);
    pbio_dcmotor_set_variable_settings(self->port, PBIO_MAX_DUTY_PCT);
    return MP_OBJ_FROM_PTR(self);
}

/*
DCMotor
    def __str__(self):
        """String representation of DCMotor object."""
*/
STATIC void motor_DCMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind ) {
    motor_DCMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    char dcmotor_settings_string[MAX_DCMOTOR_SETTINGS_STR_LENGTH];
    pbio_dcmotor_print_settings(self->port, dcmotor_settings_string);
    mp_printf(print, "%s", dcmotor_settings_string);
}

/*
DCMotor
    def settings(self, relative_torque_limit=100):
        """Update the motor settings.
        Arguments:
            relative_torque_limit {float} -- Percentage (-100.0 to 100.0) of the maximum stationary torque that the motor is allowed to produce.
*/
STATIC mp_obj_t motor_DCMotor_settings(mp_obj_t self_in, mp_obj_t stall_torque_limit) {
    motor_DCMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_dcmotor_set_variable_settings(self->port, mp_obj_get_float(stall_torque_limit));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_DCMotor_settings_obj, motor_DCMotor_settings);

/*
DCMotor
    def duty(self, duty):
        """Set motor duty cycle.
        Arguments:
            duty {float} -- Percentage from -100.0 to 100.0
*/
STATIC mp_obj_t motor_DCMotor_duty(mp_obj_t self_in, mp_obj_t duty_cycle) {    
    motor_DCMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_dcmotor_set_duty_cycle(self->port, mp_obj_get_float(duty_cycle));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_DCMotor_duty_obj, motor_DCMotor_duty);

/*
DCMotor
    def brake(self):
        """Stop by setting duty cycle to 0."""
*/
STATIC mp_obj_t motor_DCMotor_brake(mp_obj_t self_in) {
    motor_DCMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_dcmotor_brake(self->port);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_DCMotor_brake_obj, motor_DCMotor_brake);

/*
DCMotor
    def coast(self):
        """Coast the motor."""
*/
STATIC mp_obj_t motor_DCMotor_coast(mp_obj_t self_in) {
    motor_DCMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_dcmotor_coast(self->port);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_DCMotor_coast_obj, motor_DCMotor_coast);


/*
DCMotor Class tables
*/

// creating the table of members
STATIC const mp_rom_map_elem_t motor_DCMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_settings), MP_ROM_PTR(&motor_DCMotor_settings_obj) },    
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_DCMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&motor_DCMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_DCMotor_duty_obj) },
};

STATIC MP_DEFINE_CONST_DICT(motor_DCMotor_locals_dict, motor_DCMotor_locals_dict_table);

// create the class-object itself
const mp_obj_type_t motor_DCMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_DCMotor,
    .print = motor_DCMotor_print,
    .make_new = motor_DCMotor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_DCMotor_locals_dict,
};

/*
EncodedMotor
class EncodedMotor(DCMotor):
    """Class for motors with encoders."""
*/

/*
EncodedMotor
    def __init__(self, port, direction):
        """Initialize the motor.
        Arguments:
            port {const} -- Port to which the device is connected: PORT_A, PORT_B, etc.
            direction {const} -- DIR_NORMAL or DIR_INVERTED (default: {DIR_NORMAL})
            gear_ratio {float} -- Absolute slow down factor of a gear train (default: {1.0})
        """
*/
mp_obj_t motor_EncodedMotor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    mp_arg_check_num(n_args, n_kw, 1, 3, false);
    motor_EncodedMotor_obj_t *self = m_new_obj(motor_EncodedMotor_obj_t);
    self->base.type = &motor_EncodedMotor_type;
    self->port = mp_obj_get_int(args[0]);
    int8_t direction = (n_args > 1) ? mp_obj_get_int(args[1]) : PBIO_MOTOR_DIR_NORMAL;
    float_t gear_ratio = (n_args >= 3) ? mp_obj_get_float(args[2]): 1.0;
    pbio_dcmotor_set_constant_settings(self->port, direction);
    pbio_dcmotor_set_variable_settings(self->port, PBIO_MAX_DUTY);
    // Set default settings for an encoded motor. Inherited classes can provide device specific parameters
    pbio_encmotor_set_constant_settings(self->port, 1, gear_ratio);
    pbio_encmotor_set_variable_settings(self->port, 1000, 1, 1000, 1000, 100, 5000, 5000, 50);
    return MP_OBJ_FROM_PTR(self);
}

/*
EncodedMotor
    def __str__(self):
        """String representation of DCMotor object."""
*/
STATIC void motor_EncodedMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind){
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    char dcmotor_settings_string[MAX_DCMOTOR_SETTINGS_STR_LENGTH];
    pbio_dcmotor_print_settings(self->port, dcmotor_settings_string);
    char encmotor_settings_string[MAX_ENCMOTOR_SETTINGS_STR_LENGTH];
    pbio_encmotor_print_settings(self->port, encmotor_settings_string);
    mp_printf(print, "%s\n%s", dcmotor_settings_string, encmotor_settings_string);
}

/*
EncodedMotor
    def settings(self, relative_torque_limit, max_speed, tolerance, acceleration_start, acceleration_end, tight_loop_time_ms, pid_kp, pid_ki, pid_kd):
        """Update the motor settings.
        Arguments:
        [1] relative_torque_limit {float}   -- Percentage (-100.0 to 100.0) of the maximum stationary torque that the motor is allowed to produce.
        [2] max_speed {float}               -- Soft limit on the reference speed in all run commands
        [3] tolerance {float}               -- Allowed deviation (deg) from target before motion is considered complete
        [4] acceleration_start {float}      -- Acceleration when beginning to move. Positive value in degrees per second per second
        [5] acceleration_end {float}        -- Deceleration when stopping. Positive value in degrees per second per second
        [6] tight_loop_time {float}         -- When a run function is called twice in this interval (seconds), assume that the user is doing their own speed control.
        [7] pid_kp {float}                  -- Proportional position control constant (and integral speed control constant)
        [8] pid_ki {float}                  -- Integral position control constant
        [9] pid_kd {float}                  -- Derivative position control constant (and proportional speed control constant)
*/
STATIC mp_obj_t motor_EncodedMotor_settings(size_t n_args, const mp_obj_t *args){
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(args[0]);    
    pbio_dcmotor_set_variable_settings(self->port, mp_obj_get_float(args[1]));
    pbio_encmotor_set_variable_settings(self->port, 
                                     (int16_t) mp_obj_get_float(args[2]),
                                     (int16_t) mp_obj_get_float(args[3]),
                                     (int16_t) mp_obj_get_float(args[4]),
                                     (int16_t) mp_obj_get_float(args[5]),
                                     (int16_t) (mp_obj_get_float(args[6]) * MS_PER_SECOND),
                                     (int16_t) (mp_obj_get_float(args[7]) * PID_PRESCALE),
                                     (int16_t) (mp_obj_get_float(args[8]) * PID_PRESCALE),
                                     (int16_t) (mp_obj_get_float(args[9]) * PID_PRESCALE)
                                     );
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_EncodedMotor_settings_obj, 10, 10, motor_EncodedMotor_settings);

/*
EncodedMotor Class tables
*/

STATIC const mp_rom_map_elem_t motor_EncodedMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_DCMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&motor_DCMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_DCMotor_duty_obj) },
    { MP_ROM_QSTR(MP_QSTR_settings), MP_ROM_PTR(&motor_EncodedMotor_settings_obj) },
};

STATIC MP_DEFINE_CONST_DICT(motor_EncodedMotor_locals_dict, motor_EncodedMotor_locals_dict_table);

const mp_obj_type_t motor_EncodedMotor_type = {
    { &mp_type_type },
    // { &motor_DCMotor_type }, // or this?
    .name = MP_QSTR_EncodedMotor,
    .print = motor_EncodedMotor_print,
    .make_new = motor_EncodedMotor_make_new,
    .parent = &motor_DCMotor_type, //Inherit from DCMotor (?)
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};

/*
Motor module tables
*/

STATIC const mp_map_elem_t motor_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR__motor) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_DCMotor), (mp_obj_t)&motor_DCMotor_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_EncodedMotor), (mp_obj_t)&motor_EncodedMotor_type},
};

STATIC MP_DEFINE_CONST_DICT (mp_module_motor_globals, motor_globals_table);

const mp_obj_module_t mp_module_motor = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_motor_globals,
};
