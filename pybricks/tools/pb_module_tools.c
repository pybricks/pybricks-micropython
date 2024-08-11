// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/builtin.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/objmodule.h"
#include "py/runtime.h"
#include "py/stream.h"

#include <pbio/int_math.h>
#include <pbio/task.h>
#include <pbsys/light.h>
#include <pbsys/program_stop.h>
#include <pbsys/status.h>

#include <pybricks/parameters.h>
#include <pybricks/common.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_matrix.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>


// Global state of the run loop for async user programs. Gets set when run_task
// is called and cleared when it completes
static bool run_loop_is_active;

bool pb_module_tools_run_loop_is_active(void) {
    return run_loop_is_active;
}

void pb_module_tools_assert_blocking(void) {
    if (run_loop_is_active) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("This can only be called before multitasking starts."));
    }
}

// The awaitables for the wait() function have no object associated with
// it (unlike e.g. a motor), so we make a starting point here. These never
// have to cancel each other so shouldn't need to be in a list, but this lets
// us share the same code with other awaitables. It also minimizes allocation.
MP_REGISTER_ROOT_POINTER(mp_obj_t wait_awaitables);

static bool pb_module_tools_wait_test_completion(mp_obj_t obj, uint32_t end_time) {
    return mp_hal_ticks_ms() - end_time < UINT32_MAX / 2;
}

static mp_obj_t pb_module_tools_wait(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(time));

    mp_int_t time = pb_obj_get_int(time_in);

    // outside run loop, do blocking wait. This would be handled below as well,
    // but for this very simple call we'd rather avoid the overhead.
    if (!pb_module_tools_run_loop_is_active()) {
        if (time > 0) {
            mp_hal_delay_ms(time);
        }
        return mp_const_none;
    }

    // Require that duration is nonnegative small int. This makes it cheaper to
    // test completion state in iteration loop.
    time = pbio_int_math_bind(time, 0, INT32_MAX >> 2);

    return pb_type_awaitable_await_or_wait(
        NULL, // wait functions are not associated with an object
        MP_STATE_PORT(wait_awaitables),
        mp_hal_ticks_ms() + (time < 0 ? 0 : time),
        pb_module_tools_wait_test_completion,
        pb_type_awaitable_return_none,
        pb_type_awaitable_cancel_none,
        PB_TYPE_AWAITABLE_OPT_NONE);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_module_tools_wait_obj, 0, pb_module_tools_wait);

/**
 * Waits for a task to complete.
 *
 * If an exception is raised while waiting, then the task is canceled.
 *
 * @param [in]  task    The task
 * @param [in]  timeout The timeout in milliseconds or -1 to wait forever.
 */
void pb_module_tools_pbio_task_do_blocking(pbio_task_t *task, mp_int_t timeout) {

    pb_module_tools_assert_blocking();

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
        MP_UNREACHABLE
    } else {
        pbio_task_cancel(task);

        while (task->status == PBIO_ERROR_AGAIN) {
            MICROPY_VM_HOOK_LOOP

            // Stop waiting (and potentially blocking) in case of forced shutdown.
            if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
                break;
            }
        }

        nlr_jump(nlr.ret_val);
    }
}

// The awaitables associated with pbio tasks can originate from different
// objects. At the moment, they are only associated with Bluetooth tasks, and
// they cannot run at the same time. So we keep a single list of awaitables
// here instead of with each Bluetooth-related MicroPython object.
MP_REGISTER_ROOT_POINTER(mp_obj_t pbio_task_awaitables);

static bool pb_module_tools_pbio_task_test_completion(mp_obj_t obj, uint32_t end_time) {
    pbio_task_t *task = MP_OBJ_TO_PTR(obj);

    // Keep going if not done yet.
    if (task->status == PBIO_ERROR_AGAIN) {
        return false;
    }

    // If done, make sure it was successful.
    pb_assert(task->status);
    return true;
}

mp_obj_t pb_module_tools_pbio_task_wait_or_await(pbio_task_t *task) {
    return pb_type_awaitable_await_or_wait(
        MP_OBJ_FROM_PTR(task),
        MP_STATE_PORT(pbio_task_awaitables),
        pb_type_awaitable_end_time_none,
        pb_module_tools_pbio_task_test_completion,
        pb_type_awaitable_return_none,
        pb_type_awaitable_cancel_none,
        PB_TYPE_AWAITABLE_OPT_RAISE_ON_BUSY);
}

