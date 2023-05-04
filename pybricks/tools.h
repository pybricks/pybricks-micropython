// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_TOOLS_H
#define PYBRICKS_INCLUDED_PYBRICKS_TOOLS_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/obj.h"

extern const mp_obj_module_t pb_module_task;
void pb_module_task_init(void);

/**
 * Tests if awaitable operation is complete. Returns MP_OBJ_STOP_ITERATION if
 * done, possibly with return argument, else mp_const_none.
 *
 * On completion, this function is expected to close/stop hardware
 * operations as needed (hold a motor, etc.). This is not the same as cancel
 * below, which always stops the relevant hardware (i.e. always coast).
 */
typedef mp_obj_t (*pb_awaitable_test_completion_t)(mp_obj_t obj);

/**
 * Called on cancel/close. Used to stop hardware operation in unhandled
 * conditions.
 */
typedef void (*pb_awaitable_cancel_t)(mp_obj_t obj);

bool pb_module_task_run_loop_is_active();

void pb_type_tools_awaitable_init(void);

mp_obj_t pb_type_tools_await_or_wait(mp_obj_t obj, pb_awaitable_test_completion_t test_completion, pb_awaitable_cancel_t cancel);

mp_obj_t pb_type_tools_await_time(mp_obj_t duration_in);

extern const mp_obj_type_t pb_type_StopWatch;

#endif // PYBRICKS_PY_TOOLS

#endif // PYBRICKS_INCLUDED_PYBRICKS_TOOLS_H
