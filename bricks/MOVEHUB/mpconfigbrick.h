#define PYBRICKS_BRICK_MOVEHUB
#define PYBRICKS_BRICK_NAME             "BOOST Move Hub"


// Set to (1) to enable the compiler and interactive micropython prompt. Requires about 19K (19568) of flash
#define PYBRICKS_ENABLE_REPL            (1)

// Set to (1) to enable basic motor functionality such as setting the duty cycle, braking, and coasting.
#define PYBRICKS_HW_ENABLE_MOTORS       (1)

// Set to (1) to enable motor encoder functionality and speed control. Has an effect only if PYBRICKS_HW_ENABLE_MOTORS is enabled.
#define PYBRICKS_HW_ENABLE_ENCODERS     (0)

extern const struct _mp_obj_module_t mp_module_hub;
extern const struct _mp_obj_module_t mp_module_constants;

#define PYBRICKS_PORT_BUILTIN_MODULES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_hub), (mp_obj_t)&mp_module_hub },  \
    { MP_OBJ_NEW_QSTR(MP_QSTR__constants), (mp_obj_t)&mp_module_constants },  
