#include <pbio/motorcontrol.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/builtin.h"

#include "modmotor.h"
#include "pberror.h"
#include "pbobj.h"


/* Motor stop enum */

STATIC const mp_rom_map_elem_t motor_Stop_enum_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_coast),      MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_STOP_COAST  ) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_brake),      MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_STOP_BRAKE  ) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_hold),       MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_STOP_HOLD   ) },
};
PB_DEFINE_CONST_ENUM(motor_Stop_enum, motor_Stop_enum_table);

/* Motor direction enum */

STATIC const mp_rom_map_elem_t motor_Dir_enum_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_normal),    MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_DIR_NORMAL    ) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_inverted),  MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_DIR_INVERTED  ) },
};
PB_DEFINE_CONST_ENUM(motor_Dir_enum, motor_Dir_enum_table);

/* Motor run type enum */

STATIC const mp_rom_map_elem_t motor_Run_enum_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_foreground),  MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_RUN_FOREGROUND) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_background),  MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_RUN_BACKGROUND) },
};
PB_DEFINE_CONST_ENUM(motor_Run_enum, motor_Run_enum_table);

/* Wait for maneuver to complete */

STATIC void wait_for_completion(pbio_port_t port, pbio_motor_run_t runtype) {
    if (runtype == PBIO_MOTOR_RUN_FOREGROUND) {
        while (motor_control_active[PORT_TO_IDX(port)] == PBIO_MOTOR_CONTROL_RUNNING) {
            mp_hal_delay_ms(10);
        }
    };
}

/*
Motor
    def __init__(self, port, direction):
        """Initialize the motor.
        Arguments:
            port {const} -- Port to which the device is connected: PORT_A, PORT_B, etc.
        Optional arguments (DC and Encoded motor)
            direction {const} -- DIR_NORMAL or DIR_INVERTED (default: {DIR_NORMAL})
        Optional arguments (Encoded motor only)
            train1 {list} -- List of the number of teeth of the gears in the first gear train (attached to the motor)
            train2 {list} -- List of the number of teeth of the gears in the second gear train
            ...
        """

Two examples of the gear train lists:

EXAMPLE 1: [12, 36]
  _____________
 |            |
 |    motor   |
 |____________|
       ||
       || 12t      36t
     ||||||  ||||||||||||||
                   ||
                   ||
               output axle


Example 2: [12, 20, 36], [20, 40], [20, 8, 40]
  _____________
 |            |
 |    motor   |
 |____________|
       ||
       || 12t    20t           36t
     ||||||  |||||||||||  ||||||||||||||
                                ||
                                ||
                             ||||||||  |||||||||||||||||
                                20t           || 40t
                                              ||
                                            ||||||||  |||  |||||||||||||||||
                                                20t    8t         || 40t
                                                                  ||
                                                              output axle

*/

