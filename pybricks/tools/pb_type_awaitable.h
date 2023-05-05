// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_TOOLS_AWAITABLE_H
#define PYBRICKS_INCLUDED_PYBRICKS_TOOLS_AWAITABLE_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/obj.h"

/**
 * Options for canceling an awaitable.
 */
typedef enum _pb_type_awaitable_cancel_opt_t {
    /** Do nothing to linked awaitables. */
    PB_TYPE_AWAITABLE_CANCEL_NONE       = 0,
    /**
     * Makes all linked awaitables end gracefully. Can be used if awaitables
     * running in parallel are using the same resources. This way, the newly
     * started operation "wins" and everything else is cancelled.
     */
    PB_TYPE_AWAITABLE_CANCEL_AWAITABLE  = 1 << 1,
    /**
     * Calls the cancel function for each linked awaitable that is not already
     * done. Only used to close hardware resources that aren't already cleaned
     * up by lower level drivers.
     */
    PB_TYPE_AWAITABLE_CANCEL_CALLBACK   = 1 << 2,
} pb_type_awaitable_cancel_opt_t;

/**
 * A generator-like type for waiting on some operation to complete.
 */
typedef struct _pb_type_awaitable_obj_t pb_type_awaitable_obj_t;

/**
 * Tests if awaitable operation is complete. Returns MP_OBJ_STOP_ITERATION if
 * done, possibly with return argument, else mp_const_none.
 *
 * On completion, this function is expected to close/stop hardware
 * operations as needed (hold a motor, etc.). This is not the same as cancel
 * below, which always stops the relevant hardware (i.e. always coast).
 */
typedef mp_obj_t (*pb_type_awaitable_test_completion_t)(mp_obj_t obj, uint32_t start_time);

/**
 * Called on cancel/close. Used to stop hardware operation in unhandled
 * conditions.
 */
typedef void (*pb_type_awaitable_cancel_t)(mp_obj_t obj);

/**
 * Constant configuration of functions and settings that govern the behavior
 * of an awaitable associated with a particular method or function.
 */
typedef struct _pb_type_awaitable_config_t {
    /**
     * Tests if operation is complete.
     */
    pb_type_awaitable_test_completion_t test_completion_func;
    /**
     * Called on cancellation.
     */
    pb_type_awaitable_cancel_t cancel_func;
    /**
     * Cancellation options.
     */
    pb_type_awaitable_cancel_opt_t cancel_opt;
} pb_type_awaitable_config_t;

void pb_type_awaitable_cancel_all(mp_obj_t obj, pb_type_awaitable_cancel_opt_t cancel_opt, pb_type_awaitable_obj_t *first_awaitable);

mp_obj_t pb_type_awaitable_await_or_block(mp_obj_t obj, const pb_type_awaitable_config_t *config, pb_type_awaitable_obj_t *first_awaitable);

#endif // PYBRICKS_PY_TOOLS

#endif // PYBRICKS_INCLUDED_PYBRICKS_TOOLS_AWAITABLE_H
