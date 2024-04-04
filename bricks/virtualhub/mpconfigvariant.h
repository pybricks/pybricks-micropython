// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

#define PYBRICKS_HUB_CLASS_NAME         (MP_QSTR_VirtualHub)

#define PYBRICKS_HUB_NAME               "virtualhub"
#define PYBRICKS_HUB_VIRTUALHUB         (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON              (1)
#define PYBRICKS_PY_COMMON_BLE          (0)
#define PYBRICKS_PY_COMMON_CHARGER      (1)
#define PYBRICKS_PY_COMMON_COLOR_LIGHT  (1)
#define PYBRICKS_PY_COMMON_CONTROL      (1)
#define PYBRICKS_PY_COMMON_IMU          (0)
#define PYBRICKS_PY_COMMON_KEYPAD       (1)
#define PYBRICKS_PY_COMMON_KEYPAD_HUB_BUTTONS (1)
#define PYBRICKS_PY_COMMON_LIGHT_ARRAY  (1)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX (0)
#define PYBRICKS_PY_COMMON_LOGGER       (1)
#define PYBRICKS_PY_COMMON_LOGGER_REAL_FILE (1)
#define PYBRICKS_PY_COMMON_MOTORS       (1)
#define PYBRICKS_PY_COMMON_SPEAKER      (0)
#define PYBRICKS_PY_COMMON_SYSTEM       (1)
#define PYBRICKS_PY_EV3DEVICES          (0)
#define PYBRICKS_PY_EXPERIMENTAL        (1)
#define PYBRICKS_PY_HUBS                (1)
#define PYBRICKS_PY_IODEVICES           (1)
#define PYBRICKS_PY_MEDIA               (1)
#define PYBRICKS_PY_MEDIA_EV3DEV        (0)
#define PYBRICKS_PY_NXTDEVICES          (0)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON   (1)
#define PYBRICKS_PY_PARAMETERS_ICON     (1)
#define PYBRICKS_PY_PUPDEVICES          (1)
#define PYBRICKS_PY_PUPDEVICES_REMOTE   (0)
#define PYBRICKS_PY_DEVICES             (1)
#define PYBRICKS_PY_ROBOTICS            (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE (0)
#define PYBRICKS_PY_TOOLS               (1)
#define PYBRICKS_PY_TOOLS_HUB_MENU      (0)

// Pybricks options
#define PYBRICKS_OPT_COMPILER                   (1)
#define PYBRICKS_OPT_FLOAT                      (1)
#define PYBRICKS_OPT_TERSE_ERR                  (0)
#define PYBRICKS_OPT_EXTRA_MOD                  (1)
#define PYBRICKS_OPT_CUSTOM_IMPORT              (1)

// Upstream MicroPython options
#define MICROPY_FLOAT_IMPL                      (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_LONGINT_IMPL                    (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_DEBUG_PRINTERS                  (1)
#define MICROPY_MODULE_ATTR_DELEGATION          (1)
#define MICROPY_MODULE_BUILTIN_INIT             (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO         (1)
#define MICROPY_PY_BUILTINS_HELP                (1)
#define MICROPY_PY_BUILTINS_HELP_MODULES        (1)
#define MICROPY_PY_SYS_SETTRACE                 (1)
#define MICROPY_PY_UERRNO                       (1)
#define MICROPY_PY_UOS                          (1)
#define MICROPY_PY_UOS_GETENV_PUTENV_UNSETENV   (1)
#define MICROPY_PY_UOS_INCLUDEFILE              "ports/unix/moduos.c"
#define MICROPY_PY_URANDOM_EXTRA_FUNCS          (1)
#define MICROPY_PY_BUILTINS_SLICE_INDICES       (1)
#define MICROPY_PERSISTENT_CODE_SAVE            (1)
#define MICROPY_STREAMS_POSIX_API               (1)
#define MICROPY_HELPER_REPL                     (1)
#define MICROPY_KBD_EXCEPTION                   (1)

// REVISIT: This list currently matches the stm32 builds. We may consider
// adding more like the ev3dev build.
#define MICROPY_PY_UERRNO_LIST \
    X(EPERM) \
    X(EIO) \
    X(EBUSY) \
    X(ENODEV) \
    X(EINVAL) \
    X(EOPNOTSUPP) \
    X(EAGAIN) \
    X(ETIMEDOUT) \
    X(ECANCELED) \

#define MICROPY_MPHALPORT_H "mpvarianthal.h"
#define MICROPY_VARIANT_QSTR_DEFS_H "../_common/qstrdefs.h"

#define MICROPY_PORT_INIT_FUNC do { \
        extern void pb_virtualhub_port_init(void); \
        pb_virtualhub_port_init(); \
} while (0)

#define MICROPY_PORT_DEINIT_FUNC do { \
        extern void pb_virtualhub_port_deinit(void); \
        pb_virtualhub_port_deinit(); \
} while (0)

#define MICROPY_VM_HOOK_LOOP do { \
        extern void pb_virtualhub_poll(void); \
        pb_virtualhub_poll(); \
} while (0);

#define MICROPY_GC_HOOK_LOOP(i) do { \
        if ((i & 0xf) == 0) { \
            MICROPY_VM_HOOK_LOOP \
        } \
} while (0)

#define MICROPY_EVENT_POLL_HOOK do { \
        extern void pb_virtualhub_event_poll(void); \
        pb_virtualhub_event_poll(); \
} while (0);
