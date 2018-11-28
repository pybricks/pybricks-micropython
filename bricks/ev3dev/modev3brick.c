#include <pbio/port.h>
#include <pbio/button.h>

#include "pberror.h"
#include "pbobj.h"
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

STATIC mp_obj_t ev3brick_buttons(void) {
    mp_obj_t button_list[6];
    pbio_button_flags_t pressed;
    uint8_t size = 0;

    pb_assert(pbio_button_is_pressed(PBIO_PORT_SELF, &pressed));

    if (pressed & PBIO_BUTTON_CENTER) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_CENTER);
    }
    if (pressed & PBIO_BUTTON_LEFT) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_LEFT);
    }
    if (pressed & PBIO_BUTTON_RIGHT) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_RIGHT);
    }
    if (pressed & PBIO_BUTTON_UP) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_UP);
    }
    if (pressed & PBIO_BUTTON_DOWN) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_DOWN);
    }
    if (pressed & PBIO_BUTTON_STOP) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_STOP);
    }

    return mp_obj_new_tuple(size, button_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(ev3brick_buttons_obj, ev3brick_buttons);

/* Module table */

STATIC const mp_map_elem_t ev3brick_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_ev3brick_c) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Port),    (mp_obj_t)&motor_Port_enum           },
    { MP_OBJ_NEW_QSTR(MP_QSTR_buttons), (mp_obj_t)&ev3brick_buttons_obj      },
    { MP_OBJ_NEW_QSTR(MP_QSTR_light),   (mp_obj_t)&hub_set_light_obj         },
};
 STATIC MP_DEFINE_CONST_DICT (pb_module_ev3brick_globals, ev3brick_globals_table);
 const mp_obj_module_t pb_module_ev3brick = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3brick_globals,
};
