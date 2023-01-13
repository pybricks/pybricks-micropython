// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/parameters.h>
#include <pybricks/common.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_matrix.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

/**
 * A generator-like type for waiting on a motor operation to complete.
 */
typedef struct _pb_type_tools_wait_obj_t pb_type_tools_wait_obj_t;

struct _pb_type_tools_wait_obj_t {
    mp_obj_base_t base;
    /**
     * When to stop waiting.
     */
    uint32_t end_time;
    /**
     * Whether this generator object is done and thus can be recycled.
     */
    bool has_ended;
    /**
     * Linked list of awaitables.
     */
    pb_type_tools_wait_obj_t *next_awaitable;
};

STATIC mp_obj_t pb_type_tools_wait_iternext(mp_obj_t self_in) {
    pb_type_tools_wait_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Stop on reaching target time or if externally cancelled.
    if (mp_hal_ticks_ms() - self->end_time < (uint32_t)INT32_MAX || self->has_ended) {
        self->has_ended = true;
        return MP_OBJ_STOP_ITERATION;
    }
    // Not done, so keep going.
    return mp_const_none;
}

// close() cancels the awaitable.
STATIC mp_obj_t pb_type_tools_wait_close(mp_obj_t self_in) {
    pb_type_tools_wait_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->has_ended = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_tools_wait_close_obj, pb_type_tools_wait_close);

STATIC const mp_rom_map_elem_t pb_type_tools_wait_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pb_type_tools_wait_close_obj) },
};
MP_DEFINE_CONST_DICT(pb_type_tools_wait_locals_dict, pb_type_tools_wait_locals_dict_table);

// This is a partial implementation of the Python generator type. It is missing
// send(value) and throw(type[, value[, traceback]])
MP_DEFINE_CONST_OBJ_TYPE(pb_type_tools_wait,
    MP_QSTR_wait,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    iter, pb_type_tools_wait_iternext,
    locals_dict, &pb_type_tools_wait_locals_dict);

// Statically allocated awaitable from which all others can be found.
STATIC pb_type_tools_wait_obj_t first_awaitable;

// Reset first awaitable on initializing MicroPython.
STATIC void pb_type_tools_wait_reset(void) {
    first_awaitable.base.type = &pb_type_tools_wait;
    first_awaitable.has_ended = true;
    first_awaitable.next_awaitable = MP_OBJ_NULL;
}

STATIC mp_obj_t pb_type_tools_wait_new(mp_int_t duration) {

    // When to stop waiting.
    uint32_t end_time = mp_hal_ticks_ms() + (uint32_t)duration;

    // Find next available awaitable.
    pb_type_tools_wait_obj_t *awaitable = &first_awaitable;
    while (!awaitable->has_ended && awaitable->next_awaitable != MP_OBJ_NULL) {
        awaitable = awaitable->next_awaitable;
    }
    // If the last known awaitable is still in use, allocate another.
    if (!awaitable->has_ended) {
        // Attach to the previous one.
        awaitable->next_awaitable = m_new_obj(pb_type_tools_wait_obj_t);

        // Initialize the new awaitable.
        awaitable = awaitable->next_awaitable;
        awaitable->next_awaitable = MP_OBJ_NULL;
        awaitable->base.type = &pb_type_tools_wait;
    }

    // Initialize awaitable with the end time.
    awaitable->has_ended = duration < 0 ? true: false;
    awaitable->end_time = end_time;

    // Return the awaitable where the user can await it.
    return MP_OBJ_FROM_PTR(awaitable);
}

STATIC mp_obj_t tools_wait(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(time));

    mp_int_t time = pb_obj_get_int(time_in);

    // In async mode, return awaitable.
    if (pb_module_tools_run_loop_is_active()) {
        return pb_type_tools_wait_new(time);
    }

    // In blocking mode, wait until done.
    if (time > 0) {
        mp_hal_delay_ms(time);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_wait_obj, 0, tools_wait);

STATIC bool _pb_module_tools_run_loop_is_active;

bool pb_module_tools_run_loop_is_active() {
    return _pb_module_tools_run_loop_is_active;
}

STATIC mp_obj_t pb_module_tools___init__(void) {
    _pb_module_tools_run_loop_is_active = false;
    pb_type_tools_wait_reset();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(pb_module_tools___init___obj, pb_module_tools___init__);

STATIC mp_obj_t pb_module_tools_set_run_loop_active(mp_obj_t self_in) {
    _pb_module_tools_run_loop_is_active = mp_obj_is_true(self_in);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_module_tools_set_run_loop_active_obj, pb_module_tools_set_run_loop_active);

STATIC const mp_rom_map_elem_t tools_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_tools)      },
    { MP_ROM_QSTR(MP_QSTR___init__),    MP_ROM_PTR(&pb_module_tools___init___obj)},
    { MP_ROM_QSTR(MP_QSTR__set_run_loop_active), MP_ROM_PTR(&pb_module_tools_set_run_loop_active_obj)},
    { MP_ROM_QSTR(MP_QSTR_wait),        MP_ROM_PTR(&tools_wait_obj)     },
    { MP_ROM_QSTR(MP_QSTR_StopWatch),   MP_ROM_PTR(&pb_type_StopWatch)  },
    #if MICROPY_PY_BUILTINS_FLOAT
    { MP_ROM_QSTR(MP_QSTR_Matrix),      MP_ROM_PTR(&pb_type_Matrix)           },
    { MP_ROM_QSTR(MP_QSTR_vector),      MP_ROM_PTR(&pb_geometry_vector_obj)   },
    { MP_ROM_QSTR(MP_QSTR_cross),       MP_ROM_PTR(&pb_type_matrix_cross_obj) },
    // backwards compatibility for pybricks.geometry.Axis
    { MP_ROM_QSTR(MP_QSTR_Axis),        MP_ROM_PTR(&pb_enum_type_Axis) },
    #endif // MICROPY_PY_BUILTINS_FLOAT
};
STATIC MP_DEFINE_CONST_DICT(pb_module_tools_globals, tools_globals_table);

const mp_obj_module_t pb_module_tools = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_tools_globals,
};

#if PYBRICKS_RUNS_ON_EV3DEV
// ev3dev extends the C module in Python
MP_REGISTER_MODULE(MP_QSTR__tools, pb_module_tools);
#else
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_tools, pb_module_tools);
#endif

// backwards compatibility for pybricks.geometry
#if MICROPY_PY_BUILTINS_FLOAT
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_geometry, pb_module_tools);
#endif // MICROPY_PY_BUILTINS_FLOAT

#endif // PYBRICKS_PY_TOOLS
