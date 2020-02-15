// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <glib.h>

#include <pbdrv/config.h>
#include "pbinit.h"

#define MICROPY_HW_BOARD_NAME             "LEGO MINDSTORMS EV3 Intelligent Brick"
#define MICROPY_HW_MCU_NAME               "Texas Instruments AM1808"

#define PYBRICKS_HUB_EV3                (1)

// Pybricks modules
#define PYBRICKS_PY_BUTTONS             (1)
#define PYBRICKS_PY_EV3DEVICES          (1)
#define PYBRICKS_PY_EXPERIMENTAL        (1)
#define PYBRICKS_PY_IODEVICES           (1)
#define PYBRICKS_PY_MEDIA_EV3DEV        (1)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PUPDEVICES          (0)
#define PYBRICKS_PY_ROBOTICS            (1)
#define PYBRICKS_PY_USIGNAL             (1)

#define MICROPY_PORT_INIT_FUNC pybricks_init()
#define MICROPY_PORT_DEINIT_FUNC pybricks_deinit()
#define MICROPY_MPHALPORT_H "ev3dev_mphal.h"
#define MICROPY_VM_HOOK_LOOP do { \
    extern int pbio_do_one_event(void); \
    pbio_do_one_event(); \
} while (0);
#define MICROPY_EVENT_POLL_HOOK do { \
    extern void mp_handle_pending(void); \
    mp_handle_pending(); \
    extern int pbio_do_one_event(void); \
    while (pbio_do_one_event()) { } \
    MP_THREAD_GIL_EXIT(); \
    g_main_context_iteration(g_main_context_get_thread_default(), TRUE); \
    MP_THREAD_GIL_ENTER(); \
} while (0);

#define MICROPY_PY_SYS_PATH_DEFAULT (":~/.pybricks-micropython/lib:/usr/lib/pybricks-micropython")

extern const struct _mp_obj_module_t pb_module_bluetooth;
extern const struct _mp_obj_module_t pb_module_ev3devices;
extern const struct _mp_obj_module_t pb_module_experimental;
extern const struct _mp_obj_module_t pb_module_hubs;
extern const struct _mp_obj_module_t pb_module_iodevices;
extern const struct _mp_obj_module_t pb_module_media_ev3dev;
extern const struct _mp_obj_module_t pb_module_nxtdevices;
extern const struct _mp_obj_module_t pb_module_parameters;
extern const struct _mp_obj_module_t pb_module_robotics;
extern const struct _mp_obj_module_t pb_module_tools;
extern const struct _mp_obj_module_t pb_module_usignal;

#define PYBRICKS_PORT_BUILTIN_MODULES \
    { MP_ROM_QSTR(MP_QSTR_bluetooth_c),     MP_ROM_PTR(&pb_module_bluetooth)        }, \
    { MP_ROM_QSTR(MP_QSTR_ev3devices_c),    MP_ROM_PTR(&pb_module_ev3devices)       }, \
    { MP_ROM_QSTR(MP_QSTR_experimental_c),  MP_ROM_PTR(&pb_module_experimental)     }, \
    { MP_ROM_QSTR(MP_QSTR_hubs_c),          MP_ROM_PTR(&pb_module_hubs)             }, \
    { MP_ROM_QSTR(MP_QSTR_iodevices_c),     MP_ROM_PTR(&pb_module_iodevices)        }, \
    { MP_ROM_QSTR(MP_QSTR_media_ev3dev_c),  MP_ROM_PTR(&pb_module_media_ev3dev)     }, \
    { MP_ROM_QSTR(MP_QSTR_nxtdevices_c),    MP_ROM_PTR(&pb_module_nxtdevices)       }, \
    { MP_ROM_QSTR(MP_QSTR_parameters_c),    MP_ROM_PTR(&pb_module_parameters)       }, \
    { MP_ROM_QSTR(MP_QSTR_robotics_c),      MP_ROM_PTR(&pb_module_robotics)         }, \
    { MP_ROM_QSTR(MP_QSTR_tools),           MP_ROM_PTR(&pb_module_tools)            }, \
    { MP_ROM_QSTR(MP_QSTR_usignal),         MP_ROM_PTR(&pb_module_usignal)          },

#define PBYRICKS_PORT_BUILTINS
