// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/builtin.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/objmodule.h"
#include "py/runtime.h"

#include <pbio/int_math.h>

#include <pybricks/parameters.h>
#include <pybricks/common.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_matrix.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>


// Global state of the run loop for async user programs. Gets set when run_task
// is called and cleared when it completes
STATIC bool run_loop_is_active;

bool pb_module_tools_run_loop_is_active() {
    return run_loop_is_active;
}

void pb_module_tools_assert_blocking(void) {
    if (run_loop_is_active) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("This can only be called before multitasking starts."));
    }
}

// The awaitables for the wait() function have no object associated with
// it (unlike e.g. a motor), so we make a starting point here. These never
// have to cancel each other so shouldn't need to be in a list, but this lets
// us share the same code with other awaitables. It also minimizes allocation.
MP_REGISTER_ROOT_POINTER(mp_obj_t wait_awaitables);

// Reset global state when user program starts.
void pb_module_tools_init(void) {
    MP_STATE_PORT(wait_awaitables) = mp_obj_new_list(0, NULL);
    run_loop_is_active = false;
}

STATIC bool pb_module_tools_wait_test_completion(mp_obj_t obj, uint32_t end_time) {
    return mp_hal_ticks_ms() - end_time < UINT32_MAX / 2;
}

STATIC mp_obj_t pb_module_tools_wait(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(time));

    mp_int_t time = pb_obj_get_int(time_in);

    // outside run loop, do blocking wait. This would be handled below as well,
    // but for this very simple call we'd rather avoid the overhead.
    if (!pb_module_tools_run_loop_is_active()) {
        if (time > 0) {
            mp_hal_delay_ms(time);
        }
        return mp_const_none;
    }

    // Require that duration is nonnegative small int. This makes it cheaper to
    // test completion state in iteration loop.
    time = pbio_int_math_bind(time, 0, INT32_MAX >> 2);

    return pb_type_awaitable_await_or_wait(
        NULL, // wait functions are not associated with an object
        MP_STATE_PORT(wait_awaitables),
        mp_hal_ticks_ms() + (time < 0 ? 0 : time),
        pb_module_tools_wait_test_completion,
        pb_type_awaitable_return_none,
        pb_type_awaitable_cancel_none,
        PB_TYPE_AWAITABLE_OPT_NONE);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pb_module_tools_wait_obj, 0, pb_module_tools_wait);

STATIC mp_obj_t pb_module_tools_run_task(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(task),
        PB_ARG_DEFAULT_INT(loop_time, 10));

    run_loop_is_active = true;

    uint32_t start_time = mp_hal_ticks_ms();
    uint32_t loop_time = pb_obj_get_positive_int(loop_time_in);

    mp_obj_iter_buf_t iter_buf;
    mp_obj_t iterable = mp_getiter(task_in, &iter_buf);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {

        while (mp_iternext(iterable) != MP_OBJ_STOP_ITERATION) {

            gc_collect();

            if (loop_time == 0) {
                continue;
            }

            uint32_t elapsed = mp_hal_ticks_ms() - start_time;
            if (elapsed < loop_time) {
                mp_hal_delay_ms(loop_time - elapsed);
            }
            start_time += loop_time;
        }

        nlr_pop();
        run_loop_is_active = false;
    } else {
        run_loop_is_active = false;
        nlr_jump(nlr.ret_val);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pb_module_tools_run_task_obj, 1, pb_module_tools_run_task);

STATIC const mp_rom_map_elem_t tools_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_tools)                    },
    { MP_ROM_QSTR(MP_QSTR_wait),        MP_ROM_PTR(&pb_module_tools_wait_obj)         },
    { MP_ROM_QSTR(MP_QSTR_run_task),    MP_ROM_PTR(&pb_module_tools_run_task_obj)     },
    { MP_ROM_QSTR(MP_QSTR_StopWatch),   MP_ROM_PTR(&pb_type_StopWatch)                },
    { MP_ROM_QSTR(MP_QSTR_multitask),   MP_ROM_PTR(&pb_type_Task)                     },
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
