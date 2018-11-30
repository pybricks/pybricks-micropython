/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Laurens Valk
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

#include "pbinit.h"

#define PYBRICKS_BRICK_EV3
#define MICROPY_HW_BOARD_NAME             "LEGO MINDSTORMS EV3 Intelligent Brick"
#define MICROPY_HW_MCU_NAME               "Texas Instruments AM1808"

#define MICROPY_PORT_INIT_FUNC pybricks_init()
#define MICROPY_PORT_DEINIT_FUNC pybricks_deinit()

#define MICROPY_PY_SYS_PATH_DEFAULT (":~/.micropython/lib:/usr/lib/micropython")

extern const struct _mp_obj_module_t pb_module_ev3brick;
extern const struct _mp_obj_module_t pb_module_ev3devices;
extern const struct _mp_obj_module_t pb_module_timing;
extern const struct _mp_obj_module_t pb_module_robotics;

#define PYBRICKS_PORT_BUILTIN_MODULES \
    { MP_ROM_QSTR(MP_QSTR_ev3devices_c), MP_ROM_PTR(&pb_module_ev3devices) }, \
    { MP_ROM_QSTR(MP_QSTR_ev3brick_c),   MP_ROM_PTR(&pb_module_ev3brick)   }, \
    { MP_ROM_QSTR(MP_QSTR_timing),       MP_ROM_PTR(&pb_module_timing)     }, \
    { MP_ROM_QSTR(MP_QSTR_robotics),     MP_ROM_PTR(&pb_module_robotics)   },
