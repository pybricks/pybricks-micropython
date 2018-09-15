#include "pbinit.h"

#define PYBRICKS_BRICK_EV3
#define PYBRICKS_BRICK_NAME             "LEGO MINDSTORMS EV3 Intelligent Brick"

#define MICROPY_PORT_INIT_FUNC pybricks_init()
#define MICROPY_PORT_DEINIT_FUNC pybricks_deinit()

extern const struct _mp_obj_module_t mp_module_constants;
extern const struct _mp_obj_module_t mp_module_ev3devices;

#define PYBRICKS_PORT_BUILTIN_MODULES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_ev3devices_c), (mp_obj_t)&mp_module_ev3devices }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR__constants), (mp_obj_t)&mp_module_constants },
