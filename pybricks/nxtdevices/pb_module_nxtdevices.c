// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include "py/builtin.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/nxtdevices/nxtdevices.h>

static const mp_rom_map_elem_t nxtdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),          MP_ROM_QSTR(MP_QSTR_nxtdevices)                  },
    #if PYBRICKS_PY_COMMON_MOTORS
    { MP_ROM_QSTR(MP_QSTR_Motor),             MP_ROM_PTR(&pb_type_Motor)                       },
    #endif
    { MP_ROM_QSTR(MP_QSTR_TouchSensor),       MP_ROM_PTR(&pb_type_nxtdevices_TouchSensor)      },
    { MP_ROM_QSTR(MP_QSTR_LightSensor),       MP_ROM_PTR(&pb_type_nxtdevices_LightSensor)      },
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),       MP_ROM_PTR(&pb_type_nxtdevices_ColorSensor)      },
    #if PYBRICKS_PY_EV3DEVDEVICES
    { MP_ROM_QSTR(MP_QSTR_EnergyMeter),       MP_ROM_PTR(&pb_type_nxtdevices_EnergyMeter)      },
    { MP_ROM_QSTR(MP_QSTR_SoundSensor),       MP_ROM_PTR(&pb_type_nxtdevices_SoundSensor)      },
    { MP_ROM_QSTR(MP_QSTR_TemperatureSensor), MP_ROM_PTR(&pb_type_nxtdevices_TemperatureSensor)},
    #endif
};
static MP_DEFINE_CONST_DICT(pb_module_nxtdevices_globals, nxtdevices_globals_table);

const mp_obj_module_t pb_module_nxtdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_nxtdevices_globals,
};

void pb_module_nxtdevices_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    // Support only load.
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }

    // Load from the frozen _i2c_sensors module.
    const mp_obj_t import_args[] = { MP_OBJ_NEW_QSTR(MP_QSTR__i2c_sensors) };
    mp_obj_t module = mp_builtin___import__(MP_ARRAY_SIZE(import_args), import_args);
    mp_load_method_maybe(module, attr, dest);
}

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_nxtdevices, pb_module_nxtdevices);
#endif
MP_REGISTER_MODULE_DELEGATION(pb_module_nxtdevices, pb_module_nxtdevices_attr);

#endif // PYBRICKS_PY_NXTDEVICES
