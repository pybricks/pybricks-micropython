#include "modmotor.h"

/*
DCMotor
class DCMotor():
    """Class for motors without encoders."""
*/

// Class structure for DC Motors
typedef struct _motor_DCMotor_obj_t {
    mp_obj_base_t base;
    pbio_port_t port;
} motor_DCMotor_obj_t;

/*
DCMotor
    def __init__(self, port, direction):
        """Initialize the motor.
        Arguments:
            port {const} -- Port to which the device is connected: PORT_A, PORT_B, etc.
            direction {const} -- DIR_NORMAL or DIR_INVERTED (default: {DIR_NORMAL})
        """
*/

mp_obj_t motor_DCMotor_make_new(const mp_obj_type_id_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    motor_DCMotor_obj_t *self = m_new_obj(motor_DCMotor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    self->port = mp_obj_get_int(args[0]);
    int8_t direction = (n_args > 1) ? mp_obj_get_int(args[1]) : PBIO_MOTOR_DIR_NORMAL;
    pbio_dcmotor_setup(self->port, type->device_id, direction);
    return MP_OBJ_FROM_PTR(self);
}

/*
DCMotor
    def __str__(self):
        """String representation of DCMotor object."""
*/
void motor_DCMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind ) {
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
    pbio_dcmotor_set_settings(self->port, mp_obj_get_float(stall_torque_limit));
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
    { MP_ROM_QSTR(MP_QSTR_device_id), MP_OBJ_NEW_SMALL_INT(PBIO_ID_UNKNOWN_DCMOTOR) },
    { MP_ROM_QSTR(MP_QSTR_settings), MP_ROM_PTR(&motor_DCMotor_settings_obj) },    
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_DCMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&motor_DCMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_DCMotor_duty_obj) },
};

MP_DEFINE_CONST_DICT(motor_DCMotor_locals_dict, motor_DCMotor_locals_dict_table);

// create the class-object itself
const mp_obj_type_id_t motor_DCMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_DCMotor,
    .print = motor_DCMotor_print,
    .make_new = motor_DCMotor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_DCMotor_locals_dict,
    .device_id = PBIO_ID_UNKNOWN_DCMOTOR,
};

/*
EncodedMotor
class EncodedMotor(DCMotor):
    """Class for motors with encoders."""
*/