/**
 * Reads one byte from stdin without blocking if a byte is available, and
 * optionally converts it to character representation.
 *
 * @param [in]  last    Choose @c True to read until the last byte is read
 *                      or @c False to get the first available byte.
 * @param [in]  chr     Choose @c False to return the integer value of the byte.
 *                      Choose @c True to return a single character string of
 *                      the resulting byte if it is printable and otherwise
 *                      return @c None .
 *
 * @returns The resulting byte if there was one, converted as above, otherwise @c None .
 *
 */
static mp_obj_t pb_module_tools_read_input_byte(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_FALSE(last),
        PB_ARG_DEFAULT_FALSE(chr));

    int chr = -1;

    while ((mp_hal_stdio_poll(MP_STREAM_POLL_RD) & MP_STREAM_POLL_RD)) {
        // REVISIT: In theory, this should not block if mp_hal_stdio_poll() and
        // mp_hal_stdin_rx_chr() are implemented correctly and nothing happens
        // in a thread/interrupt/kernel that changes the state.
        chr = mp_hal_stdin_rx_chr();

        // For last=False, break to stop at first byte. Otherwise, keep reading.
        if (!mp_obj_is_true(last_in)) {
            break;
        }
    }

    // If no data is available, return None.
    if (chr < 0) {
        return mp_const_none;
    }

    // If chr=False, return the integer value of the byte.
    if (!mp_obj_is_true(chr_in)) {
        return MP_OBJ_NEW_SMALL_INT(chr);
    }

    // If char requested but not printable, return None.
    if (chr < 32 || chr > 126) {
        return mp_const_none;
    }

    // Return the character as a string.
    const char result[] = {chr};
    return mp_obj_new_str(result, sizeof(result));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_module_tools_read_input_byte_obj, 0, pb_module_tools_read_input_byte);

static mp_obj_t pb_module_tools_run_task(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_NONE(task),
        PB_ARG_DEFAULT_INT(loop_time, 10));

    // Without args, this function is used to test if the run loop is active.
    if (n_args == 0) {
        return mp_obj_new_bool(run_loop_is_active);
    }

    // Can only run one loop at a time.
    if (run_loop_is_active) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Run loop already active."));
    }

    run_loop_is_active = true;

    uint32_t start_time = mp_hal_ticks_ms();
    uint32_t loop_time = pb_obj_get_positive_int(loop_time_in);

    mp_obj_iter_buf_t iter_buf;
    mp_obj_t iterable = mp_getiter(task_in, &iter_buf);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {

        while (mp_iternext(iterable) != MP_OBJ_STOP_ITERATION) {

            gc_collect();

            if (loop_time == 0) {
                continue;
            }

            uint32_t elapsed = mp_hal_ticks_ms() - start_time;
            if (elapsed < loop_time) {
                mp_hal_delay_ms(loop_time - elapsed);
            }
            start_time += loop_time;
        }

        nlr_pop();
        run_loop_is_active = false;
    } else {
        run_loop_is_active = false;
        nlr_jump(nlr.ret_val);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_module_tools_run_task_obj, 0, pb_module_tools_run_task);

// Reset global awaitable state when user program starts.
void pb_module_tools_init(void) {
    MP_STATE_PORT(wait_awaitables) = mp_obj_new_list(0, NULL);
    MP_STATE_PORT(pbio_task_awaitables) = mp_obj_new_list(0, NULL);
    run_loop_is_active = false;
}

#if PYBRICKS_PY_TOOLS_HUB_MENU

static void pb_module_tools_hub_menu_display_symbol(mp_obj_t symbol) {
    if (mp_obj_is_str(symbol)) {
        pb_type_LightMatrix_display_char(pbsys_hub_light_matrix, symbol);
    } else {
        pb_type_LightMatrix_display_number(pbsys_hub_light_matrix, symbol);
    }
}

/**
 * Waits for a button press or release.
 *
 * @param [in]  press   Choose @c true to wait for press or @c false to wait for release.
 * @returns             When waiting for pressed, it returns the button that was pressed, otherwise returns 0.
 */
static pbio_button_flags_t pb_module_tools_hub_menu_wait_for_press(bool press) {

    // This function should only be used in a blocking context.
    pb_module_tools_assert_blocking();

    pbio_error_t err;
    pbio_button_flags_t btn;
    while ((err = pbio_button_is_pressed(&btn)) == PBIO_SUCCESS && ((bool)btn) == !press) {
        MICROPY_EVENT_POLL_HOOK;
    }
    pb_assert(err);
    return btn;
}

