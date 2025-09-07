// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2025 The Pybricks Authors


#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "pb_type_async.h"

#include <pybricks/tools.h>
#include <pybricks/util_pb/pb_error.h>

/**
 * Cancels the iterable so it will stop awaiting.
 *
 * This will not call close(). Safe to call even if iter is NULL.
 *
 * @param [in] iter The awaitable object.
 */
void pb_type_async_schedule_cancel(pb_type_async_t *iter) {
    if (!iter) {
        return;
    }
    // Don't set it to MP_OBJ_NULL right away, or the calling code wouldn't
    // know it was exhausted, and it would await on the renewed operation.
    iter->parent_obj = MP_OBJ_SENTINEL;
}

mp_obj_t pb_type_async_close(mp_obj_t iter_in) {
    pb_type_async_t *iter = MP_OBJ_TO_PTR(iter_in);
    if (iter->close && iter->parent_obj != MP_OBJ_NULL) {
        iter->close(iter->parent_obj);
    }
    // Closing is stronger than cancellation. In case of close, we expect that
    // the awaitable is no longer to be iterated afterwards, so it would not
    // reach exhaustion on its own and could never be re-used, so do it here.
    iter->parent_obj = MP_OBJ_NULL;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_async_close_obj, pb_type_async_close);

static mp_obj_t pb_type_async_iternext(mp_obj_t iter_in) {
    pb_type_async_t *iter = MP_OBJ_TO_PTR(iter_in);

    // It was scheduled for cancellation externally (or exhausted normally
    // previously). We are hereby letting the calling code know we are
    // exhausted, so now we can set parent_obj to MP_OBJ_NULL to indicate it is
    // ready to be used again. This assumes user did not keep a reference to it
    // and does not next() or await it again. It is safe if they do, but the
    // user code would be awaiting whatever it is re-used for.
    if (iter->parent_obj == MP_OBJ_SENTINEL || iter->parent_obj == MP_OBJ_NULL) {
        iter->parent_obj = MP_OBJ_NULL;
        return MP_OBJ_STOP_ITERATION;
    }

    // Special case without iterator means yield exactly once and then complete.
    if (!iter->iter_once) {
        pb_type_async_schedule_cancel(iter);
        return mp_const_none;
    }

    // Run one iteration of the protothread.
    pbio_error_t err = iter->iter_once(&iter->state, iter->parent_obj);

    // Yielded, keep going.
    if (err == PBIO_ERROR_AGAIN) {
        return mp_const_none;
    }

    // Raises on other errors, Proceeds on successful completion.
    pb_assert(err);

    // This causes the stop iteration to provide the return value.
    if (iter->return_map) {
        mp_make_stop_iteration(iter->return_map(iter->parent_obj));
    }

    // As above, notify caller of exhaustion so this iterable can be re-used.
    iter->parent_obj = MP_OBJ_NULL;
    return MP_OBJ_STOP_ITERATION;
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
 * @param  [in]  prev       Candidate iterable object that might be re-used.
 * @returns An awaitable if the runloop is active, otherwise the mapped return value.
 */
mp_obj_t pb_type_async_wait_or_await(pb_type_async_t *config, pb_type_async_t **prev) {

    config->base.type = &pb_type_async;

    // Return allocated awaitable if runloop active.
    if (pb_module_tools_run_loop_is_active()) {
        // Re-use existing awaitable if exists and is free, otherwise allocate
        // another one. This allows many resources with one concurrent physical
        // operation like a motor to operate without re-allocation.
        pb_type_async_t *iter = (prev && *prev && (*prev)->parent_obj == MP_OBJ_NULL) ?
            *prev : (pb_type_async_t *)m_malloc(sizeof(pb_type_async_t));
        *iter = *config;
        if (prev) {
            *prev = iter;
        }

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