mp_obj_t motor_Motor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    // Load self and determine port
    mp_arg_check_num(n_args, n_kw, 1, 4, false);
    motor_Motor_obj_t *self = m_new_obj(motor_Motor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    self->port = mp_obj_get_int(args[0]);
    // Configure direction or set to default
    int8_t direction = (n_args > 1) ? mp_obj_get_int(args[1]) : PBIO_MOTOR_DIR_NORMAL;
    // Determine expected device id from class name and check whether the motor has encoders
    pbio_id_t device_id;
    switch(type->name) {
        #if defined(PYBRICKS_BRICK_MOVEHUB)
        case MP_QSTR_TrainMotor:
            device_id = PBIO_ID_PUP_TRAIN_MOTOR;
            self->encoded = false;
            break;
        case MP_QSTR_MovehubMotor:
            device_id = PBIO_ID_PUP_MOVEHUB_MOTOR;
            self->encoded = true;
            break;
        case MP_QSTR_InteractiveMotor:
            device_id = PBIO_ID_PUP_INTERACTIVE_MOTOR;
            self->encoded = true;
            break;
        #endif //PYBRICKS_BRICK_MOVEHUB
        #if defined(PYBRICKS_BRICK_EV3)
        case MP_QSTR_MediumMotor:
            device_id = PBIO_ID_EV3_LARGE_MOTOR;
            self->encoded = true;
            break;
        case MP_QSTR_LargeMotor:
            device_id = PBIO_ID_EV3_LARGE_MOTOR;
            self->encoded = true;
            break;
        #endif //PYBRICKS_BRICK_EV3
        default:
            device_id = PBIO_ID_NO_DEVICE;
            self->encoded = false;
    }
    // Initialization specific to encoded motors
    if(self->encoded) {
        // Compute overall gear ratio from each gear train
        float_t gear_ratio = 1.0;
        int8_t n_trains = n_args - 2;
        for (int8_t train = 0; train < n_trains; train++) {
            // For this gear train, unpack the list of gears
            mp_obj_t *gears;
            size_t n_gears;
            mp_obj_get_array(args[train+2], &n_gears, &gears);
            // For this gear train, compute the ratio from the first and last gear
            int16_t first_gear = mp_obj_get_int(gears[0]);
            int16_t last_gear = mp_obj_get_int(gears[n_gears-1]);
            if (first_gear < 1 || last_gear < 1) {
                pb_assert(PBIO_ERROR_INVALID_ARG);
            }
            // Include the ratio of this train in the overall gear train
            gear_ratio = (gear_ratio*last_gear)/first_gear;
        }
        // Configure the encoded motor with the selected arguments at pbio level
        pb_assert(pbio_encmotor_setup(self->port, device_id, direction, gear_ratio));
    }
    else {
        // Configure the dc motor with the selected arguments at pbio level
        pb_assert(pbio_dcmotor_setup(self->port, device_id, direction));
    }
    return MP_OBJ_FROM_PTR(self);
}

/*
Motor
    def __str__(self):
        """String representation of DCMotor object."""
*/
void motor_Motor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind){
    motor_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_port_t port = get_port(self_in);
    char dcmotor_settings_string[MAX_DCMOTOR_SETTINGS_STR_LENGTH];
    pbio_dcmotor_print_settings(port, dcmotor_settings_string);
    if (self->encoded) {
        char encmotor_settings_string[MAX_ENCMOTOR_SETTINGS_STR_LENGTH];
        pbio_encmotor_print_settings(port, encmotor_settings_string);
        mp_printf(print, "%s\n%s", dcmotor_settings_string, encmotor_settings_string);
    }
    else {
        mp_printf(print, "%s", dcmotor_settings_string);
    }
}

/*
Motor
    def settings(self, relative_torque_limit, stall_speed_limit, stall_time, min_speed, max_speed, tolerance, acceleration_start, acceleration_end, tight_loop_time, pid_kp, pid_ki, pid_kd):
        """Update the motor settings.
        Keyword Arguments (TODO):
        relative_torque_limit {int}   -- Percentage (-100.0 to 100.0) of the maximum stationary torque that the motor is allowed to produce.
        stall_speed_limit {int}       -- If this speed cannnot be reached even with the maximum torque, the motor is considered to be stalled
        stall_time {int}              -- Minimum stall time before the run_stalled action completes
        min_speed {int}               -- If speed is equal or less than this, consider the motor to be standing still
        max_speed {int}               -- Soft limit on the reference speed in all run commands
        tolerance {int}               -- Allowed deviation (deg) from target before motion is considered complete
        acceleration_start {int}      -- Acceleration when beginning to move. Positive value in degrees per second per second
        acceleration_end {int}        -- Deceleration when stopping. Positive value in degrees per second per second
        tight_loop_time {int}         -- When a run function is called twice in this interval (seconds), assume that the user is doing their own speed control.
        pid_kp {int}                  -- Proportional angle control constant (and integral speed control constant)
        pid_ki {int}                  -- Integral angle control constant
        pid_kd {int}                  -- Derivative angle control constant (and proportional speed control constant)
*/
STATIC mp_obj_t motor_Motor_settings(size_t n_args, const mp_obj_t *args){
    pbio_port_t port = get_port(args[0]);

    // TODO: Keyword/optional arguments

    // TODO: If device is a DC motor, return an error if the user tries to set Motor settings

    pb_assert(pbio_encmotor_set_settings(port,
                                         mp_obj_get_num(args[1]),
                                         mp_obj_get_num(args[2]),
                                         mp_obj_get_num(args[3]),
                                         mp_obj_get_num(args[4]),
                                         mp_obj_get_num(args[5]),
                                         mp_obj_get_num(args[6]),
                                         mp_obj_get_num(args[7]),
                                         mp_obj_get_num(args[8]),
                                         mp_obj_get_num(args[9]),
                                         mp_obj_get_num(args[10]),
                                         mp_obj_get_num(args[11]),
                                         mp_obj_get_num(args[12])));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_settings_obj, 13, 13, motor_Motor_settings);