// Class structure for Encoded Motors
typedef struct _motor_EncodedMotor_obj_t {
    mp_obj_base_t base;
    uint8_t port;
} motor_EncodedMotor_obj_t;


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
mp_obj_t motor_EncodedMotor_make_new(const mp_obj_type_id_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    mp_arg_check_num(n_args, n_kw, 1, 3, false);
    motor_EncodedMotor_obj_t *self = m_new_obj(motor_EncodedMotor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    self->port = mp_obj_get_int(args[0]);    
    int8_t direction = (n_args > 1) ? mp_obj_get_int(args[1]) : PBIO_MOTOR_DIR_NORMAL;
    float_t gear_ratio = (n_args >= 3) ? mp_obj_get_float(args[2]): 1.0;
    pbio_encmotor_setup(self->port, type->device_id, direction, gear_ratio);
    return MP_OBJ_FROM_PTR(self);
}

/*
EncodedMotor
    def __str__(self):
        """String representation of DCMotor object."""
*/
void motor_EncodedMotor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind){
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
        Keyword Arguments (TODO):
        [1] relative_torque_limit {float}   -- Percentage (-100.0 to 100.0) of the maximum stationary torque that the motor is allowed to produce.
        [2] max_speed {float}               -- Soft limit on the reference speed in all run commands
        [3] tolerance {float}               -- Allowed deviation (deg) from target before motion is considered complete
        [4] acceleration_start {float}      -- Acceleration when beginning to move. Positive value in degrees per second per second
        [5] acceleration_end {float}        -- Deceleration when stopping. Positive value in degrees per second per second
        [6] tight_loop_time {float}         -- When a run function is called twice in this interval (seconds), assume that the user is doing their own speed control.
        [7] pid_kp {float}                  -- Proportional angle control constant (and integral speed control constant)
        [8] pid_ki {float}                  -- Integral angle control constant
        [9] pid_kd {float}                  -- Derivative angle control constant (and proportional speed control constant)
*/
STATIC mp_obj_t motor_EncodedMotor_settings(size_t n_args, const mp_obj_t *args){
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(args[0]);    
    pbio_encmotor_set_settings(self->port, 
                                     mp_obj_get_float(args[1]),
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
EncodedMotor
    def angle(self):
        """Return the angle of the motor/mechanism (degrees).
        Returns:
            float -- Position of the motor/mechanism (degrees).
        """
*/
STATIC mp_obj_t motor_EncodedMotor_angle(mp_obj_t self_in) {
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    float_t angle;
    pbio_motor_get_angle(self->port, &angle);
    return mp_obj_new_float(angle);
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_EncodedMotor_angle_obj, motor_EncodedMotor_angle);

/*
EncodedMotor
    def reset_angle(self, reset_angle):
        """Reset the angle of the motor/mechanism (degrees).
        Arguments:
            reset_angle {const} -- Value to which the rotation sensor angle should be reset (default: {0})        
*/
STATIC mp_obj_t motor_EncodedMotor_reset_angle(size_t n_args, const mp_obj_t *args){
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(args[0]);    
    float_t reset_angle = n_args > 1 ? mp_obj_get_float(args[1]) : 0.0;
    pbio_motor_reset_angle(self->port, reset_angle);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_EncodedMotor_reset_angle_obj, 1, 2, motor_EncodedMotor_reset_angle);

/*
EncodedMotor
    def speed(self):
        """Return the angular speed of the motor/mechanism (degrees per second).
        Returns:
            float -- Angular speed of the motor/mechanism (degrees per second).
        """
*/
STATIC mp_obj_t motor_EncodedMotor_speed(mp_obj_t self_in) {
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    float_t speed;
    pbio_motor_get_angular_rate(self->port, &speed);
    return mp_obj_new_float(speed);
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_EncodedMotor_speed_obj, motor_EncodedMotor_speed);

/*
EncodedMotor
    def run(self, speed):
        """Start and keep running the motor/mechanism at the given speed (degrees per second).
        Arguments:
            speed {float} -- Target speed (degrees per second)
        """
*/
STATIC mp_obj_t motor_EncodedMotor_run(mp_obj_t self_in, mp_obj_t speed) {
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_motor_run(self->port, mp_obj_get_float(speed));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_EncodedMotor_run_obj, motor_EncodedMotor_run);

/*
EncodedMotor
    def stop(self, after_stop=COAST, wait=True):
        """Stop a motor/mechanism.
        Keyword Arguments (TODO):
            after_stop {const} -- What to do after the motor stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for complete stop (True) or decelerate in the background (False). (default: {True})
        """
*/
STATIC mp_obj_t motor_EncodedMotor_stop(mp_obj_t self_in, mp_obj_t after_stop, mp_obj_t wait) {
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);    
    pbio_motor_stop(self->port, mp_obj_get_int(after_stop), mp_obj_get_int(wait));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(motor_EncodedMotor_stop_obj, motor_EncodedMotor_stop);

/*
EncodedMotor
    def run_time(self, speed, duration, after_stop=COAST, wait=True):
        """Run a motor/mechanism at the given speed for a given duration. Then stop.
        Arguments:
            speed {float} -- Target speed (degrees per second)
            duration {float} -- Total duration, including start and stop (seconds)
        Keyword Arguments (TODO):
            after_stop {const} -- What to do after the motor stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})
        """
*/
STATIC mp_obj_t motor_EncodedMotor_run_time(size_t n_args, const mp_obj_t *args){
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(args[0]);    
    pbio_motor_run_time(self->port, mp_obj_get_float(args[1]), mp_obj_get_float(args[2]), mp_obj_get_int(args[3]), mp_obj_get_int(args[4]));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_EncodedMotor_run_time_obj, 5, 5, motor_EncodedMotor_run_time);

/*
EncodedMotor
    def run_stalled(self, speed, after_stop=COAST, wait=True):
        """Run a motor/mechanism at the given speed until it stalls. Then stop.
        Arguments:
            speed {float} -- Target speed (degrees per second)
        Keyword Arguments (TODO):
            after_stop {const} -- What to do after the motor stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})
        Returns:
            float -- If wait is True, then return the angle (degrees) at the time of stalling, otherwise return None
        """
*/
STATIC mp_obj_t motor_EncodedMotor_run_stalled(size_t n_args, const mp_obj_t *args){
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(args[0]);    
    float_t stall_point;
    pbio_motor_wait_t wait = mp_obj_get_int(args[3]);
    pbio_motor_run_stalled(self->port, mp_obj_get_float(args[1]), &stall_point, mp_obj_get_int(args[2]), wait);
    if (wait == PBIO_MOTOR_WAIT_COMPLETION) {
        return mp_obj_new_float(stall_point);
    }
    else{
        return mp_const_none;
    }
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_EncodedMotor_run_stalled_obj, 4, 4, motor_EncodedMotor_run_stalled);

/*
EncodedMotor
    def run_angle(self, speed, angle, after_stop=COAST, wait=True):
        """Rotate a motor by the given angle at a given speed.
        Arguments:
            speed {float} -- Absolute target speed (degrees per second). Run direction is automatically determined based on angle.
            target {float} -- Angle that the motor/mechanism should rotate by (degrees).
        Keyword Arguments (TODO):
            after_stop {const} -- What to do after the motor stops at the target: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})
        """
*/
STATIC mp_obj_t motor_EncodedMotor_run_angle(size_t n_args, const mp_obj_t *args){
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(args[0]);    
    pbio_motor_run_angle(self->port, mp_obj_get_float(args[1]), mp_obj_get_float(args[2]), mp_obj_get_int(args[3]), mp_obj_get_int(args[4]));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_EncodedMotor_run_angle_obj, 5, 5, motor_EncodedMotor_run_angle);

/*
EncodedMotor
    def run_target(self, speed, target, after_stop=COAST, wait=True):
        """Run a motor at a given speed and stop precisely at given target.
        Arguments:
            speed {float} -- Absolute target speed (degrees per second). Run direction (sign) is automatically determined based on target.
            target {float} -- Target for the motor/mechanism (degrees)
        Keyword Arguments (TODO):
            after_stop {const} -- What to do after the motor stops at the target: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})
        """

*/
STATIC mp_obj_t motor_EncodedMotor_run_target(size_t n_args, const mp_obj_t *args){
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(args[0]);    
    pbio_motor_run_target(self->port, mp_obj_get_float(args[1]), mp_obj_get_float(args[2]), mp_obj_get_int(args[3]), mp_obj_get_int(args[4]));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_EncodedMotor_run_target_obj, 5, 5, motor_EncodedMotor_run_target);

/*
EncodedMotor
    def track_target(self, target):
        """Position tracking for use in a control loop.
        Arguments:
            target {float} -- Target for the motor/mechanism (degrees)
        """
*/
STATIC mp_obj_t motor_EncodedMotor_track_target(mp_obj_t self_in, mp_obj_t target) {
    motor_EncodedMotor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_motor_track_target(self->port, mp_obj_get_float(target));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_EncodedMotor_track_target_obj, motor_EncodedMotor_track_target);


/*
EncodedMotor Class tables
*/

const mp_rom_map_elem_t motor_EncodedMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_device_id), MP_OBJ_NEW_SMALL_INT(PBIO_ID_UNKNOWN_ENCMOTOR) },
    { MP_ROM_QSTR(MP_QSTR_coast), MP_ROM_PTR(&motor_DCMotor_coast_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&motor_DCMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_DCMotor_duty_obj) },    
    { MP_ROM_QSTR(MP_QSTR_angle), MP_ROM_PTR(&motor_EncodedMotor_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_speed), MP_ROM_PTR(&motor_EncodedMotor_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_angle), MP_ROM_PTR(&motor_EncodedMotor_reset_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&motor_EncodedMotor_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&motor_EncodedMotor_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_time), MP_ROM_PTR(&motor_EncodedMotor_run_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_stalled), MP_ROM_PTR(&motor_EncodedMotor_run_stalled_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_angle), MP_ROM_PTR(&motor_EncodedMotor_run_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_target), MP_ROM_PTR(&motor_EncodedMotor_run_target_obj) },
    { MP_ROM_QSTR(MP_QSTR_track_target), MP_ROM_PTR(&motor_EncodedMotor_track_target_obj) },
    { MP_ROM_QSTR(MP_QSTR_settings), MP_ROM_PTR(&motor_EncodedMotor_settings_obj) },
};

