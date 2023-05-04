// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/mphal.h"
#include "py/mpstate.h"

#include <pybricks/tools.h>

/**
 * A generator-like type for waiting on some operation to complete.
 */
typedef struct _pb_type_tools_awaitable_obj_t pb_type_tools_awaitable_obj_t;

struct _pb_type_tools_awaitable_obj_t {
    mp_obj_base_t base;
    /**
     * Object associated with this awaitable, such as the motor we wait on.
     */
    mp_obj_t obj;
    /**
     * End time. Only used for simple waits with no associated object.
     */
    uint32_t end_time;
    /**
     * Tests if operation is complete.
     */
    pb_awaitable_test_completion_t test_completion;
    /**
     * Called on cancellation.
     */
    pb_awaitable_cancel_t cancel;
    /**
     * Linked list of awaitables.
     */
    pb_type_tools_awaitable_obj_t *next_awaitable;
};

// close() cancels the awaitable.
STATIC mp_obj_t pb_type_tools_awaitable_close(mp_obj_t self_in) {
    pb_type_tools_awaitable_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->test_completion = NULL;
    // Handle optional clean up/cancelling of hardware operation.
    if (self->cancel) {
        self->cancel(self->obj);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_tools_awaitable_close_obj, pb_type_tools_awaitable_close);

STATIC mp_obj_t pb_type_tools_awaitable_iternext(mp_obj_t self_in) {
    pb_type_tools_awaitable_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // If completed callback was unset, then we are done.
    if (self->test_completion == NULL) {
        return MP_OBJ_STOP_ITERATION;
    }

    // Test completion status.
    mp_obj_t completion = self->test_completion(self->obj);

    // none means keep going, everything else means done so finalize.
    if (completion != mp_const_none) {
        self->test_completion = NULL;
    }
    return completion;
}

STATIC const mp_rom_map_elem_t pb_type_tools_awaitable_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pb_type_tools_awaitable_close_obj) },
};
MP_DEFINE_CONST_DICT(pb_type_tools_awaitable_locals_dict, pb_type_tools_awaitable_locals_dict_table);

// This is a partial implementation of the Python generator type. It is missing
// send(value) and throw(type[, value[, traceback]])
MP_DEFINE_CONST_OBJ_TYPE(pb_type_tools_awaitable,
    MP_QSTR_wait,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    iter, pb_type_tools_awaitable_iternext,
    locals_dict, &pb_type_tools_awaitable_locals_dict);

// Statically allocated awaitable from which all others can be found.
STATIC pb_type_tools_awaitable_obj_t first_awaitable;

// Reset first awaitable on initializing MicroPython.
void pb_type_tools_awaitable_init(void) {
    first_awaitable.base.type = &pb_type_tools_awaitable;
    first_awaitable.test_completion = NULL;
    first_awaitable.next_awaitable = MP_OBJ_NULL;
}

STATIC pb_type_tools_awaitable_obj_t *pb_type_tools_awaitable_get(void) {

    // Find next available awaitable.
    pb_type_tools_awaitable_obj_t *awaitable = &first_awaitable;
    while (awaitable->test_completion != NULL && awaitable->next_awaitable != MP_OBJ_NULL) {
        awaitable = awaitable->next_awaitable;
    }
    // If the last known awaitable is still in use, allocate another.
    if (awaitable->test_completion != NULL) {
        // Attach to the previous one.
        awaitable->next_awaitable = mp_obj_malloc(pb_type_tools_awaitable_obj_t, &pb_type_tools_awaitable);

        // Initialize the new awaitable.
        awaitable = awaitable->next_awaitable;
        awaitable->next_awaitable = MP_OBJ_NULL;
    }
    return awaitable;
}

// Get an available awaitable, add callbacks, and return as object so user can await it.
STATIC mp_obj_t pb_type_tools_awaitable_new(mp_obj_t obj, pb_awaitable_test_completion_t test_completion, pb_awaitable_cancel_t cancel) {
    pb_type_tools_awaitable_obj_t *awaitable = pb_type_tools_awaitable_get();
    awaitable->obj = obj;
    awaitable->test_completion = test_completion;
    awaitable->cancel = cancel;
    return MP_OBJ_FROM_PTR(awaitable);
}

STATIC mp_obj_t pb_tools_wait_test_completion(mp_obj_t obj) {
    pb_type_tools_awaitable_obj_t *awaitable = MP_OBJ_TO_PTR(obj);
    return (mp_hal_ticks_ms() - awaitable->end_time) < (uint32_t)INT32_MAX ? MP_OBJ_STOP_ITERATION : mp_const_none;
}

mp_obj_t pb_type_tools_await_time(mp_obj_t duration_in) {
    mp_int_t duration = mp_obj_get_int(duration_in);
    pb_type_tools_awaitable_obj_t *awaitable = pb_type_tools_awaitable_get();
    awaitable->obj = MP_OBJ_FROM_PTR(awaitable);
    awaitable->test_completion = duration > 0 ? pb_tools_wait_test_completion : NULL;
    awaitable->end_time = mp_hal_ticks_ms() + (uint32_t)duration;
    awaitable->cancel = NULL;
    return MP_OBJ_FROM_PTR(awaitable);
}

// Helper functions to simplify the final part of most methods that can be
// either blocking or not, like motor methods.
mp_obj_t pb_type_tools_await_or_wait(mp_obj_t obj, pb_awaitable_test_completion_t test_completion, pb_awaitable_cancel_t cancel) {

    // Make an awaitable object for the given operation.
    mp_obj_t generator = pb_type_tools_awaitable_new(obj, test_completion, cancel);

    // Within run loop, just return the generator that user program will iterate.
    if (pb_module_tools_run_loop_is_active()) {
        return generator;
    }

    // Otherwise block and wait for it to complete.
    nlr_buf_t nlr;
    mp_obj_t ret = MP_OBJ_NULL;
    if (nlr_push(&nlr) == 0) {
        while (pb_type_tools_awaitable_iternext(generator) == mp_const_none) {
            mp_hal_delay_ms(5);
        }
        ret = MP_STATE_THREAD(stop_iteration_arg);
        nlr_pop();
    } else {
        // Cancel the operation if an exception was raised.
        pb_type_tools_awaitable_obj_t *self = MP_OBJ_TO_PTR(generator);
        if (self->cancel) {
            self->cancel(self->obj);
        }
        nlr_jump(nlr.ret_val);
    }
    return ret == MP_OBJ_NULL ? mp_const_none : ret;
}

#endif // PYBRICKS_PY_TOOLS
