// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_NXTBRICK

#include <pbio/util.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

#include "py/misc.h"
#include "py/obj.h"
#include "py/runtime.h"

typedef struct _hubs_NXTBrick_obj_t {
    mp_obj_base_t base;
    mp_obj_t battery;
    mp_obj_t buttons;
    mp_obj_t speaker;
    mp_obj_t system;
} hubs_NXTBrick_obj_t;

static mp_obj_t pb_type_nxtbrick_button_pressed(void) {
    pbio_button_flags_t flags;
    pb_assert(pbio_button_is_pressed(&flags));
    mp_obj_t pressed[4];
    size_t num = 0;
    if (flags & PBIO_BUTTON_LEFT) {
        pressed[num++] = pb_type_button_new(MP_QSTR_LEFT);
    }
    if (flags & PBIO_BUTTON_RIGHT) {
        pressed[num++] = pb_type_button_new(MP_QSTR_RIGHT);
    }
    if (flags & PBIO_BUTTON_CENTER) {
        pressed[num++] = pb_type_button_new(MP_QSTR_CENTER);
    }
    if (flags & PBIO_BUTTON_DOWN) {
        pressed[num++] = pb_type_button_new(MP_QSTR_DOWN);
    }
    return mp_obj_new_set(num, pressed);
}

static mp_obj_t hubs_NXTBrick_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_NXTBrick_obj_t *self = mp_obj_malloc(hubs_NXTBrick_obj_t, type);
    self->battery = MP_OBJ_FROM_PTR(&pb_module_battery);
    self->buttons = pb_type_Keypad_obj_new(pb_type_nxtbrick_button_pressed);
    self->speaker = mp_call_function_0(MP_OBJ_FROM_PTR(&pb_type_Speaker));
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);
    return MP_OBJ_FROM_PTR(self);
}

static const pb_attr_dict_entry_t hubs_NXTBrick_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_battery, hubs_NXTBrick_obj_t, battery),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_buttons, hubs_NXTBrick_obj_t, buttons),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_speaker, hubs_NXTBrick_obj_t, speaker),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_NXTBrick_obj_t, system),
    PB_ATTR_DICT_SENTINEL
};

MP_DEFINE_CONST_OBJ_TYPE(pb_type_ThisHub,
    PYBRICKS_HUB_CLASS_NAME,
    MP_TYPE_FLAG_NONE,
    make_new, hubs_NXTBrick_make_new,
    attr, pb_attribute_handler,
    protocol, hubs_NXTBrick_attr_dict);

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_NXTBRICK
