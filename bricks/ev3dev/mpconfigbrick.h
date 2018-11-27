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
    { MP_OBJ_NEW_QSTR(MP_QSTR_ev3devices_c), (mp_obj_t)&pb_module_ev3devices }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_ev3brick_c),   (mp_obj_t)&pb_module_ev3brick   }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_timing),       (mp_obj_t)&pb_module_timing     }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_robotics),     (mp_obj_t)&pb_module_robotics   },
