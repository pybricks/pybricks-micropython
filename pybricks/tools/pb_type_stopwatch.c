// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/mphal.h"
#include "py/runtime.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

#include "tools.h"

typedef struct _tools_StopWatch_obj_t {
    mp_obj_base_t base;
    uint32_t time_start;
    uint32_t time_stop;
    uint32_t time_spent_pausing;
    bool running;
} tools_StopWatch_obj_t;

STATIC mp_obj_t tools_StopWatch_reset(mp_obj_t self_in) {
    tools_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->time_start = mp_hal_ticks_ms();
    self->time_stop = self->time_start;
    self->time_spent_pausing = 0;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(tools_StopWatch_reset_obj, tools_StopWatch_reset);

STATIC mp_obj_t tools_StopWatch_time(mp_obj_t self_in) {
    tools_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int_from_uint(
        self->running ?
        mp_hal_ticks_ms() - self->time_start - self->time_spent_pausing :
        self->time_stop - self->time_start - self->time_spent_pausing);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(tools_StopWatch_time_obj, tools_StopWatch_time);

STATIC mp_obj_t tools_StopWatch_pause(mp_obj_t self_in) {
    tools_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->running) {
        self->running = false;
        self->time_stop = mp_hal_ticks_ms();
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(tools_StopWatch_pause_obj, tools_StopWatch_pause);

STATIC mp_obj_t tools_StopWatch_resume(mp_obj_t self_in) {
    tools_StopWatch_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->running) {
        self->running = true;
        self->time_spent_pausing += mp_hal_ticks_ms() - self->time_stop;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(tools_StopWatch_resume_obj, tools_StopWatch_resume);

STATIC mp_obj_t tools_StopWatch_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    tools_StopWatch_obj_t *self = m_new_obj(tools_StopWatch_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    tools_StopWatch_reset(self);
    tools_StopWatch_resume(self);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t tools_StopWatch_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&tools_StopWatch_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&tools_StopWatch_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&tools_StopWatch_pause_obj) },
    { MP_ROM_QSTR(MP_QSTR_resume), MP_ROM_PTR(&tools_StopWatch_resume_obj) },
};
STATIC MP_DEFINE_CONST_DICT(tools_StopWatch_locals_dict, tools_StopWatch_locals_dict_table);

const mp_obj_type_t pb_type_StopWatch = {
    { &mp_type_type },
    .name = MP_QSTR_StopWatch,
    .make_new = tools_StopWatch_make_new,
    .locals_dict = (mp_obj_dict_t *)&tools_StopWatch_locals_dict,
};

#endif // PYBRICKS_PY_TOOLS
