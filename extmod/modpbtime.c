#include "py/obj.h"
#include "extmod/utime_mphal.h"

STATIC const mp_rom_map_elem_t pb_time_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_time) },
    { MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&mp_utime_sleep_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&mp_utime_ticks_ms_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pb_time_globals, pb_time_globals_table);

const mp_obj_module_t pb_module_time = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_time_globals,
};
