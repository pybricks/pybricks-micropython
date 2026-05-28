// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_MESSAGING_APP_DATA

#include <string.h>

#include <pbsys/command.h>
#include <pbsys/host.h>

#include "py/mphal.h"
#include "py/objstr.h"

#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_async.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

typedef struct {
    uint8_t mode;
    size_t offset;
    size_t size;
} pb_type_app_data_mode_info_t;

typedef struct _pb_type_app_data_obj_t {
    mp_obj_base_t base;
    pb_type_async_t *tx_iter;
    size_t rx_len;
    size_t num_modes;
    pb_type_app_data_mode_info_t *modes;
    uint8_t rx_buffer[] __attribute__((aligned(4)));
} pb_type_app_data_obj_t;


// pointer to dynamically allocated app_data singleton for driver callback.
static pb_type_app_data_obj_t *app_data_instance;

static pbio_error_t handle_incoming_app_data(uint16_t offset, uint32_t size, const uint8_t *data) {
    // Can't write if rx_buffer does not exist or isn't big enough.
    if (!app_data_instance || offset + size > app_data_instance->rx_len) {
        return PBIO_ERROR_INVALID_ARG;
    }
    memcpy(app_data_instance->rx_buffer + offset, data, size);
    return PBIO_SUCCESS;
}

static pb_type_app_data_mode_info_t *get_mode_info(pb_type_app_data_obj_t *self, mp_obj_t mode_in) {
    size_t mode = mp_obj_get_int(mode_in);
    for (size_t i = 0; i < self->num_modes; i++) {
        if (self->modes[i].mode == mode) {
            return &self->modes[i];
        }
    }
    return NULL;
}

static mp_obj_t pb_type_app_data_get_bytes(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_app_data_obj_t, self,
        PB_ARG_REQUIRED(mode),
        PB_ARG_DEFAULT_NONE(index));

    pb_type_app_data_mode_info_t *mode_info = get_mode_info(self, mode_in);
    if (!mode_info) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid mode"));
    }

    // No index, return all bytes for the mode.
    if (index_in == mp_const_none) {
        return mp_obj_new_bytes(&self->rx_buffer[mode_info->offset], mode_info->size);
    }

    // Index given, return single byte as integer.
    size_t index = mp_obj_get_int(index_in);
    if (index >= mode_info->size) {
        mp_raise_ValueError(MP_ERROR_TEXT("index out of range"));
    }
    return mp_obj_new_int(self->rx_buffer[mode_info->offset + index]);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_app_data_get_bytes_obj, 1, pb_type_app_data_get_bytes);

static pbio_error_t app_data_write_bytes_iterate_once(pbio_os_state_t *state, mp_obj_t parent_obj) {
    // No need to pass in buffered arguments since they were copied on the
    // inital run. We can just keep calling this until completion.
    return pbsys_host_send_event(state, PBIO_PYBRICKS_EVENT_WRITE_APP_DATA, NULL, 0);
}

static mp_obj_t pb_type_app_data_write_bytes(mp_obj_t self_in, mp_obj_t data_in) {

    // The first call will copy given data (or raise errors) so we don't need
    // to buffer things here.
    size_t size;
    const uint8_t *data = (const uint8_t *)mp_obj_str_get_data(data_in, &size);
    pbio_os_state_t state = 0;
    pbio_error_t err = pbsys_host_send_event(&state, PBIO_PYBRICKS_EVENT_WRITE_APP_DATA, data, size);

    // Expect yield after the initial call.
    if (err == PBIO_SUCCESS) {
        pb_assert(PBIO_ERROR_FAILED);
    } else if (err != PBIO_ERROR_AGAIN) {
        pb_assert(err);
    }

    pb_type_async_t config = {
        .parent_obj = self_in,
        .iter_once = app_data_write_bytes_iterate_once,
        .state = state,
    };

    pb_type_app_data_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return pb_type_async_wait_or_await(&config, &self->tx_iter, true);
}
static MP_DEFINE_CONST_FUN_OBJ_2(pb_type_app_data_write_bytes_obj, pb_type_app_data_write_bytes);

static mp_obj_t pb_type_app_data_configure(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_app_data_obj_t, self,
        PB_ARG_REQUIRED(mode),
        PB_ARG_REQUIRED(parameter),
        PB_ARG_REQUIRED(value));

    size_t size;
    const uint8_t *value = (const uint8_t *)mp_obj_str_get_data(value_in, &size);

    // Create new bytes object with mode and size prepended.
    vstr_t vstr;
    vstr_init_len(&vstr, 3 + size);
    vstr.buf[0] = 0x01;
    vstr.buf[1] = mp_obj_get_int(mode_in);
    vstr.buf[2] = mp_obj_get_int(parameter_in);
    memcpy(vstr.buf + 3, value, size);
    return pb_type_app_data_write_bytes(MP_OBJ_FROM_PTR(self), mp_obj_new_bytes_from_vstr(&vstr));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_app_data_configure_obj, 1, pb_type_app_data_configure);