MP_DEFINE_CONST_DICT(motor_EncodedMotor_locals_dict, motor_EncodedMotor_locals_dict_table);

const mp_obj_type_id_t motor_EncodedMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_EncodedMotor,
    .print = motor_EncodedMotor_print,
    .make_new = motor_EncodedMotor_make_new,
    .parent = &motor_DCMotor_type,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
    .device_id = PBIO_ID_UNKNOWN_ENCMOTOR,
};

const mp_obj_type_id_t motor_Internal_type = {
    { &mp_type_type },
    .name = MP_QSTR_Internal,
    .print = motor_DCMotor_print,
    .make_new = motor_DCMotor_make_new,
    .parent = &motor_DCMotor_type,
    .locals_dict = (mp_obj_dict_t*)&motor_DCMotor_locals_dict,
    .device_id = PBIO_ID_PUP_MOVEHUB_MOTOR,
};


/*
Motor module tables
*/

STATIC const mp_map_elem_t motor_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR__motor) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_DCMotor), (mp_obj_t)&motor_DCMotor_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_EncodedMotor), (mp_obj_t)&motor_EncodedMotor_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_Internal), (mp_obj_t)&motor_Internal_type},
};

STATIC MP_DEFINE_CONST_DICT (mp_module_motor_globals, motor_globals_table);

const mp_obj_module_t mp_module_motor = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_motor_globals,
};
