// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include "py/mpconfig.h"

#if PYBRICKS_PY_DEBUG

#include <pbdrv/adc.h>
#include <pbdrv/button.h>

#include "py/obj.h"

#include "pberror.h"


STATIC mp_obj_t debug_read_adc(mp_obj_t ch) {
    uint16_t value;

    pb_assert(pbdrv_adc_get_ch(mp_obj_get_int(ch), &value));

    return mp_obj_new_int(value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(debug_read_adc_obj, debug_read_adc);

STATIC mp_obj_t debug_read_buttons() {
    pbio_button_flags_t flags;

    pb_assert(pbdrv_button_is_pressed(&flags));

    return mp_obj_new_int(flags);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(debug_read_buttons_obj, debug_read_buttons);

STATIC const mp_rom_map_elem_t debug_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_ROM_QSTR(MP_QSTR_debug)          },
    { MP_ROM_QSTR(MP_QSTR_read_adc),        MP_ROM_PTR(&debug_read_adc_obj)     },
    { MP_ROM_QSTR(MP_QSTR_read_buttons),    MP_ROM_PTR(&debug_read_buttons_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_debug_globals, debug_globals_table);

const mp_obj_module_t pb_module_debug = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_debug_globals,
};

#endif //PYBRICKS_PY_DEBUG
