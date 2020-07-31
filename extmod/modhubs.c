// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbsys/sys.h>

#include <pbio/port.h>
#include <pbio/button.h>

#include "modparameters.h"

#include "common/common.h"

#include "util/pberror.h"
#include "util/pbobj.h"

#if PYBRICKS_HUB_EV3

#include "pb_ev3dev_types.h"

// Class structure for EV3Brick
typedef struct _hubs_EV3Brick_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    mp_obj_t screen;
    mp_obj_t speaker;
} hubs_EV3Brick_obj_t;

STATIC mp_obj_t hubs_EV3Brick_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_EV3Brick_obj_t *self = m_new_obj(hubs_EV3Brick_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_obj_t screen_args[] = { MP_ROM_QSTR(MP_QSTR__screen_) };
    self->screen = pb_type_ev3dev_Image.make_new(&pb_type_ev3dev_Image, 1, 0, screen_args);
    self->speaker = pb_type_ev3dev_Speaker.make_new(&pb_type_ev3dev_Speaker, 0, 0, NULL);

    // Create an instance of the Light class, representing the brick status light
    self->light = common_ColorLight_obj_make_new(NULL);

    return MP_OBJ_FROM_PTR(self);
}

/*
EV3Brick class tables
*/
STATIC const mp_rom_map_elem_t hubs_EV3Brick_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_buttons),     MP_ROM_PTR(&pb_module_buttons)      },
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)      },
    { MP_ROM_QSTR(MP_QSTR_screen),      MP_ROM_ATTRIBUTE_OFFSET(hubs_EV3Brick_obj_t, screen)  },
    { MP_ROM_QSTR(MP_QSTR_speaker),     MP_ROM_ATTRIBUTE_OFFSET(hubs_EV3Brick_obj_t, speaker) },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(hubs_EV3Brick_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(hubs_EV3Brick_locals_dict, hubs_EV3Brick_locals_dict_table);

STATIC const mp_obj_type_t hubs_EV3Brick_type = {
    { &mp_type_type },
    .name = MP_QSTR_EV3Brick,
    .make_new = hubs_EV3Brick_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_EV3Brick_locals_dict,
};

#endif // PYBRICKS_HUB_EV3

#if (PYBRICKS_HUB_MOVEHUB || PYBRICKS_HUB_CITYHUB || PYBRICKS_HUB_CPLUSHUB || PYBRICKS_HUB_PRIMEHUB)

// pybricks._common.HubClass.shutdown
STATIC mp_obj_t hubs_HubClass_shutdown(mp_obj_t self_in) {
    pbsys_power_off();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(hub_shutdown_obj, hubs_HubClass_shutdown);

// pybricks._common.HubClass.reboot
STATIC mp_obj_t hubs_HubClass_reboot(mp_obj_t self_in) {
    pbsys_reboot(0);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(hub_reboot_obj, hubs_HubClass_reboot);

// pybricks._common.HubClass.update
STATIC mp_obj_t hubs_HubClass_update(mp_obj_t self_in) {
    pbsys_reboot(1);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(hub_update_obj, hubs_HubClass_update);

#endif

#if PYBRICKS_HUB_MOVEHUB

// Class structure for MoveHub
typedef struct _hubs_MoveHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
} hubs_MoveHub_obj_t;

STATIC mp_obj_t hubs_MoveHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_MoveHub_obj_t *self = m_new_obj(hubs_MoveHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    // Create an instance of the Light class, representing the hub light
    self->light = common_ColorLight_obj_make_new(NULL);

    return MP_OBJ_FROM_PTR(self);
}

/*
MoveHub class tables
*/
STATIC const mp_rom_map_elem_t hubs_MoveHub_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
    { MP_ROM_QSTR(MP_QSTR_shutdown),    MP_ROM_PTR(&hub_shutdown_obj)     },
    { MP_ROM_QSTR(MP_QSTR_reboot),      MP_ROM_PTR(&hub_reboot_obj)       },
    { MP_ROM_QSTR(MP_QSTR_update),      MP_ROM_PTR(&hub_update_obj)       },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(hubs_MoveHub_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(hubs_MoveHub_locals_dict, hubs_MoveHub_locals_dict_table);

STATIC const mp_obj_type_t hubs_MoveHub_type = {
    { &mp_type_type },
    .name = MP_QSTR_MoveHub,
    .make_new = hubs_MoveHub_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_MoveHub_locals_dict,
};

#endif // PYBRICKS_HUB_MOVEHUB

#if PYBRICKS_HUB_CITYHUB

// Class structure for CityHub
typedef struct _hubs_CityHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
} hubs_CityHub_obj_t;

STATIC mp_obj_t hubs_CityHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_CityHub_obj_t *self = m_new_obj(hubs_CityHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    // Create an instance of the Light class, representing the hub light
    self->light = common_ColorLight_obj_make_new(NULL);

    return MP_OBJ_FROM_PTR(self);
}

/*
CityHub class tables
*/
STATIC const mp_rom_map_elem_t hubs_CityHub_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
    { MP_ROM_QSTR(MP_QSTR_shutdown),    MP_ROM_PTR(&hub_shutdown_obj)     },
    { MP_ROM_QSTR(MP_QSTR_reboot),      MP_ROM_PTR(&hub_reboot_obj)       },
    { MP_ROM_QSTR(MP_QSTR_update),      MP_ROM_PTR(&hub_update_obj)       },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(hubs_CityHub_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(hubs_CityHub_locals_dict, hubs_CityHub_locals_dict_table);

STATIC const mp_obj_type_t hubs_CityHub_type = {
    { &mp_type_type },
    .name = MP_QSTR_CityHub,
    .make_new = hubs_CityHub_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_CityHub_locals_dict,
};

#endif // PYBRICKS_HUB_CITYHUB

#if PYBRICKS_HUB_CPLUSHUB

// Class structure for CPlusHub
typedef struct _hubs_CPlusHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
} hubs_CPlusHub_obj_t;

STATIC mp_obj_t hubs_CPlusHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_CPlusHub_obj_t *self = m_new_obj(hubs_CPlusHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    // Create an instance of the Light class, representing the hub light
    self->light = common_ColorLight_obj_make_new(NULL);

    return MP_OBJ_FROM_PTR(self);
}

/*
CPlusHub class tables
*/
STATIC const mp_rom_map_elem_t hubs_CPlusHub_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
    { MP_ROM_QSTR(MP_QSTR_shutdown),    MP_ROM_PTR(&hub_shutdown_obj)     },
    { MP_ROM_QSTR(MP_QSTR_reboot),      MP_ROM_PTR(&hub_reboot_obj)       },
    { MP_ROM_QSTR(MP_QSTR_update),      MP_ROM_PTR(&hub_update_obj)       },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(hubs_CPlusHub_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(hubs_CPlusHub_locals_dict, hubs_CPlusHub_locals_dict_table);

STATIC const mp_obj_type_t hubs_CPlusHub_type = {
    { &mp_type_type },
    .name = MP_QSTR_CPlusHub,
    .make_new = hubs_CPlusHub_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_CPlusHub_locals_dict,
};

#endif // PYBRICKS_HUB_CPLUSHUB

#ifdef PBDRV_CONFIG_HUB_PRIMEHUB

// Class structure for PrimeHub
typedef struct _hubs_PrimeHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
} hubs_PrimeHub_obj_t;

STATIC void hubs_PrimeHub_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, qstr_str(MP_QSTR_PrimeHub));
}

STATIC mp_obj_t hubs_PrimeHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_PrimeHub_obj_t *self = m_new_obj(hubs_PrimeHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    // Create an instance of the Light class, representing the hub light
    self->light = common_ColorLight_obj_make_new(NULL);

    return MP_OBJ_FROM_PTR(self);
}

/*
PrimeHub class tables
*/
STATIC const mp_rom_map_elem_t hubs_PrimeHub_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_Port),        MP_ROM_PTR(&pb_enum_type_Port)    },
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
    { MP_ROM_QSTR(MP_QSTR_shutdown),    MP_ROM_PTR(&hub_shutdown_obj)     },
    { MP_ROM_QSTR(MP_QSTR_reboot),      MP_ROM_PTR(&hub_reboot_obj)       },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(hubs_PrimeHub_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(hubs_PrimeHub_locals_dict, hubs_PrimeHub_locals_dict_table);

STATIC const mp_obj_type_t hubs_PrimeHub_type = {
    { &mp_type_type },
    .name = MP_QSTR_PrimeHub,
    .print = hubs_PrimeHub_print,
    .make_new = hubs_PrimeHub_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_PrimeHub_locals_dict,
};

#endif // PBDRV_CONFIG_HUB_PRIMEHUB

#if PYBRICKS_HUB_NXT

// Class structure for NXTBrick
typedef struct _hubs_NXTBrick_obj_t {
    mp_obj_base_t base;
} hubs_NXTBrick_obj_t;

STATIC mp_obj_t hubs_NXTBrick_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_NXTBrick_obj_t *self = m_new_obj(hubs_NXTBrick_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    return MP_OBJ_FROM_PTR(self);
}

/*
NXTBrick class tables
*/
STATIC const mp_rom_map_elem_t hubs_NXTBrick_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
};
STATIC MP_DEFINE_CONST_DICT(hubs_NXTBrick_locals_dict, hubs_NXTBrick_locals_dict_table);

