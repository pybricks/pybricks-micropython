// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#define PYBRICKS_HUB_CLASS_NAME         (MP_QSTR_VirtualHub)

#define PYBRICKS_HUB_NAME               "virtualhub"
#define PYBRICKS_HUB_VIRTUALHUB         (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON              (1)
#define PYBRICKS_PY_COMMON_CHARGER      (1)
#define PYBRICKS_PY_COMMON_CONTROL      (1)
#define PYBRICKS_PY_COMMON_IMU          (1)
#define PYBRICKS_PY_COMMON_KEYPAD       (1)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX (1)
#define PYBRICKS_PY_COMMON_LOGGER       (1)
#define PYBRICKS_PY_COMMON_LOGGER_REAL_FILE (1)
#define PYBRICKS_PY_COMMON_MOTORS       (1)
#define PYBRICKS_PY_COMMON_SPEAKER      (1)
#define PYBRICKS_PY_COMMON_SYSTEM       (1)
#define PYBRICKS_PY_EV3DEVICES          (0)
#define PYBRICKS_PY_EXPERIMENTAL        (1)
#define PYBRICKS_PY_GEOMETRY            (1)
#define PYBRICKS_PY_HUBS                (1)
#define PYBRICKS_PY_IODEVICES           (1)
#define PYBRICKS_PY_MEDIA               (1)
#define PYBRICKS_PY_MEDIA_EV3DEV        (0)
#define PYBRICKS_PY_NXTDEVICES          (0)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON   (1)
#define PYBRICKS_PY_PARAMETERS_ICON     (1)
#define PYBRICKS_PY_PUPDEVICES          (1)
#define PYBRICKS_PY_ROBOTICS            (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE (0)
#define PYBRICKS_PY_TOOLS               (1)

// Upstream MicroPython options
#define MICROPY_MODULE_ATTR_DELEGATION          (1)
#define MICROPY_MODULE_BUILTIN_INIT             (1)
#define MICROPY_PY_BUILTINS_HELP                (1)
#define MICROPY_PY_BUILTINS_HELP_MODULES        (1)
#define MICROPY_PY_SYS_SETTRACE                 (1)
#define MICROPY_PERSISTENT_CODE_SAVE            (1)
#define MICROPY_PY_URANDOM_EXTRA_FUNCS          (1)
#define MICROPY_PY_BUILTINS_SLICE_INDICES       (1)

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
#define MICROPY_VARIANT_QSTR_DEFS_H "../common/qstrdefs.h"

#define MICROPY_PORT_INIT_FUNC do { \
        extern void pb_virtualhub_port_init(void); \
        pb_virtualhub_port_init(); \
} while (0)

#define MICROPY_PORT_DEINIT_FUNC do { \
        extern void pb_virtualhub_port_deinit(void); \
        pb_virtualhub_port_deinit(); \
} while (0)

#define MICROPY_VARIANT_ROOT_POINTERS \
    mp_obj_dict_t *pb_type_Color_dict;

#define MICROPY_VM_HOOK_LOOP do { \
        extern void pb_virtualhub_poll(void); \
        pb_virtualhub_poll(); \
} while (0);

#define MICROPY_GC_HOOK_LOOP MICROPY_VM_HOOK_LOOP

#define MICROPY_EVENT_POLL_HOOK do { \
        extern void pb_virtualhub_event_poll(void); \
        pb_virtualhub_event_poll(); \
} while (0);
