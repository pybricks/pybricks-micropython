#include "py/mphal.h"
#include "py/runtime.h"
#include "extmod/utime_mphal.h"

/*
class StopWatch():
    """Time the duration of a program or event."""
*/

// Class structure for StopWatch
typedef struct _timing_StopWatch_obj_t {
    mp_obj_base_t base;
    int32_t time_start;
    int32_t time_stop;
    int32_t time_spent_pausing;
    bool running;
} timing_StopWatch_obj_t;

/*
StopWatch
    def reset(self):
        """Set time to zero and pause."""
        self.time_start = get_time()
        self.time_stop = self.time_start
        self.time_spent_pausing = 0
        self.running = False
*/
STATIC mp_obj_t timing_StopWatch_reset(mp_obj_t self_in) {
    timing_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->time_start = mp_hal_ticks_ms();
    self->time_stop = self->time_start;
    self->time_spent_pausing = 0;
    self->running = false;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(timing_StopWatch_reset_obj, timing_StopWatch_reset);

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
STATIC mp_obj_t timing_StopWatch_time(mp_obj_t self_in) {
    timing_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(
        self->running ?
        mp_hal_ticks_ms()- self->time_start - self->time_spent_pausing :
        self->time_stop  - self->time_start - self->time_spent_pausing
    );
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(timing_StopWatch_time_obj, timing_StopWatch_time);

/*
StopWatch
    def pause(self):
        """Pause the stopwatch."""
        if self.running:
            self.running = False
            self.time_stop = get_time()
*/
STATIC mp_obj_t timing_StopWatch_pause(mp_obj_t self_in) {
    timing_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->running) {
        self->running = false;
        self->time_stop = mp_hal_ticks_ms();
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(timing_StopWatch_pause_obj, timing_StopWatch_pause);

/*
StopWatch
    def resume(self):
        """Resume the stopwatch."""
        if not self.running:
            self.running = True
            self.time_spent_pausing += get_time()-self.time_stop
*/
STATIC mp_obj_t timing_StopWatch_resume(mp_obj_t self_in) {
    timing_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->running) {
        self->running = true;
        self->time_spent_pausing += mp_hal_ticks_ms() - self->time_stop;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(timing_StopWatch_resume_obj, timing_StopWatch_resume);

/*
StopWatch
    def __init__(self):
        """Create the StopWatch object, reset it, and start it."""
        self.reset()
        self.resume()
*/
STATIC mp_obj_t timing_StopWatch_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // Initialize self
    timing_StopWatch_obj_t *self = m_new_obj(timing_StopWatch_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    timing_StopWatch_reset(self);
    timing_StopWatch_resume(self);
    return MP_OBJ_FROM_PTR(self);
}

/*
StopWatch
    def __str__(self):
        """String representation of StopWatch object."""
*/
STATIC void timing_StopWatch_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, qstr_str(MP_QSTR_StopWatch));
}


/*
StopWatch class tables
*/
STATIC const mp_rom_map_elem_t timing_StopWatch_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&timing_StopWatch_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&timing_StopWatch_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&timing_StopWatch_pause_obj) },
    { MP_ROM_QSTR(MP_QSTR_resume), MP_ROM_PTR(&timing_StopWatch_resume_obj) },
};
STATIC MP_DEFINE_CONST_DICT(timing_StopWatch_locals_dict, timing_StopWatch_locals_dict_table);

STATIC const mp_obj_type_t timing_StopWatch_type = {
    { &mp_type_type },
    .name = MP_QSTR_StopWatch,
    .print = timing_StopWatch_print,
    .make_new = timing_StopWatch_make_new,
    .locals_dict = (mp_obj_dict_t*)&timing_StopWatch_locals_dict,
};

/*
timing module tables
*/

STATIC const mp_map_elem_t timing_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),  MP_OBJ_NEW_QSTR(MP_QSTR_timing) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_wait),      (mp_obj_t)&mp_utime_sleep_ms_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_StopWatch), (mp_obj_t)&timing_StopWatch_type},
};

STATIC MP_DEFINE_CONST_DICT (
    pb_module_timing_globals,
    timing_globals_table
);

const mp_obj_module_t pb_module_timing = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_timing_globals,
};
