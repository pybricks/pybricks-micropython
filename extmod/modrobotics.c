#include "modmotor.h"
#include "py/mphal.h"

/*
class Mechanism():
    """Class to control a motor with predefined defined target angles."""

    def __init__(self, motor, targets):
        """Initialize the mechanism.

        Arguments:
            motor {Motor} -- Motor object
            targets {dict} -- Dictionary of keys (e.g. strings, colors, or sensor values) with corresponding motor target values
            speed {int} -- Speed used by all movement commands for this mechanism
        """
*/

// Class structure for Mechanism
typedef struct _robotics_Mechanism_obj_t {
    mp_obj_base_t base;
    pbio_port_t port;
    mp_obj_dict_t *targets;
    int32_t speed;
} robotics_Mechanism_obj_t;

mp_obj_t robotics_Mechanism_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // Initialize self
    mp_arg_check_num(n_args, n_kw, 3, 3, false);
    robotics_Mechanism_obj_t *self = m_new_obj(robotics_Mechanism_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    /*
    TODO: below, store the motor object (pointer) instead of the port, equivalent to self.motor = motor
    Currently not (quite) supported by MicroPython unless we modify the attribute handler
    */
    self->port = get_port(args[0]);
    self->targets = MP_OBJ_TO_PTR(args[1]);
    self->speed = mp_obj_get_int(args[2]);
    return MP_OBJ_FROM_PTR(self);
}

/*
Mechanism
    def __str__(self):
        """String representation of Mechanism object."""
*/
void robotics_Mechanism_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    robotics_Mechanism_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Mechanism on PORT_%c",  self->port);
}

/*
Mechanism
    def targets(self):
        """Return the previously configured targets.
        Returns:
            dict -- Dictionary of (identifier, target) pairs
        """
*/
STATIC mp_obj_t robotics_Mechanism_targets(mp_obj_t self_in) {
    robotics_Mechanism_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_FROM_PTR(self->targets);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_Mechanism_targets_obj, robotics_Mechanism_targets);

/*
Mechanism class tables
*/

STATIC const mp_rom_map_elem_t robotics_Mechanism_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_targets), MP_ROM_PTR(&robotics_Mechanism_targets_obj) },
};
MP_DEFINE_CONST_DICT(robotics_Mechanism_locals_dict, robotics_Mechanism_locals_dict_table);

const mp_obj_type_t robotics_Mechanism_type = {
    { &mp_type_type },
    .name = MP_QSTR_Mechanism,
    .print = robotics_Mechanism_print,
    .make_new = robotics_Mechanism_make_new,
    .locals_dict = (mp_obj_dict_t*)&robotics_Mechanism_locals_dict,
};

/*
class StopWatch():
    """Time the duration of a program or event."""
*/

// Class structure for StopWatch
typedef struct _robotics_StopWatch_obj_t {
    mp_obj_base_t base;
    int32_t time_start;
    int32_t time_stop;
    int32_t time_spent_pausing;
    bool running;
} robotics_StopWatch_obj_t;

/*
StopWatch
    def reset(self):
        """Set time to zero and pause."""
        self.time_start = get_time()
        self.time_stop = self.time_start
        self.time_spent_pausing = 0
        self.running = False
*/
STATIC mp_obj_t robotics_StopWatch_reset(mp_obj_t self_in) {
    robotics_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->time_start = mp_hal_ticks_ms();
    self->time_stop = self->time_start;
    self->time_spent_pausing = 0;
    self->running = false;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_StopWatch_reset_obj, robotics_StopWatch_reset);

/*
StopWatch
    def time(self):
        """Get the current time of the stopwatch.

        Returns:
            int -- time in milliseconds

        """
        if self.running:
            return get_time()-self.time_start-self.time_spent_pausing
        else:
            return self.time_stop-self.time_start-self.time_spent_pausing
*/
STATIC mp_obj_t robotics_StopWatch_time(mp_obj_t self_in) {
    robotics_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(
        self->running ?
        mp_hal_ticks_ms()- self->time_start - self->time_spent_pausing :
        self->time_stop  - self->time_start - self->time_spent_pausing
    );
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_StopWatch_time_obj, robotics_StopWatch_time);

/*
StopWatch
    def pause(self):
        """Pause the stopwatch."""
        if self.running:
            self.running = False
            self.time_stop = get_time()
*/
STATIC mp_obj_t robotics_StopWatch_pause(mp_obj_t self_in) {
    robotics_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->running) {
        self->running = false;
        self->time_stop = mp_hal_ticks_ms();
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_StopWatch_pause_obj, robotics_StopWatch_pause);

/*
StopWatch
    def resume(self):
        """Resume the stopwatch."""
        if not self.running:
            self.running = True
            self.time_spent_pausing += get_time()-self.time_stop
*/
STATIC mp_obj_t robotics_StopWatch_resume(mp_obj_t self_in) {
    robotics_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->running) {
        self->running = true;
        self->time_spent_pausing += mp_hal_ticks_ms() - self->time_stop;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_StopWatch_resume_obj, robotics_StopWatch_resume);

/*
StopWatch
    def __init__(self):
        """Create the StopWatch object, reset it, and start it."""
        self.reset()
        self.resume()
*/
mp_obj_t robotics_StopWatch_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // Initialize self
    robotics_StopWatch_obj_t *self = m_new_obj(robotics_StopWatch_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    robotics_StopWatch_reset(self);
    robotics_StopWatch_resume(self);
    return MP_OBJ_FROM_PTR(self);
}

/*
StopWatch
    def __str__(self):
        """String representation of StopWatch object."""
*/
void robotics_StopWatch_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, qstr_str(MP_QSTR_StopWatch));
}


/*
StopWatch class tables
*/
STATIC const mp_rom_map_elem_t robotics_StopWatch_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&robotics_StopWatch_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&robotics_StopWatch_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&robotics_StopWatch_pause_obj) },
    { MP_ROM_QSTR(MP_QSTR_resume), MP_ROM_PTR(&robotics_StopWatch_resume_obj) },
};
MP_DEFINE_CONST_DICT(robotics_StopWatch_locals_dict, robotics_StopWatch_locals_dict_table);

const mp_obj_type_t robotics_StopWatch_type = {
    { &mp_type_type },
    .name = MP_QSTR_StopWatch,
    .print = robotics_StopWatch_print,
    .make_new = robotics_StopWatch_make_new,
    .locals_dict = (mp_obj_dict_t*)&robotics_StopWatch_locals_dict,
};

/*
robotics module tables
*/

STATIC const mp_map_elem_t robotics_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_robotics) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Mechanism), (mp_obj_t)&robotics_Mechanism_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_StopWatch), (mp_obj_t)&robotics_StopWatch_type},
};

STATIC MP_DEFINE_CONST_DICT (
    pb_module_robotics_globals,
    robotics_globals_table
);

const mp_obj_module_t pb_module_robotics = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_robotics_globals,
};
