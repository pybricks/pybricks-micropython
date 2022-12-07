// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <pbio/error.h>
#include <pbio/task.h>
#include <pbsys/status.h>

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

        while (task->status == PBIO_ERROR_AGAIN
               #if !PYBRICKS_PY_COMMON_CHARGER
               // HACK: This ensures we don't make the application program wait
               // forever for a failed task cancellation when shutdown is
               // requested. See https://github.com/pybricks/pybricks-micropython/pull/129
               // We can remove this hack once we can ensure that task
               // cancellation does not fail (for known cases).
               && !pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)
               #endif
               ) {
            MICROPY_VM_HOOK_LOOP
        }

        nlr_jump(nlr.ret_val);
    }
}