static mp_obj_t pb_type_app_data_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(modes));

    // Can only create one instance during initialization.
    pb_module_tools_assert_blocking();
    if (app_data_instance) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("host rx_buffer already allocated"));
    }

    // Unpack modes to find out required allocation size.
    size_t alloc_size = 0;
    size_t num_modes;
    mp_obj_t *mode_items;
    mp_obj_get_array(modes_in, &num_modes, &mode_items);

    // Sort by mode number. Tuples compare lexicographically, so this sorts by the first element.
    pb_assert_type(modes_in, &mp_type_list);
    mp_obj_t sorted = mp_obj_new_list(num_modes, mode_items);
    mp_obj_list_sort(1, &sorted, (mp_map_t *)&mp_const_empty_map);
    mp_obj_get_array(sorted, &num_modes, &mode_items);

    // Prepare mode setup command.
    vstr_t mode_vstr;
    vstr_init_len(&mode_vstr, 1 + num_modes);
    mode_vstr.buf[0] = 0x00;

    pb_type_app_data_mode_info_t *modes = m_new(pb_type_app_data_mode_info_t, num_modes);

    for (size_t i = 0; i < num_modes; i++) {

        // Expect each item to be a (mode, size) tuple.
        mp_obj_t item = mode_items[i];
        size_t pair_size;
        mp_obj_t *pair_items;
        mp_obj_get_array(item, &pair_size, &pair_items);
        if (pair_size != 2 || mp_obj_get_int(pair_items[0]) > 255) {
            mp_raise_TypeError(MP_ERROR_TEXT("modes must be a list of (mode, size) tuples"));
        }

        // Store mode info for easy reading/writing later.
        pb_type_app_data_mode_info_t *mode_info = &modes[i];
        mode_info->mode = mp_obj_get_int(pair_items[0]);
        mode_info->offset = alloc_size;
        mode_info->size = mp_obj_get_int(pair_items[1]);

        // Mode must be unique
        for (size_t j = 0; j < i; j++) {
            if (mode_info->mode == modes[j].mode) {
                mp_raise_ValueError(MP_ERROR_TEXT("mode numbers must be unique"));
            }
        }

        // Mode command is just a concatenation of mode numbers.
        mode_vstr.buf[i + 1] = mode_info->mode;

        // Total allocation size.
        alloc_size += mode_info->size;
    }

    // Use finalizer so we can deactivate the data callback when rx_buffer is garbage collected.
    app_data_instance = mp_obj_malloc_var_with_finaliser(pb_type_app_data_obj_t, uint8_t, alloc_size, type);
    app_data_instance->rx_len = alloc_size;
    app_data_instance->num_modes = num_modes;
    app_data_instance->modes = modes;

    // Activate callback now that we have allocated the rx_buffer.
    pbsys_command_set_write_app_data_callback(handle_incoming_app_data);

    app_data_instance->tx_iter = NULL;

    // Send mode setup command to host.
    pb_type_app_data_write_bytes(MP_OBJ_FROM_PTR(app_data_instance), mp_obj_new_bytes_from_vstr(&mode_vstr));

    return MP_OBJ_FROM_PTR(app_data_instance);
}

mp_obj_t pb_type_app_data_close(mp_obj_t stream) {
    if (app_data_instance) {
        pbsys_command_set_write_app_data_callback(NULL);
        app_data_instance = NULL;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_app_data_close_obj, pb_type_app_data_close);

static const mp_rom_map_elem_t pb_type_app_data_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__),      MP_ROM_PTR(&pb_type_app_data_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_close),        MP_ROM_PTR(&pb_type_app_data_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_bytes),    MP_ROM_PTR(&pb_type_app_data_get_bytes_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_bytes),  MP_ROM_PTR(&pb_type_app_data_write_bytes_obj) },
    { MP_ROM_QSTR(MP_QSTR_configure),    MP_ROM_PTR(&pb_type_app_data_configure_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_app_data_locals_dict, pb_type_app_data_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_app_data,
    MP_QSTR_AppData,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_app_data_make_new,
    locals_dict, &pb_type_app_data_locals_dict);

#endif // PYBRICKS_PY_MESSAGING_APP_DATA
