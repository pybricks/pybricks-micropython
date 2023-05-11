// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/mphal.h"
#include "py/mpstate.h"
#include "py/runtime.h"

#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_awaitable.h>

struct _pb_type_awaitable_obj_t {
    mp_obj_base_t base;
    /**
     * Object associated with this awaitable, such as the motor we wait on.
     */
    mp_obj_t obj;
    /**
     * Start time. Gets passed to completion test to allow for graceful timeout.
     */
    uint32_t start_time;
    /**
     * Tests if operation is complete. Gets reset to NULL on completion,
     * which means that it can be used again.
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
    /**
     * Linked list of awaitables.
     */
    pb_type_awaitable_obj_t *next_awaitable;
};

// close() cancels the awaitable.
STATIC mp_obj_t pb_type_awaitable_close(mp_obj_t self_in) {
    pb_type_awaitable_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->test_completion = NULL;
    // Handle optional clean up/cancelling of hardware operation.
    if (self->cancel) {
        self->cancel(self->obj);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_awaitable_close_obj, pb_type_awaitable_close);

STATIC mp_obj_t pb_type_awaitable_iternext(mp_obj_t self_in) {
    pb_type_awaitable_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // If completed callback was unset, then we completed previously.
    if (self->test_completion == NULL) {
        return MP_OBJ_STOP_ITERATION;
    }

    // Keep going if not completed by returning None.
    if (!self->test_completion(self->obj, self->start_time)) {
        return mp_const_none;
    }

    // Complete, so unset callback.
    self->test_completion = NULL;

    // For no return value, return basic stop iteration.
    if (!self->return_value) {
        return MP_OBJ_STOP_ITERATION;
    }

    // Otherwise, set return value via stop iteration.
    return mp_make_stop_iteration(self->return_value(self->obj));
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

STATIC pb_type_awaitable_obj_t *pb_type_awaitable_get(pb_type_awaitable_obj_t *first_awaitable) {

    // Find next available awaitable that exists and is not used.
    pb_type_awaitable_obj_t *awaitable = first_awaitable;
    while (awaitable->next_awaitable && awaitable->test_completion) {
        awaitable = awaitable->next_awaitable;
    }
    // Above loop stops if a) there is no next awaitable or b) the current
    // awaitable is not in use. Only case a) requires allocating another.
    if (!awaitable->next_awaitable) {
        // Attach to the previous one.
        awaitable->next_awaitable = mp_obj_malloc(pb_type_awaitable_obj_t, &pb_type_awaitable);

        // Initialize the new awaitable.
        awaitable = awaitable->next_awaitable;
        awaitable->test_completion = NULL;
        awaitable->next_awaitable = NULL;
    }
    return awaitable;
}

/**
 * Cancels all awaitables associated with an object.
 *
 * This is normally used by the function that makes a new awaitable, but it can
 * also be called independently to cancel without starting a new awaitable.
 *
 * @param [in] obj                   The object whose method we want to wait for completion.
 * @param [in] first_awaitable       The first awaitable in the linked list of awaitables from @p obj.
 * @param [in] cancel_opt            Whether to cancel linked awaitables, hardware, or both.
 */
void pb_type_awaitable_cancel_all(mp_obj_t obj, pb_type_awaitable_obj_t *first_awaitable, pb_type_awaitable_cancel_opt_t cancel_opt) {

    // Exit if nothing to do.
    if (!pb_module_tools_run_loop_is_active() || cancel_opt == PB_TYPE_AWAITABLE_CANCEL_NONE || !first_awaitable) {
        return;
    }

    pb_type_awaitable_obj_t *awaitable = first_awaitable;
    while (awaitable->next_awaitable) {
        // Don't cancel if already done.
        if (!awaitable->test_completion) {
            continue;
        }
        // Cancel hardware operation if requested and available.
        if (cancel_opt & PB_TYPE_AWAITABLE_CANCEL_CALLBACK && awaitable->cancel) {
            awaitable->cancel(awaitable->obj);
        }
        // Set awaitable to done in order to cancel it gracefully.
        if (cancel_opt & PB_TYPE_AWAITABLE_CANCEL_AWAITABLE) {
            awaitable->test_completion = NULL;
        }
        awaitable = awaitable->next_awaitable;
    }
}

/**
 * Get a new awaitable in async mode or block and wait for it to complete in sync mode.
 *
 * Automatically cancels any previous awaitables associated with the object if requested.
 *
 * @param [in] obj                   The object whose method we want to wait for completion.
 * @param [in] first_awaitable       The first awaitable in the linked list of awaitables from @p obj.
 * @param [in] test_completion_func  Function to test if the operation is complete.
 * @param [in] return_value_func     Function that gets the return value for the awaitable.
 * @param [in] cancel_func           Function to cancel the hardware operation.
 * @param [in] cancel_opt            Whether to cancel linked awaitables, hardware, or both.
 */
mp_obj_t pb_type_awaitable_await_or_wait(
    mp_obj_t obj,
    pb_type_awaitable_obj_t **first_awaitable,
    pb_type_awaitable_test_completion_t test_completion_func,
    pb_type_awaitable_return_t return_value_func,
    pb_type_awaitable_cancel_t cancel_func,
    pb_type_awaitable_cancel_opt_t cancel_opt) {

    uint32_t start_time = mp_hal_ticks_ms();

    // Within run loop, return the generator that user program will iterate.
    if (pb_module_tools_run_loop_is_active()) {
        // First cancel linked awaitables if requested.
        pb_type_awaitable_cancel_all(obj, *first_awaitable, cancel_opt);

        // If the first awaitable was not yet created, do so now.
        if (!*first_awaitable) {
            *first_awaitable = mp_obj_malloc(pb_type_awaitable_obj_t, &pb_type_awaitable);
            (*first_awaitable)->test_completion = NULL;
            (*first_awaitable)->next_awaitable = NULL;
        }

        // Gets existing awaitable or creates a new one.
        pb_type_awaitable_obj_t *awaitable = pb_type_awaitable_get(*first_awaitable);

        // Initialize awaitable.
        awaitable->obj = obj;
        awaitable->test_completion = test_completion_func;
        awaitable->return_value = return_value_func;
        awaitable->cancel = cancel_func;
        awaitable->start_time = start_time;
        return MP_OBJ_FROM_PTR(awaitable);
    }

    // Outside run loop, block until the operation is complete.
    while (!test_completion_func(obj, start_time)) {
        mp_hal_delay_ms(1);
    }
    if (!return_value_func) {
        return mp_const_none;
    }
    return return_value_func(obj);
}

#endif // PYBRICKS_PY_TOOLS
