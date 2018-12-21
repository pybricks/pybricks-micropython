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


#include <pbio/light.h>
#include <pbio/button.h>
#include <pbio/motorcontrol.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "pbobj.h"
#include "pberror.h"

/* Brick/Hub Port enum */

STATIC const mp_rom_map_elem_t pb_Port_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_A),   MP_ROM_INT(PBIO_PORT_A) },
    { MP_ROM_QSTR(MP_QSTR_B),   MP_ROM_INT(PBIO_PORT_B) },
    { MP_ROM_QSTR(MP_QSTR_C),   MP_ROM_INT(PBIO_PORT_C) },
    { MP_ROM_QSTR(MP_QSTR_D),   MP_ROM_INT(PBIO_PORT_D) },
#if defined(PYBRICKS_BRICK_EV3)
    { MP_ROM_QSTR(MP_QSTR_S1),  MP_ROM_INT(PBIO_PORT_1) },
    { MP_ROM_QSTR(MP_QSTR_S2),  MP_ROM_INT(PBIO_PORT_2) },
    { MP_ROM_QSTR(MP_QSTR_S3),  MP_ROM_INT(PBIO_PORT_3) },
    { MP_ROM_QSTR(MP_QSTR_S4),  MP_ROM_INT(PBIO_PORT_4) },
#endif //PYBRICKS_BRICK_EV3
};
STATIC PB_DEFINE_CONST_ENUM(pb_Port_enum, pb_Port_enum_table);


/* Motor stop enum */

STATIC const mp_rom_map_elem_t motor_Stop_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_coast),       MP_ROM_INT(PBIO_MOTOR_STOP_COAST)   },
    { MP_ROM_QSTR(MP_QSTR_brake),       MP_ROM_INT(PBIO_MOTOR_STOP_BRAKE)   },
    { MP_ROM_QSTR(MP_QSTR_hold),        MP_ROM_INT(PBIO_MOTOR_STOP_HOLD)    },
};
PB_DEFINE_CONST_ENUM(motor_Stop_enum, motor_Stop_enum_table);

/* Motor direction enum */

STATIC const mp_rom_map_elem_t motor_Direction_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_normal),      MP_ROM_INT(PBIO_MOTOR_DIR_NORMAL)   },
    { MP_ROM_QSTR(MP_QSTR_inverted),    MP_ROM_INT(PBIO_MOTOR_DIR_INVERTED) },
};
PB_DEFINE_CONST_ENUM(motor_Direction_enum, motor_Direction_enum_table);

/* Motor completion type enum */

STATIC const mp_rom_map_elem_t motor_Completion_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_foreground),  MP_ROM_INT(PBIO_MOTOR_RUN_FOREGROUND)   },
    { MP_ROM_QSTR(MP_QSTR_background),  MP_ROM_INT(PBIO_MOTOR_RUN_BACKGROUND)   },
};
PB_DEFINE_CONST_ENUM(motor_Completion_enum, motor_Completion_enum_table);


/* Color enum */

STATIC const mp_rom_map_elem_t pb_Color_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_black),   MP_ROM_INT(PBIO_LIGHT_COLOR_BLACK)  },
    { MP_ROM_QSTR(MP_QSTR_purple),  MP_ROM_INT(PBIO_LIGHT_COLOR_PURPLE) },
    { MP_ROM_QSTR(MP_QSTR_blue),    MP_ROM_INT(PBIO_LIGHT_COLOR_BLUE)   },
    { MP_ROM_QSTR(MP_QSTR_green),   MP_ROM_INT(PBIO_LIGHT_COLOR_GREEN)  },
    { MP_ROM_QSTR(MP_QSTR_yellow),  MP_ROM_INT(PBIO_LIGHT_COLOR_YELLOW) },
    { MP_ROM_QSTR(MP_QSTR_orange),  MP_ROM_INT(PBIO_LIGHT_COLOR_ORANGE) },
    { MP_ROM_QSTR(MP_QSTR_red),     MP_ROM_INT(PBIO_LIGHT_COLOR_RED)    },
    { MP_ROM_QSTR(MP_QSTR_white),   MP_ROM_INT(PBIO_LIGHT_COLOR_WHITE)  },
    { MP_ROM_QSTR(MP_QSTR_brown),   MP_ROM_INT(PBIO_LIGHT_COLOR_BROWN)  },
};
PB_DEFINE_CONST_ENUM(pb_Color_enum, pb_Color_enum_table);

/* Generic button enum */

STATIC const mp_rom_map_elem_t pb_Button_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_up),          MP_ROM_INT(PBIO_BUTTON_UP)         },
    { MP_ROM_QSTR(MP_QSTR_down),        MP_ROM_INT(PBIO_BUTTON_DOWN)       },
    { MP_ROM_QSTR(MP_QSTR_left),        MP_ROM_INT(PBIO_BUTTON_LEFT)       },
    { MP_ROM_QSTR(MP_QSTR_right),       MP_ROM_INT(PBIO_BUTTON_RIGHT)      },
    { MP_ROM_QSTR(MP_QSTR_center),      MP_ROM_INT(PBIO_BUTTON_CENTER)     },
    { MP_ROM_QSTR(MP_QSTR_left_up),     MP_ROM_INT(PBIO_BUTTON_LEFT_UP)    },
    { MP_ROM_QSTR(MP_QSTR_left_down),   MP_ROM_INT(PBIO_BUTTON_LEFT_DOWN)  },
    { MP_ROM_QSTR(MP_QSTR_right_up),    MP_ROM_INT(PBIO_BUTTON_RIGHT_UP)   },
    { MP_ROM_QSTR(MP_QSTR_right_down),  MP_ROM_INT(PBIO_BUTTON_RIGHT_DOWN) },
#if defined(PYBRICKS_BRICK_EV3)
    { MP_ROM_QSTR(MP_QSTR_beacon),      MP_ROM_INT(PBIO_BUTTON_UP)         },
#endif //PYBRICKS_BRICK_EV3
};
PB_DEFINE_CONST_ENUM(pb_Button_enum, pb_Button_enum_table);


#if defined(PYBRICKS_BRICK_EV3)

#endif //PYBRICKS_BRICK_EV3

/*
parameters module tables
*/

STATIC const mp_rom_map_elem_t parameters_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_parameters)     },
    { MP_ROM_QSTR(MP_QSTR_Port),        MP_ROM_PTR(&pb_Port_enum)           },
    { MP_ROM_QSTR(MP_QSTR_Stop),        MP_ROM_PTR(&motor_Stop_enum)        },
    { MP_ROM_QSTR(MP_QSTR_Direction),   MP_ROM_PTR(&motor_Direction_enum)   },
    { MP_ROM_QSTR(MP_QSTR_Completion),  MP_ROM_PTR(&motor_Completion_enum)  },
    { MP_ROM_QSTR(MP_QSTR_Color),       MP_ROM_PTR(&pb_Color_enum)          },
    { MP_ROM_QSTR(MP_QSTR_Button),      MP_ROM_PTR(&pb_Button_enum)         },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_parameters_globals, parameters_globals_table);

const mp_obj_module_t pb_module_parameters = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_parameters_globals,
};