/*
Motor
    def duty(self, duty):
        """Set motor duty cycle.
        Arguments:
            duty {int} -- Percentage from -100 to 100
*/
STATIC mp_obj_t motor_Motor_duty(mp_obj_t self_in, mp_obj_t duty_cycle) {
    motor_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_dcmotor_set_duty_cycle(self->port, mp_obj_get_num(duty_cycle)));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_Motor_duty_obj, motor_Motor_duty);

/*
Motor
    def angle(self):
        """Return the angle of the motor/mechanism (degrees).
        Returns:
            int -- Position of the motor/mechanism (degrees).
        """
*/
STATIC mp_obj_t motor_Motor_angle(mp_obj_t self_in) {
    pbio_port_t port = get_port(self_in);
    int32_t angle;
    pb_assert(pbio_encmotor_get_angle(port, &angle));
    return mp_obj_new_int(angle);
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_Motor_angle_obj, motor_Motor_angle);

/*
Motor
    def reset_angle(self, reset_angle):
        """Reset the angle of the motor/mechanism (degrees).
        Arguments:
            reset_angle {int} -- Value to which the rotation sensor angle should be reset (default: {0})
*/
STATIC mp_obj_t motor_Motor_reset_angle(size_t n_args, const mp_obj_t *args){
    pbio_port_t port = get_port(args[0]);
    int32_t reset_angle = n_args > 1 ? mp_obj_get_num(args[1]) : 0;
    pb_assert(pbio_encmotor_reset_angle(port, reset_angle));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_reset_angle_obj, 1, 2, motor_Motor_reset_angle);

/*
Motor
    def speed(self):
        """Return the angular speed of the motor/mechanism (degrees per second).
        Returns:
            int -- Angular speed of the motor/mechanism (degrees per second).
        """
*/
STATIC mp_obj_t motor_Motor_speed(mp_obj_t self_in) {
    pbio_port_t port = get_port(self_in);
    int32_t speed;
    pb_assert(pbio_encmotor_get_angular_rate(port, &speed));
    return mp_obj_new_int(speed);
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_Motor_speed_obj, motor_Motor_speed);

/*
Motor
    def run(self, speed, wait=False):
        """Start and keep running the motor/mechanism at the given speed (degrees per second).
        Arguments:
            speed {int} -- Target speed (degrees per second)
        """
*/
STATIC mp_obj_t motor_Motor_run(size_t n_args, const mp_obj_t *args){
    pbio_port_t port = get_port(args[0]);
    pbio_motor_run_t runtype = n_args > 2 ? mp_obj_get_int(args[2]) : PBIO_MOTOR_RUN_BACKGROUND;
    pb_assert(pbio_encmotor_run(port, mp_obj_get_num(args[1])));
    wait_for_completion(port, runtype);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_run_obj, 2, 3, motor_Motor_run);

/*
Motor
    def stop(self, after_stop=COAST):
        """Stop a motor/mechanism.
        Optional arguments:
            after_stop {const} -- What to do after stopping the motor command: BRAKE, COAST, or HOLD. (default: {COAST})
        """
*/

