// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS_HOSTBUFFER

#include <string.h>

#include <pbsys/command.h>

#include "py/mphal.h"

#include <pybricks/tools.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

typedef struct _pb_type_hostbuffer_obj_t {
    mp_obj_base_t base;
    uint32_t size;
    uint8_t buffer[];
} pb_type_hostbuffer_obj_t;

// pointer to dynamically allocated buffer_obj_singleton for driver callback.
static pb_type_hostbuffer_obj_t *buffer_obj_singleton;

static void handle_write_data_buffer(uint16_t offset, uint32_t size, const uint8_t *data) {
    // Can't write if buffer does not exist or isn't big enough.
    if (!buffer_obj_singleton || offset + size > buffer_obj_singleton->size) {
        return;
    }
    memcpy(buffer_obj_singleton->buffer + offset, data, size);
}

STATIC mp_obj_t pb_type_hostbuffer_get_data(mp_obj_t self_in) {
    pb_type_hostbuffer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // REVISIT
    // - offset and size args to read only what's needed
    // - (python-level?) wrapper API using ustruct
    return mp_obj_new_bytes(self->buffer, self->size);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_hostbuffer_get_data_obj, pb_type_hostbuffer_get_data);

STATIC mp_obj_t pb_type_hostbuffer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(size));

    // Can only create one instance for now.
    if (buffer_obj_singleton) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("host buffer already allocated"));
    }

    size_t size = mp_obj_get_int(size_in);

    // Use finalizer so we can deactivate the data callback when buffer is garbage collected.
    buffer_obj_singleton = m_new_obj_var_with_finaliser(pb_type_hostbuffer_obj_t, uint8_t, size);
    buffer_obj_singleton->base.type = type;
    buffer_obj_singleton->size = size;

    pbsys_command_set_write_program_data_buffer_callback(handle_write_data_buffer);

    return MP_OBJ_FROM_PTR(buffer_obj_singleton);
}

mp_obj_t pb_type_hostbuffer_close(mp_obj_t stream) {
    if (buffer_obj_singleton) {
        pbsys_command_set_write_program_data_buffer_callback(NULL);
        buffer_obj_singleton = NULL;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_hostbuffer_close_obj, pb_type_hostbuffer_close);

STATIC const mp_rom_map_elem_t pb_type_hostbuffer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__),         MP_ROM_PTR(&pb_type_hostbuffer_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_close),           MP_ROM_PTR(&pb_type_hostbuffer_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_data),        MP_ROM_PTR(&pb_type_hostbuffer_get_data_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_hostbuffer_locals_dict, pb_type_hostbuffer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_hostbuffer,
    MP_QSTR_hostbuffer,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_hostbuffer_make_new,
    locals_dict, &pb_type_hostbuffer_locals_dict);

#endif // PYBRICKS_PY_TOOLS_HOSTBUFFER
