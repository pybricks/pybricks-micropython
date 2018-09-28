#define PYBRICKS_BRICK_MOVEHUB
#define MICROPY_HW_BOARD_NAME           "BOOST Movehub"
#define MICROPY_HW_MCU_NAME             "STM32F070RB"

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.
// Requires about 19K (19568) of flash
#define MICROPY_ENABLE_COMPILER         (0)

// Set to (1) to enable motors
#define PYBRICKS_HW_ENABLE_MOTORS       (1)

extern const struct _mp_obj_module_t mp_module_hub;

#define PYBRICKS_PORT_BUILTIN_MODULES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_hub), (mp_obj_t)&mp_module_hub },
