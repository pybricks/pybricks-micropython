// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2025 The Pybricks Authors


#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "pb_type_async.h"

#include <pybricks/tools.h>
#include <pybricks/util_pb/pb_error.h>

/**
 * Cancels the iterable to it will stop awaiting.
 *
 * This will not call close(). Safe to call even if iter is NULL.
 *
 * @param [in] iter The awaitable object.
 */
void pb_type_async_cancel(pb_type_async_t *iter) {
    if (iter) {
        iter->parent_obj = MP_OBJ_NULL;
    }
}

mp_obj_t pb_type_async_close(mp_obj_t iter_in) {
    pb_type_async_t *iter = MP_OBJ_TO_PTR(iter_in);
    if (iter->close && iter->parent_obj != MP_OBJ_NULL) {
        iter->close(iter->parent_obj);
    }
    pb_type_async_cancel(iter);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_async_close_obj, pb_type_async_close);

static mp_obj_t pb_type_async_iternext(mp_obj_t iter_in) {
    pb_type_async_t *iter = MP_OBJ_TO_PTR(iter_in);

    // On special case of sentinel, yield once and complete next time.
    if (iter->parent_obj == MP_OBJ_SENTINEL) {
        iter->parent_obj = MP_OBJ_NULL;
        return mp_const_none;
    }

    // No object, so this has already ended.
    if (iter->parent_obj == MP_OBJ_NULL) {
        return MP_OBJ_STOP_ITERATION;
    }

    // Run one iteration of the protothread.
    pbio_error_t err = iter->iter_once(&iter->state, iter->parent_obj);

    // Yielded, keep going.
    if (err == PBIO_ERROR_AGAIN) {
        return mp_const_none;
    }

    // Raises on other errors, Proceeds on successful completion.
    pb_assert(err);

    // For no return map, return basic stop iteration, which results in None.
    if (!iter->return_map) {
        pb_type_async_cancel(iter);
        return MP_OBJ_STOP_ITERATION;
    }

    mp_obj_t return_obj = iter->return_map(iter->parent_obj);
    pb_type_async_cancel(iter);

    // Set return value via stop iteration.
    return mp_make_stop_iteration(return_obj);
}

static const mp_rom_map_elem_t pb_type_async_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pb_type_async_close_obj) },
};
MP_DEFINE_CONST_DICT(pb_type_async_locals_dict, pb_type_async_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_async,
    MP_QSTR_Async,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    iter, pb_type_async_iternext,
    locals_dict, &pb_type_async_locals_dict);

/**
 * Returns an awaitable operation if the runloop is active, or awaits the
 * operation here and now.
 *
 * @param  [in]  config     Configuration of the operation
 * @returns An awaitable if the runloop is active, otherwise the mapped return value.
 */
mp_obj_t pb_type_async_wait_or_await(pb_type_async_t *config) {

    config->base.type = &pb_type_async;

    // Return allocated awaitable if runloop active.
    if (pb_module_tools_run_loop_is_active()) {
        pb_type_async_t *iter = (pb_type_async_t *)m_malloc(sizeof(pb_type_async_t));
        *iter = *config;
        return MP_OBJ_FROM_PTR(iter);
    }

    // Otherwise wait for completion here without allocating the iterable.
    pbio_error_t err;
    while ((err = config->iter_once(&config->state, config->parent_obj)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK;
    }
    pb_assert(err);
    return config->return_map ? config->return_map(config->parent_obj) : mp_const_none;
}
