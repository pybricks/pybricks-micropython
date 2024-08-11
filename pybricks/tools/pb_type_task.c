// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/builtin.h"
#include "py/objmodule.h"
#include "py/runtime.h"

#include <pybricks/parameters.h>
#include <pybricks/common.h>
#include <pybricks/tools.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

/**
 * State of one coroutine task handled by Task.
 */
typedef struct {
    mp_obj_t arg;
    mp_obj_t return_val;
    mp_obj_iter_buf_t iter_buf;
    mp_obj_t iterable;
    bool done;
} pb_type_Task_progress_t;

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
    pb_type_Task_progress_t *tasks;
} pb_type_Task_obj_t;

// Cancel all tasks by calling their close methods.
static mp_obj_t pb_type_Task_close(mp_obj_t self_in) {
    pb_type_Task_obj_t *self = MP_OBJ_TO_PTR(self_in);
    for (size_t i = 0; i < self->num_tasks; i++) {
        pb_type_Task_progress_t *task = &self->tasks[i];

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
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Task_close_obj, pb_type_Task_close);

static mp_obj_t pb_type_Task_iternext(mp_obj_t self_in) {
    pb_type_Task_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Do one iteration of each task.
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {

        size_t done_total = 0;

        for (size_t i = 0; i < self->num_tasks; i++) {

            pb_type_Task_progress_t *task = &self->tasks[i];

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
                    pb_type_Task_close(self_in);
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
        return mp_make_stop_iteration(mp_obj_new_tuple(self->num_tasks, ret));
    } else {
        // On failure of one task, cancel others, then stop iterating collection by re-raising.
        pb_type_Task_close(self_in);
        nlr_jump(nlr.ret_val);
    }
}

static const mp_rom_map_elem_t pb_type_Task_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pb_type_Task_close_obj) },
};
MP_DEFINE_CONST_DICT(pb_type_Task_locals_dict, pb_type_Task_locals_dict_table);

static mp_obj_t pb_type_Task_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    // Whether to race until one task is done (True) or wait for all tasks (False).
    bool race = n_kw == 1 && mp_obj_is_true(args[n_args - 1]);

    pb_type_Task_obj_t *self = mp_obj_malloc(pb_type_Task_obj_t, type);
    self->num_tasks = n_args;
    self->num_tasks_required = race ? 1 : n_args;
    self->tasks = m_new(pb_type_Task_progress_t, n_args);
    for (size_t i = 0; i < n_args; i++) {
        pb_type_Task_progress_t *task = &self->tasks[i];
        task->arg = args[i];
        task->return_val = mp_const_none;
        task->iterable = mp_getiter(args[i], &task->iter_buf);
        task->done = false;
    }
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_OBJ_TYPE(pb_type_Task,
    MP_QSTR_Task,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    iter, pb_type_Task_iternext,
    make_new, pb_type_Task_new,
    locals_dict, &pb_type_Task_locals_dict);

#endif // PYBRICKS_PY_TOOLS
