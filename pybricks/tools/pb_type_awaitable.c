// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/mphal.h"
#include "py/mpstate.h"
#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_awaitable.h>

// The awaitable object is free to be reused.
#define AWAITABLE_FREE (NULL)

struct _pb_type_awaitable_obj_t {
    mp_obj_base_t base;
    /**
     * Object associated with this awaitable, such as the motor we wait on.
     */
    void *object;
    /**
     * End time. Gets passed to completion test to allow for graceful timeout
     * or raise timeout errors if desired.
     */
    uint32_t end_time;
    /**
     * Tests if operation is complete. Gets reset to AWAITABLE_FREE
     * on completion, which means that it can be used again.
     */
    pb_type_awaitable_test_completion_t test_completion;
    /**
     * Gets the return value of the awaitable.
     */
    pb_type_awaitable_return_t return_value;
    /**
     * Called on cancellation.
     */
    pb_type_awaitable_cancel_t cancel;
};

// close() cancels the awaitable.
STATIC mp_obj_t pb_type_awaitable_close(mp_obj_t self_in) {
    pb_type_awaitable_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->test_completion = AWAITABLE_FREE;
    // Handle optional clean up/cancelling of hardware operation.
    if (self->cancel) {
        self->cancel(self->object);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_awaitable_close_obj, pb_type_awaitable_close);

STATIC mp_obj_t pb_type_awaitable_iternext(mp_obj_t self_in) {
    pb_type_awaitable_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // If completed callback was unset, then we completed previously.
    if (self->test_completion == AWAITABLE_FREE) {
        return MP_OBJ_STOP_ITERATION;
    }

    // Keep going if not completed by returning None.
    if (!self->test_completion(self->object, self->end_time)) {
        return mp_const_none;
    }

    // Complete, so unset callback.
    self->test_completion = AWAITABLE_FREE;

    // For no return value, return basic stop iteration.
    if (!self->return_value) {
        return MP_OBJ_STOP_ITERATION;
    }

    // Otherwise, set return value via stop iteration.
    return mp_make_stop_iteration(self->return_value(self->object));
}

STATIC const mp_rom_map_elem_t pb_type_awaitable_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pb_type_awaitable_close_obj) },
};
MP_DEFINE_CONST_DICT(pb_type_awaitable_locals_dict, pb_type_awaitable_locals_dict_table);

// This is a partial implementation of the Python generator type. It is missing
// send(value) and throw(type[, value[, traceback]])
MP_DEFINE_CONST_OBJ_TYPE(pb_type_awaitable,
    MP_QSTR_wait,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    iter, pb_type_awaitable_iternext,
    locals_dict, &pb_type_awaitable_locals_dict);

/**
 * Gets an awaitable object that is not in use, or makes a new one.
 *
 * @param [in] awaitables_in        List of awaitables associated with @p object.
 */
STATIC pb_type_awaitable_obj_t *pb_type_awaitable_get(mp_obj_t awaitables_in) {

    mp_obj_list_t *awaitables = MP_OBJ_TO_PTR(awaitables_in);

    for (size_t i = 0; i < awaitables->len; i++) {
        pb_type_awaitable_obj_t *awaitable = MP_OBJ_TO_PTR(awaitables->items[i]);

        // Return awaitable if it is not in use.
        if (awaitable->test_completion == AWAITABLE_FREE) {
            return awaitable;
        }
    }

    // Otherwise allocate a new one.
    pb_type_awaitable_obj_t *awaitable = mp_obj_malloc(pb_type_awaitable_obj_t, &pb_type_awaitable);
    awaitable->test_completion = AWAITABLE_FREE;

    // Add to list of awaitables.
    mp_obj_list_append(awaitables_in, MP_OBJ_FROM_PTR(awaitable));

    return awaitable;
}

/**
 * Completion checker that is always true.
 *
 * Linked awaitables are gracefully cancelled by setting this as the completion
 * checker. This allows MicroPython to handle completion during the next call
 * to iternext.
 */
