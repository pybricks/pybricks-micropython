// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/builtin.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/objmodule.h"
#include "py/runtime.h"

#include <pybricks/parameters.h>
#include <pybricks/common.h>
#include <pybricks/tools.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

STATIC bool _pb_module_task_run_loop_is_active;

bool pb_module_task_run_loop_is_active() {
    return _pb_module_task_run_loop_is_active;
}

void pb_module_task_init(void) {
    _pb_module_task_run_loop_is_active = false;
    pb_type_tools_awaitable_init();
}

STATIC mp_obj_t pb_module_task_run(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(task),
        PB_ARG_DEFAULT_INT(loop_time, 10));

    _pb_module_task_run_loop_is_active = true;

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
        _pb_module_task_run_loop_is_active = false;
    } else {
        _pb_module_task_run_loop_is_active = false;
        nlr_jump(nlr.ret_val);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pb_module_task_run_obj, 1, pb_module_task_run);

/**
 * State of one coroutine task handled by all/race.
 */
typedef struct {
    mp_obj_t arg;
    mp_obj_t return_val;
    mp_obj_iter_buf_t iter_buf;
    mp_obj_t iterable;
    bool done;
} pb_module_task_progress_t;

typedef struct {
    mp_obj_base_t base;
    /**
     * The number of tasks managed by this all/race awaitable.
     */
    size_t num_tasks;
    /**
     * The number of tasks that must finish before the collection is done.
     */
    size_t num_tasks_required;
    /**
     * The tasks managed by this all or race awaitable.
     */
    pb_module_task_progress_t *tasks;
} pb_module_task_collection_obj_t;

// Cancel all tasks by calling their close methods.
STATIC mp_obj_t pb_module_task_collection_close(mp_obj_t self_in) {
    pb_module_task_collection_obj_t *self = MP_OBJ_TO_PTR(self_in);
    for (size_t i = 0; i < self->num_tasks; i++) {
        pb_module_task_progress_t *task = &self->tasks[i];

        // Task already complete, no need to cancel.
        if (task->done) {
            continue;
        }

        // Find close() on coroutine object, then call it.
        mp_obj_t dest[2];
        mp_load_method_maybe(task->arg, MP_QSTR_close, dest);
        if (dest[0] != MP_OBJ_NULL) {
            mp_call_method_n_kw(0, 0, dest);
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_module_task_collection_close_obj, pb_module_task_collection_close);

STATIC mp_obj_t pb_module_task_collection_iternext(mp_obj_t self_in) {
    pb_module_task_collection_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Do one iteration of each task.
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {

        size_t done_total = 0;

        for (size_t i = 0; i < self->num_tasks; i++) {

            pb_module_task_progress_t *task = &self->tasks[i];

            // This task already complete, skip.
            if (task->done) {
                done_total++;
                continue;
            }

            // Do one task iteration.
            mp_obj_t result = mp_iternext(task->iterable);

            // Not done yet, try next time.
            if (result == mp_const_none) {
                continue;
            }

            // Task is done, save return value.
            if (result == MP_OBJ_STOP_ITERATION) {
                if (MP_STATE_THREAD(stop_iteration_arg) != MP_OBJ_NULL) {
                    task->return_val = MP_STATE_THREAD(stop_iteration_arg);
                }
                task->done = true;
                done_total++;

                // If enough tasks are done, don't finish this round. This way,
                // in race(), there is only one winner.
                if (done_total >= self->num_tasks_required) {
                    // Cancel everything else.
                    pb_module_task_collection_close(self_in);
                    break;
                }
            }
        }
        // Successfully did one iteration of all tasks.
        nlr_pop();

        // If collection not done yet, indicate that it should run again.
        if (done_total < self->num_tasks_required) {
            return mp_const_none;
        }

        // Otherwise raise StopIteration with return values.
        mp_obj_t *ret = m_new(mp_obj_t, self->num_tasks);
        for (size_t i = 0; i < self->num_tasks; i++) {
            ret[i] = self->tasks[i].return_val;
        }
        return mp_make_stop_iteration(mp_obj_new_list(self->num_tasks, ret));
    } else {
        // On failure of one task, cancel others, then stop iterating collection.
        pb_module_task_collection_close(self_in);
        nlr_jump(nlr.ret_val);
    }
}

STATIC const mp_rom_map_elem_t pb_module_task_collection_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pb_module_task_collection_close_obj) },
};
MP_DEFINE_CONST_DICT(pb_module_task_collection_locals_dict, pb_module_task_collection_locals_dict_table);

extern const mp_obj_type_t pb_module_task_all;

STATIC mp_obj_t pb_module_task_collection_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    pb_module_task_collection_obj_t *self = mp_obj_malloc(pb_module_task_collection_obj_t, type);
    self->num_tasks = n_args;
    self->num_tasks_required = type == &pb_module_task_all ? n_args : 1;
    self->tasks = m_new(pb_module_task_progress_t, n_args);
    for (size_t i = 0; i < n_args; i++) {
        pb_module_task_progress_t *task = &self->tasks[i];
        task->arg = args[i];
        task->return_val = mp_const_none;
        task->iterable = mp_getiter(args[i], &task->iter_buf);
        task->done = false;
    }
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_OBJ_TYPE(pb_module_task_all,
    MP_QSTR_all,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    iter, pb_module_task_collection_iternext,
    make_new, pb_module_task_collection_new,
    locals_dict, &pb_module_task_collection_locals_dict);

MP_DEFINE_CONST_OBJ_TYPE(pb_module_task_race,
    MP_QSTR_race,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    iter, pb_module_task_collection_iternext,
    make_new, pb_module_task_collection_new,
    locals_dict, &pb_module_task_collection_locals_dict);

STATIC const mp_rom_map_elem_t task_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),  MP_ROM_QSTR(MP_QSTR_task)           },
    { MP_ROM_QSTR(MP_QSTR_run),       MP_ROM_PTR(&pb_module_task_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_all),       MP_ROM_PTR(&pb_module_task_all)     },
    { MP_ROM_QSTR(MP_QSTR_race),      MP_ROM_PTR(&pb_module_task_race)    },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_task_globals, task_globals_table);

const mp_obj_module_t pb_module_task = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_task_globals,
};

#endif // PYBRICKS_PY_TOOLS
