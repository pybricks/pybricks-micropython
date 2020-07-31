// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_MOTORS

#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include <pbio/config.h>
#include <pbio/logger.h>
#include <pbio/servo.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include "util_pb/pb_error.h"
#include "util_mp/pb_obj_helper.h"
#include "util_mp/pb_kwarg_helper.h"

// pybricks.tools.Logger class object
typedef struct _tools_Logger_obj_t {
    mp_obj_base_t base;
    pbio_log_t *log;
    int32_t *buf;
    uint32_t size;
} tools_Logger_obj_t;

STATIC mp_obj_t tools_Logger_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        tools_Logger_obj_t, self,
        PB_ARG_REQUIRED(duration),
        PB_ARG_DEFAULT_INT(divisor, 1));

    mp_int_t div = pb_obj_get_int(divisor);
    div = max(div, 1);
    mp_int_t rows = pb_obj_get_int(duration) / PBIO_CONFIG_SERVO_PERIOD_MS / div;
    mp_int_t size = rows * pbio_logger_cols(self->log);
    self->buf = m_renew(int32_t, self->buf, self->size, size);
    self->size = size;

    pbio_logger_start(self->log, self->buf, rows, div);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_Logger_start_obj, 1, tools_Logger_start);

STATIC mp_obj_t tools_Logger_get(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        tools_Logger_obj_t, self,
        PB_ARG_DEFAULT_NONE(index));

    mp_int_t index_val = pb_obj_get_default_int(index, -1);

    // Data buffer for this sample
    mp_obj_t ret[MAX_LOG_VALUES];
    int32_t data[MAX_LOG_VALUES];

    // Get data for this sample
    pb_assert(pbio_logger_read(self->log, index_val, data));
    uint8_t num_values = pbio_logger_cols(self->log);

    // Convert data to user objects
    for (uint8_t i = 0; i < num_values; i++) {
        ret[i] = mp_obj_new_int(data[i]);
    }
    return mp_obj_new_tuple(num_values, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_Logger_get_obj, 1, tools_Logger_get);

STATIC mp_obj_t tools_Logger_stop(mp_obj_t self_in) {
    tools_Logger_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pbio_logger_stop(self->log);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(tools_Logger_stop_obj, tools_Logger_stop);

static const size_t max_val_strln = sizeof("−2147483648,");

// Make a comma separated list of values
static void make_data_row_str(char *row, int32_t *data, uint8_t n) {

    // Reset string index
    size_t idx = 0;

    for (uint8_t v = 0; v < n; v++) {
        // Concatenate value, to the row
        size_t s = snprintf(&row[idx], max_val_strln, "%" PRId32 ",", data[v]);
        if (s < 2) {
            pb_assert(PBIO_ERROR_FAILED);
        }
        idx += s;

        // For the last value, replace , by \n
        if (v == n - 1) {
            row[idx - 1] = '\n';
        }
    }
}

STATIC mp_obj_t tools_Logger_save(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        tools_Logger_obj_t, self,
        PB_ARG_DEFAULT_NONE(path));
    const char *file_path = path == mp_const_none ? "log.txt" : mp_obj_str_get_str(path);

    #if PYBRICKS_HUB_EV3
    // Create an empty log file
    FILE *log_file;

    // Open file to erase it
    log_file = fopen(file_path, "w");
    if (log_file == NULL) {
        pb_assert(PBIO_ERROR_IO);
    }
    #else
    mp_printf(&mp_plat_print, "PB_OF:%s\n", file_path);
    #endif // PYBRICKS_HUB_EV3

    // Read log size information
    int32_t data[MAX_LOG_VALUES];

    pbio_logger_stop(self->log);

    uint8_t num_values = pbio_logger_cols(self->log);
    int32_t sampled = pbio_logger_rows(self->log);
    pbio_error_t err;

    // Allocate space for one null-terminated row of data
    char row_str[max_val_strln * MAX_LOG_VALUES + 1];

    // Write data to file line by line
    for (int32_t i = 0; i < sampled; i++) {

        // Read one line inside lock
        err = pbio_logger_read(self->log, i, data);
        if (err != PBIO_SUCCESS) {
            break;
        }

        // Make one string of values
        make_data_row_str(row_str, data, num_values);

        #if PYBRICKS_HUB_EV3
        // Append the row to file
        if (fprintf(log_file, "%s", row_str) < 0) {
            err = PBIO_ERROR_IO;
            break;
        }
        #else
        // Print the row
        mp_print_str(&mp_plat_print, row_str);
        #endif // PYBRICKS_HUB_EV3
    }

    #if PYBRICKS_HUB_EV3
    // Close the file
    if (fclose(log_file) != 0) {
        err = PBIO_ERROR_IO;
    }
    #else
    mp_print_str(&mp_plat_print, "PB_EOF\n");
    #endif // PYBRICKS_HUB_EV3

    pb_assert(err);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_Logger_save_obj, 1, tools_Logger_save);

STATIC mp_obj_t tools_Logger_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    tools_Logger_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_LEN:
            return MP_OBJ_NEW_SMALL_INT(pbio_logger_rows(self->log));
        default:
            return MP_OBJ_NULL;
    }
}

// dir(pybricks.tools.Logger)
STATIC const mp_rom_map_elem_t tools_Logger_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&tools_Logger_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&tools_Logger_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&tools_Logger_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_save), MP_ROM_PTR(&tools_Logger_save_obj) },
};
STATIC MP_DEFINE_CONST_DICT(tools_Logger_locals_dict, tools_Logger_locals_dict_table);

// type(pybricks.tools.Logger)
STATIC const mp_obj_type_t tools_Logger_type = {
    { &mp_type_type },
    .locals_dict = (mp_obj_dict_t *)&tools_Logger_locals_dict,
    .unary_op = tools_Logger_unary_op,
};

mp_obj_t logger_obj_make_new(pbio_log_t *log) {
    // Create new light instance
    tools_Logger_obj_t *logger = m_new_obj(tools_Logger_obj_t);
    // Set type and iodev
    logger->base.type = (mp_obj_type_t *)&tools_Logger_type;
    logger->log = log;
    return logger;
}

#endif // PYBRICKS_PY_COMMON_MOTORS