STATIC bool pb_type_awaitable_completed(mp_obj_t self_in, uint32_t start_time) {
    return true;
}

/**
 * Cancels all awaitables associated with an object.
 *
 * This is normally used by the function that makes a new awaitable, but it can
 * also be called independently to cancel without starting a new awaitable.
 *
 * @param [in] object                The object whose method we want to wait for completion.
 * @param [in] awaitables_in         List of awaitables associated with @p object.
 * @param [in] options               Controls awaitable behavior.
 */
void pb_type_awaitable_cancel_all(void *object, mp_obj_t awaitables_in, pb_type_awaitable_opt_t options) {

    // Exit if nothing to do.
    if (!pb_module_tools_run_loop_is_active() || options == PB_TYPE_AWAITABLE_OPT_NONE) {
        return;
    }

    mp_obj_list_t *awaitables = MP_OBJ_TO_PTR(awaitables_in);

    for (size_t i = 0; i < awaitables->len; i++) {
        pb_type_awaitable_obj_t *awaitable = MP_OBJ_TO_PTR(awaitables->items[i]);
        // Only cancel awaitables that are in use.
        if (awaitable->test_completion) {
            // Cancel hardware operation if requested and available.
            if (options & PB_TYPE_AWAITABLE_CANCEL_LINKED_CALLBACK && awaitable->cancel) {
                awaitable->cancel(awaitable->object);
            }
            // Set awaitable to done so it gets cancelled it gracefully on the
            // next iteration.
            if (options & PB_TYPE_AWAITABLE_CANCEL_LINKED) {
                awaitable->test_completion = pb_type_awaitable_completed;
            }
        }
    }
}

/**
 * Get a new awaitable in async mode or block and wait for it to complete in sync mode.
 *
 * Automatically cancels any previous awaitables associated with the object if requested.
 *
 * @param [in] object                The object whose method we want to wait for completion.
 * @param [in] awaitables_in         List of awaitables associated with @p object.
 * @param [in] end_time              Wall time in milliseconds when the operation should end.
 *                                   May be arbitrary if completion function does not need it.
 * @param [in] test_completion_func  Function to test if the operation is complete.
 * @param [in] return_value_func     Function that gets the return value for the awaitable.
 * @param [in] cancel_func           Function to cancel the hardware operation.
 * @param [in] options               Controls awaitable behavior.
 */
mp_obj_t pb_type_awaitable_await_or_wait(
    void *object,
    mp_obj_t awaitables_in,
    uint32_t end_time,
    pb_type_awaitable_test_completion_t test_completion_func,
    pb_type_awaitable_return_t return_value_func,
    pb_type_awaitable_cancel_t cancel_func,
    pb_type_awaitable_opt_t options) {

    // Within run loop, return the generator that user program will iterate.
    if (pb_module_tools_run_loop_is_active()) {

        // Some operations are not allowed in async mode.
        if (options & PB_TYPE_AWAITABLE_OPT_FORCE_BLOCK) {
            pb_module_tools_assert_blocking();
        }

        // First cancel linked awaitables if requested.
        pb_type_awaitable_cancel_all(object, awaitables_in, options);

        // Gets free existing awaitable or creates a new one.
        pb_type_awaitable_obj_t *awaitable = pb_type_awaitable_get(awaitables_in);

        // Initialize awaitable.
        awaitable->object = object;
        awaitable->test_completion = test_completion_func;
        awaitable->return_value = return_value_func;
        awaitable->cancel = cancel_func;
        awaitable->end_time = end_time;
        return MP_OBJ_FROM_PTR(awaitable);
    }

    // Outside run loop, block until the operation is complete.
    while (test_completion_func && !test_completion_func(object, end_time)) {
        mp_hal_delay_ms(1);
    }
    if (!return_value_func) {
        return mp_const_none;
    }
    return return_value_func(object);
}

#endif // PYBRICKS_PY_TOOLS
