// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _PB_TASK_H_
#define _PB_TASK_H_

#include <pbio/task.h>
#include "py/mpconfig.h"

void pb_wait_task(pbio_task_t *task, mp_int_t timeout);

#endif // _PB_TASK_H_
