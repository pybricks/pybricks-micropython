// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_ASYNC_H
#define PYBRICKS_INCLUDED_ASYNC_H

#include "py/mpconfig.h"

#include "py/obj.h"

#include <pbio/os.h>

/**
 * Called on cancel/close. Used to stop hardware operation in unhandled
 * conditions.
 *
 * @param [in]  parent_obj        The parent object associated with this iterable.
 * @return                        Usually mp_const_none, for compatibility with typical close functions.
 */
typedef mp_obj_t (*pb_type_async_close_t)(mp_obj_t parent_obj);

/**
 * Function that computes the return object at the end of the operation.
 *
 * If NULL, the awaitable will return None at the end.
 *
 * @param [in]  parent_obj        The parent object associated with this iterable.
 * @return                        Value to return at the end of the iteration.
 */
typedef mp_obj_t (*pb_type_async_return_map_t)(mp_obj_t parent_obj);

/**
 * Run one iteration of the protothread associated with this iterable.
 *
 * @param [in]  state             State of the operation, usually the protothread state.
 * @param [in]  parent_obj        The parent object associated with this iterable.
 * @return                        ::PBIO_ERROR_AGAIN while ongoing
 *                                ::PBIO_SUCCESS on completion.
 *                                Other error values will be raised.
 */
typedef pbio_error_t (*pb_type_async_iterate_once_t)(pbio_os_state_t *state, mp_obj_t parent_obj);

/** Object representing the iterable that is returned by an awaitable operation. */
typedef struct {
    mp_obj_base_t base;
    /**
     * The object instance whose method made us. Usually a class instance whose
     * methods returned us.
     *
     * Special values:
     *      MP_OBJ_NULL: This iterable has been fully exhausted and can be reused.
     *      MP_OBJ_SENTINEL: This iterable is will raise StopIteration when it is iterated again.
     */
    mp_obj_t parent_obj;
    /**
     * The iterable function associated with this operation. Usually a protothread.
     *
     * Special values:
     *      NULL: This iterable will yield once and complete next time.
     */
    pb_type_async_iterate_once_t iter_once;
    /**
     * Close function to call when this iterable is closed.
     */
    pb_type_async_close_t close;
    /**
     * Function that computes the return object at the end of the operation.
     */
    pb_type_async_return_map_t return_map;
    /**
     * State of the protothread used by the iterable.
     */
    pbio_os_state_t state;
} pb_type_async_t;

mp_obj_t pb_type_async_wait_or_await(pb_type_async_t *config, pb_type_async_t **prev, bool stop_prev);

void pb_type_async_schedule_stop_iteration(pb_type_async_t *iter);

#endif // PYBRICKS_INCLUDED_ASYNC_H