STATIC mp_obj_t motor_Motor_stop(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    motor_Motor_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    pbio_motor_after_stop_t after_stop = n_args > 1 ? mp_obj_get_int(args[1]) : PBIO_MOTOR_STOP_COAST;
    if (!self->encoded && after_stop == PBIO_MOTOR_STOP_HOLD){
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    // Call pbio with parsed user/default arguments
    pb_assert(pbio_encmotor_stop(self->port, after_stop));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_stop_obj, 1, 2, motor_Motor_stop);

/*
Motor
    def run_time(self, speed, duration, after_stop=COAST, wait=True):
        """Run a motor/mechanism at the given speed for a given duration. Then stop.
        Arguments:
            speed {int} -- Target speed (degrees per second)
            duration {int} -- Total duration, including start and stop (seconds)
        Optional arguments:
            after_stop {const} -- What to do after the motor stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})
        """
*/
STATIC mp_obj_t motor_Motor_run_time(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    pbio_port_t port = get_port(args[0]);
    pbio_motor_after_stop_t after_stop = n_args > 3 ? mp_obj_get_int(args[3]) : PBIO_MOTOR_STOP_COAST;
    pbio_motor_run_t runtype           = n_args > 4 ? mp_obj_get_int(args[4]) : PBIO_MOTOR_RUN_FOREGROUND;
    // Call pbio with parsed user/default arguments
    pb_assert(pbio_encmotor_run_time(port, mp_obj_get_num(args[1]), mp_obj_get_num(args[2]), after_stop));
    wait_for_completion(port, runtype);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_run_time_obj, 3, 5, motor_Motor_run_time);

/*
Motor
    def run_stalled(self, speed, after_stop=COAST, wait=True):
        """Run a motor/mechanism at the given speed until it stalls. Then stop.
        Arguments:
            speed {int} -- Target speed (degrees per second)
        Optional arguments:
            after_stop {const} -- What to do after the motor stops: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})
        Returns:
            int -- If wait is True, then return the angle (degrees) at the time of stalling, otherwise return None
        """
*/
STATIC mp_obj_t motor_Motor_run_stalled(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    pbio_port_t port = get_port(args[0]);
    pbio_motor_after_stop_t after_stop = n_args > 2 ? mp_obj_get_int(args[2]) : PBIO_MOTOR_STOP_COAST;
    pbio_motor_run_t runtype             = n_args > 3 ? mp_obj_get_int(args[3]) : PBIO_MOTOR_RUN_FOREGROUND;
    // Call pbio with parsed user/default arguments
    pb_assert(pbio_encmotor_run_stalled(port, mp_obj_get_num(args[1]), after_stop));
    wait_for_completion(port, runtype);
    // If the user specified to wait for the motion to complete, return the angle at which the motor stalled
    if (runtype == PBIO_MOTOR_RUN_FOREGROUND) {
        int32_t stall_point;
        pbio_encmotor_get_angle(port, &stall_point);
        return mp_obj_new_int(stall_point);
    }
    // If this command is set to run in the background, return None
    else{
        return mp_const_none;
    }
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_run_stalled_obj, 2, 4, motor_Motor_run_stalled);

/*
Motor
    def run_angle(self, speed, angle, after_stop=COAST, wait=True):
        """Rotate a motor by the given angle at a given speed.
        Arguments:
            speed {int} -- Absolute target speed (degrees per second). Run direction is automatically determined based on angle.
            target {int} -- Angle that the motor/mechanism should rotate by (degrees).
        Optional arguments:
            after_stop {const} -- What to do after the motor stops at the target: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})
        """
*/
STATIC mp_obj_t motor_Motor_run_angle(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    pbio_port_t port = get_port(args[0]);
    pbio_motor_after_stop_t after_stop = n_args > 3 ? mp_obj_get_int(args[3]) : PBIO_MOTOR_STOP_COAST;
    pbio_motor_run_t runtype           = n_args > 4 ? mp_obj_get_int(args[4]) : PBIO_MOTOR_RUN_FOREGROUND;
    // Call pbio with parsed user/default arguments
    pb_assert(pbio_encmotor_run_angle(port, mp_obj_get_num(args[1]), mp_obj_get_num(args[2]), after_stop));
    wait_for_completion(port, runtype);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_run_angle_obj, 3, 5, motor_Motor_run_angle);

/*
Motor
    def run_target(self, speed, target, after_stop=COAST, wait=True):
        """Run a motor at a given speed and stop precisely at given target.
        Arguments:
            speed {int} -- Absolute target speed (degrees per second). Run direction (sign) is automatically determined based on target.
            target {int} -- Target for the motor/mechanism (degrees)
        Optional arguments:
            after_stop {const} -- What to do after the motor stops at the target: BRAKE, COAST, or HOLD. (default: {COAST})
            wait {bool} -- Wait for motion to be complete (True) or run task in the background (False). (default: {True})
        """

*/
STATIC mp_obj_t motor_Motor_run_target(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    pbio_port_t port = get_port(args[0]);
    pbio_motor_after_stop_t after_stop = n_args > 3 ? mp_obj_get_int(args[3]) : PBIO_MOTOR_STOP_COAST;
    pbio_motor_run_t runtype           = n_args > 4 ? mp_obj_get_int(args[4]) : PBIO_MOTOR_RUN_FOREGROUND;
    // Call pbio with parsed user/default arguments
    pb_assert(pbio_encmotor_run_target(port, mp_obj_get_num(args[1]), mp_obj_get_num(args[2]), after_stop));
    wait_for_completion(port, runtype);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_run_target_obj, 3, 5, motor_Motor_run_target);

/*
Motor
    def track_target(self, target):
        """Position tracking for use in a control loop.
        Arguments:
            target {int} -- Target for the motor/mechanism (degrees)
        """
*/
STATIC mp_obj_t motor_Motor_track_target(mp_obj_t self_in, mp_obj_t target) {
    pbio_port_t port = get_port(self_in);
    pb_assert(pbio_encmotor_track_target(port, mp_obj_get_num(target)));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_Motor_track_target_obj, motor_Motor_track_target);


/*
Motor Class tables
*/

STATIC const mp_rom_map_elem_t motor_EncodedMotor_locals_dict_table[] = {
    //
    // Methods common to DCMotor and EncodedMotor
    //
    { MP_ROM_QSTR(MP_QSTR_settings), MP_ROM_PTR(&motor_Motor_settings_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&motor_Motor_stop_obj) },    
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&motor_Motor_duty_obj) },
    //
    // Methods specific to EncodedMotor
    //
    { MP_ROM_QSTR(MP_QSTR_angle), MP_ROM_PTR(&motor_Motor_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_speed), MP_ROM_PTR(&motor_Motor_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_angle), MP_ROM_PTR(&motor_Motor_reset_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&motor_Motor_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_time), MP_ROM_PTR(&motor_Motor_run_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_stalled), MP_ROM_PTR(&motor_Motor_run_stalled_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_angle), MP_ROM_PTR(&motor_Motor_run_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_target), MP_ROM_PTR(&motor_Motor_run_target_obj) },
    { MP_ROM_QSTR(MP_QSTR_track_target), MP_ROM_PTR(&motor_Motor_track_target_obj) },
};

MP_DEFINE_CONST_DICT(motor_EncodedMotor_locals_dict, motor_EncodedMotor_locals_dict_table);

/*
DCMotor Class tables
*/

// Instead of using the MP_DEFINE_CONST_DICT macro directly, we modify it
// to use a shortened version of the EncodedMotor locals dict table
const mp_obj_dict_t motor_DCMotor_locals_dict = {
    .base = {&mp_type_dict},
    .map = {
        .all_keys_are_qstrs = 1,
        .is_fixed = 1,
        .is_ordered = 1,
        // Use the first 4 items from the EncodedMotor table
        .used = 4,
        .alloc = 4,
        .table = (mp_map_elem_t*)(mp_rom_map_elem_t*)motor_EncodedMotor_locals_dict_table,
    },
};
