#include <pbio/port.h>
#include <pbio/button.h>

#include "pbobj.h"
#include "extmod/utime_mphal.h"
#include "modcommon.h"

/* Port enum */

STATIC const mp_rom_map_elem_t motor_Port_enum_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_A),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_A) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_B),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_B) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_C),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_C) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_D) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_S1),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_1) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_S2),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_2) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_S3),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_3) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_S4),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_4) },
};
STATIC PB_DEFINE_CONST_ENUM(motor_Port_enum, motor_Port_enum_table);

/* Button enum */

STATIC const mp_rom_map_elem_t ev3brick_button_enum_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_up),     MP_OBJ_NEW_SMALL_INT(PBIO_BUTTON_UP)     },
    { MP_OBJ_NEW_QSTR(MP_QSTR_down),   MP_OBJ_NEW_SMALL_INT(PBIO_BUTTON_DOWN)   },
    { MP_OBJ_NEW_QSTR(MP_QSTR_left),   MP_OBJ_NEW_SMALL_INT(PBIO_BUTTON_LEFT)   },
    { MP_OBJ_NEW_QSTR(MP_QSTR_right),  MP_OBJ_NEW_SMALL_INT(PBIO_BUTTON_RIGHT)  },
    { MP_OBJ_NEW_QSTR(MP_QSTR_center), MP_OBJ_NEW_SMALL_INT(PBIO_BUTTON_CENTER) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_back),   MP_OBJ_NEW_SMALL_INT(PBIO_BUTTON_STOP)   },
};
PB_DEFINE_CONST_ENUM(ev3brick_button_enum, ev3brick_button_enum_table);

/* Return a list of pressed buttons (EV3/ev3dev can detect only one button at a time). */

STATIC mp_obj_t ev3brick_buttons(void) {
    pbio_button_flags_t pressed = 0;
    pbdrv_button_is_pressed(PBIO_PORT_SELF, &pressed);
    mp_obj_t button_list[1];
    button_list[0] = mp_obj_new_int(pressed);
    return mp_obj_new_list(pressed ? 1 : 0, button_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(ev3brick_buttons_obj, ev3brick_buttons);

/* Module table */

STATIC const mp_map_elem_t ev3brick_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_ev3brick_c)   },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Port), (mp_obj_t)&motor_Port_enum },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Color), (mp_obj_t)&pb_Color_enum },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Button), (mp_obj_t)&ev3brick_button_enum },
    { MP_ROM_QSTR(MP_QSTR_wait), (mp_obj_t)&mp_utime_sleep_ms_obj },
    { MP_ROM_QSTR(MP_QSTR_buttons), (mp_obj_t)&ev3brick_buttons_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_light), (mp_obj_t)&hub_set_light_obj },
};
 STATIC MP_DEFINE_CONST_DICT (pb_module_ev3brick_globals, ev3brick_globals_table);
 const mp_obj_module_t pb_module_ev3brick = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3brick_globals,
};
