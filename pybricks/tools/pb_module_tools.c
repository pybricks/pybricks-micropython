// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include <string.h>

#include "py/builtin.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/objmodule.h"
#include "py/runtime.h"
#include "py/stream.h"

#include <pbio/int_math.h>
#include <pbio/util.h>
#include <pbsys/light.h>
#include <pbsys/program_stop.h>
#include <pbsys/status.h>

#include <pybricks/parameters.h>
#include <pybricks/common.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_async.h>
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

/**
 * Statically allocated wait objects that can be re-used without allocation
 * once exhausted. Should be sufficient for trivial applications.
 *
 * More are allocated as needed. If a user has more than this many parallel
 * waits, the user can probably afford to allocate anyway.
 *
 * This is set to zero each time MicroPython starts.
 */
static pb_type_async_t waits[6];

static pbio_error_t pb_module_tools_wait_iter_once(pbio_os_state_t *state, mp_obj_t parent_obj) {
    // Not a protothread, but using the state variable to store final time.
    return pbio_util_time_has_passed(pbdrv_clock_get_ms(), (uint32_t)*state) ? PBIO_SUCCESS: PBIO_ERROR_AGAIN;
}

static mp_obj_t pb_module_tools_wait(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(time));

    mp_int_t time = pb_obj_get_int(time_in);

    // Outside run loop, do blocking wait to avoid async overhead.
    if (!pb_module_tools_run_loop_is_active()) {
        if (time > 0) {
            mp_hal_delay_ms(time);
        }
        return mp_const_none;
    }

    // Find statically allocated candidate that can be re-used again because
    // it was never used or used and exhausted. If it stays at NULL then a new
    // awaitable is allocated.
    pb_type_async_t *reuse = NULL;
    for (uint32_t i = 0; i < MP_ARRAY_SIZE(waits); i++) {
        if (waits[i].parent_obj == MP_OBJ_NULL) {
            reuse = &waits[i];
            break;
        }
    }

    pb_type_async_t config = {
        // Not associated with any parent object.
        .parent_obj = mp_const_none,
        // Yield once for duration 0 to avoid blocking loops.
        .iter_once = time == 0 ? NULL : pb_module_tools_wait_iter_once,
        // No protothread here; use it to encode end time.
        .state = pbdrv_clock_get_ms() + (uint32_t)time,
    };

    return pb_type_async_wait_or_await(&config, &reuse, false);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_module_tools_wait_obj, 0, pb_module_tools_wait);

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
        PB_ARG_DEFAULT_NONE(task));

    // Without args, this function is used to test if the run loop is active.
    if (task_in == mp_const_none) {
        return mp_obj_new_bool(run_loop_is_active);
    }

    // Can only run one loop at a time.
    if (run_loop_is_active) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Run loop already active."));
    }

    mp_obj_iter_buf_t iter_buf;
    nlr_buf_t nlr;

    if (nlr_push(&nlr) == 0) {
        run_loop_is_active = true;
        mp_obj_t iterable = mp_getiter(task_in, &iter_buf);
        while (mp_iternext(iterable) != MP_OBJ_STOP_ITERATION) {
            // Keep running system processes.
            MICROPY_VM_HOOK_LOOP
            // Stop on exception such as SystemExit.
            mp_handle_pending(true);
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
    memset(waits, 0, sizeof(waits));
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

    pbio_button_flags_t btn;
    while ((bool)(btn = pbdrv_button_get_pressed()) == !press) {
        MICROPY_EVENT_POLL_HOOK;
    }
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

    // Disable normal stop behavior since we need the buttons for the menu.
    // Except if the Bluetooth button is used for stopping, since we don't need
    // it for the menu.
    if (stop_button != PBIO_BUTTON_RIGHT_UP) {
        pbsys_program_stop_set_buttons(0);
    }

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
    #if PYBRICKS_PY_TOOLS_APP_DATA
    { MP_ROM_QSTR(MP_QSTR_AppData),  MP_ROM_PTR(&pb_type_app_data)               },
    #endif // PYBRICKS_PY_TOOLS_APP_DATA
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

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES

MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_tools, pb_module_tools);

// backwards compatibility for pybricks.geometry
#if MICROPY_PY_BUILTINS_FLOAT
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_geometry, pb_module_tools);
#endif // MICROPY_PY_BUILTINS_FLOAT

#endif // !MICROPY_MODULE_BUILTIN_SUBPACKAGES

#endif // PYBRICKS_PY_TOOLS