/**
 * Displays a menu on the hub display and allows the user to pick a symbol
 * using the buttons.
 *
 * @param [in]  n_args  The number of args.
 * @param [in]  args    The args passed in Python code (the menu entries).
 */
static mp_obj_t pb_module_tools_hub_menu(size_t n_args, const mp_obj_t *args) {

    // Validate arguments by displaying all of them, ending with the first.
    // This ensures we fail right away instead of midway through the menu. It
    // happens so fast that there isn't a time penalty for this.
    for (int i = n_args - 1; i >= 0; i--) {
        pb_module_tools_hub_menu_display_symbol(args[i]);
    }

    // Disable stop button and cache original setting to restore later.
    pbio_button_flags_t stop_button = pbsys_program_stop_get_buttons();
    pbsys_program_stop_set_buttons(0);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {

        size_t selection = 0;

        while (true) {
            pb_module_tools_hub_menu_wait_for_press(false);
            pbio_button_flags_t btn = pb_module_tools_hub_menu_wait_for_press(true);

            // Selection made, exit.
            if (btn & PBIO_BUTTON_CENTER) {
                break;
            }

            // Increment/decrement selection for left/right buttons.
            if (btn & PBIO_BUTTON_RIGHT) {
                selection = (selection + 1) % n_args;
            } else if (btn & PBIO_BUTTON_LEFT) {
                selection = selection == 0 ? n_args - 1 : selection - 1;
            }

            // Display current selection.
            pb_module_tools_hub_menu_display_symbol(args[selection]);
        }

        // Wait for release before returning, just like starting a normal program.
        pb_module_tools_hub_menu_wait_for_press(false);

        // Restore stop button setting prior to starting menu.
        pbsys_program_stop_set_buttons(stop_button);

        // Complete and return selected object.
        nlr_pop();
        return args[selection];
    } else {
        pbsys_program_stop_set_buttons(stop_button);
        nlr_jump(nlr.ret_val);
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR(pb_module_tools_hub_menu_obj, 2, pb_module_tools_hub_menu);

#endif // PYBRICKS_PY_TOOLS_HUB_MENU

static const mp_rom_map_elem_t tools_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_tools)                    },
    { MP_ROM_QSTR(MP_QSTR_wait),        MP_ROM_PTR(&pb_module_tools_wait_obj)         },
    { MP_ROM_QSTR(MP_QSTR_read_input_byte), MP_ROM_PTR(&pb_module_tools_read_input_byte_obj) },
    #if PYBRICKS_PY_TOOLS_HUB_MENU
    { MP_ROM_QSTR(MP_QSTR_hub_menu),    MP_ROM_PTR(&pb_module_tools_hub_menu_obj)     },
    #endif // PYBRICKS_PY_TOOLS_HUB_MENU
    { MP_ROM_QSTR(MP_QSTR_run_task),    MP_ROM_PTR(&pb_module_tools_run_task_obj)     },
    { MP_ROM_QSTR(MP_QSTR_StopWatch),   MP_ROM_PTR(&pb_type_StopWatch)                },
    { MP_ROM_QSTR(MP_QSTR_multitask),   MP_ROM_PTR(&pb_type_Task)                     },
    #if MICROPY_PY_BUILTINS_FLOAT
    { MP_ROM_QSTR(MP_QSTR_Matrix),      MP_ROM_PTR(&pb_type_Matrix)           },
    { MP_ROM_QSTR(MP_QSTR_vector),      MP_ROM_PTR(&pb_geometry_vector_obj)   },
    { MP_ROM_QSTR(MP_QSTR_cross),       MP_ROM_PTR(&pb_type_matrix_cross_obj) },
    // backwards compatibility for pybricks.geometry.Axis
    { MP_ROM_QSTR(MP_QSTR_Axis),        MP_ROM_PTR(&pb_enum_type_Axis) },
    #endif // MICROPY_PY_BUILTINS_FLOAT
};
static MP_DEFINE_CONST_DICT(pb_module_tools_globals, tools_globals_table);

const mp_obj_module_t pb_module_tools = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_tools_globals,
};

#if PYBRICKS_RUNS_ON_EV3DEV
// ev3dev extends the C module in Python
MP_REGISTER_MODULE(MP_QSTR__tools, pb_module_tools);
#else
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_tools, pb_module_tools);
#endif

// backwards compatibility for pybricks.geometry
#if MICROPY_PY_BUILTINS_FLOAT
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_geometry, pb_module_tools);
#endif // MICROPY_PY_BUILTINS_FLOAT

#endif // PYBRICKS_PY_TOOLS
