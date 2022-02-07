// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <pbio/error.h>
#include <pbio/task.h>

#include "py/mpconfig.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/nlr.h"
#include "py/runtime.h"

#include <pybricks/util_pb/pb_error.h>

/**
 * Waits for a task to complete.
 *
 * If an exception is raised while waiting, then the task is canceled.
 *
 * @param [in]  task    The task
 * @param [in]  timeout The timeout in milliseconds or -1 to wait forever.
 */
void pb_wait_task(pbio_task_t *task, mp_int_t timeout) {
    nlr_buf_t nlr;

    if (nlr_push(&nlr) == 0) {
        mp_uint_t start = mp_hal_ticks_ms();

        while (timeout < 0 || mp_hal_ticks_ms() - start < (mp_uint_t)timeout) {
            MICROPY_EVENT_POLL_HOOK

            if (task->status != PBIO_ERROR_AGAIN) {
                nlr_pop();
                pb_assert(task->status);
                return;
            }
        }

        mp_raise_OSError(MP_ETIMEDOUT);
        nlr_pop();
    } else {
        pbio_task_cancel(task);

        while (task->status == PBIO_ERROR_AGAIN) {
            MICROPY_VM_HOOK_LOOP
        }

        nlr_jump(nlr.ret_val);
    }
}
