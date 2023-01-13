// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/mphal.h"

#include <pybricks/tools.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

/**
 * A generator-like type for waiting on a motor operation to complete.
 */
typedef struct _pb_type_tools_wait_obj_t pb_type_tools_wait_obj_t;

struct _pb_type_tools_wait_obj_t {
    mp_obj_base_t base;
    /**
     * When to stop waiting.
     */
    uint32_t end_time;
    /**
     * Whether this generator object is done and thus can be recycled.
     */
    bool has_ended;
    /**
     * Callback to call on completion or cancellation.
     */
    void (*callback)(void);
    /**
     * Linked list of awaitables.
     */
    pb_type_tools_wait_obj_t *next_awaitable;
};

STATIC mp_obj_t pb_type_tools_wait_iternext(mp_obj_t self_in) {
    pb_type_tools_wait_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Stop on reaching target time or if externally cancelled.
    if (mp_hal_ticks_ms() - self->end_time < (uint32_t)INT32_MAX || self->has_ended) {
        self->has_ended = true;
        if (self->callback) {
            self->callback();
        }
        return MP_OBJ_STOP_ITERATION;
    }
    // Not done, so keep going.
    return mp_const_none;
}

// close() cancels the awaitable.
STATIC mp_obj_t pb_type_tools_wait_close(mp_obj_t self_in) {
    pb_type_tools_wait_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->has_ended = true;
    if (self->callback) {
        self->callback();
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_tools_wait_close_obj, pb_type_tools_wait_close);

STATIC const mp_rom_map_elem_t pb_type_tools_wait_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pb_type_tools_wait_close_obj) },
};
MP_DEFINE_CONST_DICT(pb_type_tools_wait_locals_dict, pb_type_tools_wait_locals_dict_table);

// This is a partial implementation of the Python generator type. It is missing
// send(value) and throw(type[, value[, traceback]])
MP_DEFINE_CONST_OBJ_TYPE(pb_type_tools_wait,
    MP_QSTR_wait,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    iter, pb_type_tools_wait_iternext,
    locals_dict, &pb_type_tools_wait_locals_dict);

// Statically allocated awaitable from which all others can be found.
STATIC pb_type_tools_wait_obj_t first_awaitable;

// Reset first awaitable on initializing MicroPython.
void pb_type_tools_wait_reset(void) {
    first_awaitable.base.type = &pb_type_tools_wait;
    first_awaitable.has_ended = true;
    first_awaitable.next_awaitable = MP_OBJ_NULL;
}

mp_obj_t pb_type_tools_wait_new(mp_int_t duration, void (*callback)(void)) {

    // When to stop waiting.
    uint32_t end_time = mp_hal_ticks_ms() + (uint32_t)duration;

    // Find next available awaitable.
    pb_type_tools_wait_obj_t *awaitable = &first_awaitable;
    while (!awaitable->has_ended && awaitable->next_awaitable != MP_OBJ_NULL) {
        awaitable = awaitable->next_awaitable;
    }
    // If the last known awaitable is still in use, allocate another.
    if (!awaitable->has_ended) {
        // Attach to the previous one.
        awaitable->next_awaitable = m_new_obj(pb_type_tools_wait_obj_t);

        // Initialize the new awaitable.
        awaitable = awaitable->next_awaitable;
        awaitable->next_awaitable = MP_OBJ_NULL;
        awaitable->base.type = &pb_type_tools_wait;
    }

    // Initialize awaitable with the end time and callback.
    awaitable->callback = callback;
    awaitable->has_ended = duration < 0 ? true: false;
    awaitable->end_time = end_time;

    // Return the awaitable where the user can await it.
    return MP_OBJ_FROM_PTR(awaitable);
}

#endif // PYBRICKS_PY_TOOLS
