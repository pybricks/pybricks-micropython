// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_TOOLS_H
#define PYBRICKS_INCLUDED_PYBRICKS_TOOLS_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/obj.h"

#include <pbio/task.h>

void pb_module_tools_init(void);

bool pb_module_tools_run_loop_is_active(void);

void pb_module_tools_assert_blocking(void);

void pb_module_tools_pbio_task_do_blocking(pbio_task_t *task, mp_int_t timeout);

mp_obj_t pb_module_tools_pbio_task_wait_or_await(pbio_task_t *task);

extern const mp_obj_type_t pb_type_StopWatch;

extern const mp_obj_type_t pb_type_Task;

#endif // PYBRICKS_PY_TOOLS

#endif // PYBRICKS_INCLUDED_PYBRICKS_TOOLS_H
