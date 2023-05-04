// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/builtin.h"
#include "py/mphal.h"
#include "py/objmodule.h"
#include "py/runtime.h"

#include <pybricks/parameters.h>
#include <pybricks/common.h>
#include <pybricks/tools.h>

// Implementation of wait that always blocks. Needed for system runloop code
// to briefly wait inside runloop.
STATIC mp_obj_t pb_module_task_run(mp_obj_t time_in) {
    mp_int_t time = pb_obj_get_int(time_in);
    if (time > 0) {
        mp_hal_delay_ms(time);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_module_task_run_obj, pb_module_task_run);

STATIC bool _pb_module_task_run_loop_is_active;

bool pb_module_task_run_loop_is_active() {
    return _pb_module_task_run_loop_is_active;
}

void pb_module_task_init(void) {
    _pb_module_task_run_loop_is_active = false;
}

STATIC const mp_rom_map_elem_t task_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_task)      },
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&pb_module_task_run_obj) },

};
STATIC MP_DEFINE_CONST_DICT(pb_module_task_globals, task_globals_table);

const mp_obj_module_t pb_module_task = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_task_globals,
};

#endif // PYBRICKS_PY_TOOLS
