/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David Lechner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define PYBRICKS_BRICK_MOVEHUB
#define MICROPY_HW_BOARD_NAME           "BOOST Move Hub"
#define MICROPY_HW_MCU_NAME             "STM32F070RB"

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.
// Requires about 19K (19568) of flash
#define MICROPY_ENABLE_COMPILER         (1)

// Set to MICROPY_FLOAT_IMPL_FLOAT to enable floating point support in user code or
// set to MICROPY_FLOAT_IMPL_NONE to disable floating point support in user code
// Requires about 20K (21312) of flash
#define MICROPY_FLOAT_IMPL              (MICROPY_FLOAT_IMPL_NONE)

// Set to (1) to include the advanced module or (0) to exclude it. Requires about ... bytes of flash
// This module includes the IODevice class for setting device modes and reading/writing raw data
#define PYBRICKS_MODULE_ADVANCED        (1)

// Set to (1) to enable user access to GPIO and ADC. Set to (0) to disable
// Requires about 360 bytes of flash. PYBRICKS_MODULE_ADVANCED must be set
// for this option to take effect.
#define PYBRICKS_ENABLE_HARDWARE_DEBUG  (0)

extern const struct _mp_obj_module_t pb_module_movehub;
extern const struct _mp_obj_module_t pb_module_pupdevices;
extern const struct _mp_obj_module_t pb_module_timing;
extern const struct _mp_obj_module_t pb_module_robotics;

#if PYBRICKS_MODULE_ADVANCED
extern const struct _mp_obj_module_t pb_module_advanced;
#define PYBRICKS_MODULE_ADVANCED_DEF { MP_OBJ_NEW_QSTR(MP_QSTR_advanced),    (mp_obj_t)&pb_module_advanced },
#else
#define PYBRICKS_MODULE_ADVANCED_DEF
#endif

#define PYBRICKS_PORT_BUILTIN_MODULES \
    PYBRICKS_MODULE_ADVANCED_DEF \
    { MP_OBJ_NEW_QSTR(MP_QSTR_movehub),     (mp_obj_t)&pb_module_movehub },    \
    { MP_OBJ_NEW_QSTR(MP_QSTR_devices),     (mp_obj_t)&pb_module_pupdevices },  \
    { MP_OBJ_NEW_QSTR(MP_QSTR_timing),      (mp_obj_t)&pb_module_timing     },  \
    { MP_OBJ_NEW_QSTR(MP_QSTR_robotics),    (mp_obj_t)&pb_module_robotics },