STATIC const mp_obj_type_t hubs_NXTBrick_type = {
    { &mp_type_type },
    .name = MP_QSTR_NXTBrick,
    .make_new = hubs_NXTBrick_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_NXTBrick_locals_dict,
};

#endif // PYBRICKS_HUB_NXT

/* Module table */

STATIC const mp_rom_map_elem_t hubs_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_hubs)      },
    #if PYBRICKS_HUB_MOVEHUB
    { MP_ROM_QSTR(MP_QSTR_MoveHub),    MP_ROM_PTR(&hubs_MoveHub_type) },
    #endif
    #if PYBRICKS_HUB_CITYHUB
    { MP_ROM_QSTR(MP_QSTR_CityHub),    MP_ROM_PTR(&hubs_CityHub_type) },
    #endif
    #if PYBRICKS_HUB_CPLUSHUB
    { MP_ROM_QSTR(MP_QSTR_CPlusHub),    MP_ROM_PTR(&hubs_CPlusHub_type) },
    #endif
    #ifdef PBDRV_CONFIG_HUB_PRIMEHUB
    { MP_ROM_QSTR(MP_QSTR_PrimeHub),    MP_ROM_PTR(&hubs_PrimeHub_type) },
    #endif
    #if PYBRICKS_HUB_NXT
    { MP_ROM_QSTR(MP_QSTR_NXTBrick),    MP_ROM_PTR(&hubs_NXTBrick_type) },
    #endif
    #if PYBRICKS_HUB_EV3
    { MP_ROM_QSTR(MP_QSTR_EV3Brick),    MP_ROM_PTR(&hubs_EV3Brick_type) },
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_module_hubs_globals, hubs_globals_table);

const mp_obj_module_t pb_module_hubs = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_hubs_globals,
};
