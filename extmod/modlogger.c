// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include <pbio/logger.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include "modlogger.h"

#include "pbthread.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

// pybricks.tools.Logger class object
typedef struct _tools_Logger_obj_t {
    mp_obj_base_t base;
    pbio_log_t *log;
} tools_Logger_obj_t;

STATIC mp_obj_t tools_Logger_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        tools_Logger_obj_t, self,
        PB_ARG_REQUIRED(duration)
    );

    mp_int_t duration_arg = pb_obj_get_int(duration);

    pbio_error_t err;

    pb_thread_enter();
    err = pbio_logger_start(self->log, duration_arg);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_Logger_start_obj, 0, tools_Logger_start);

STATIC mp_obj_t tools_Logger_get(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        tools_Logger_obj_t, self,
        PB_ARG_DEFAULT_NONE(index)
    );

    mp_int_t index_val = pb_obj_get_default_int(index, -1);

    // Data buffer for this sample
    mp_obj_t ret[MAX_LOG_VALUES];
    int32_t data[MAX_LOG_VALUES];

    uint8_t num_values;
    pbio_error_t err;

    // Get data for this sample
    pb_thread_enter();
    err = pbio_logger_read(self->log, index_val, data);
    num_values = self->log->num_values;
    pb_thread_exit();
    pb_assert(err);

    // Convert data to user objects
    for (uint8_t i = 0; i < self->log->num_values; i++) {
        ret[i] = mp_obj_new_int(data[i]);
    }
    return mp_obj_new_tuple(num_values, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_Logger_get_obj, 0, tools_Logger_get);

STATIC mp_obj_t tools_Logger_stop(mp_obj_t self_in) {
    tools_Logger_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pbio_error_t err;

    pb_thread_enter();
    err = pbio_logger_stop(self->log);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(tools_Logger_stop_obj, tools_Logger_stop);

static const size_t max_val_strln = sizeof("−2147483648,");

// Make a comma separated list of values
static void make_data_row_str(char *row, int32_t *data, uint8_t n) {

    // Set initial row to empty string so we can concat to its
    row[0] = 0;

    for (uint8_t v = 0; v < n; v++) {
        // Convert value to string
        if (snprintf(&row[strlen(row)], max_val_strln, "%" PRId32, data[v]) < 0) {
            pb_assert(PBIO_ERROR_IO);
        }

        // Concatenate line break or comma separator
        if (snprintf(&row[strlen(row)], 2, "%s", v == n-1 ? "\n" : ",") < 0) {
            pb_assert(PBIO_ERROR_IO);
        }
    }
}

STATIC mp_obj_t tools_Logger_save(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

#if PYBRICKS_HUB_EV3
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        tools_Logger_obj_t, self,
        PB_ARG_DEFAULT_NONE(path)
    );

    // Create an empty log file
    const char *file_path = path == mp_const_none ? "log.txt" : mp_obj_str_get_str(path);
    FILE *log_file;

    // Open file to erase it
    log_file = fopen(file_path, "w");
    if (log_file == NULL) {
        pb_assert(PBIO_ERROR_IO);
    }
    if (fclose(log_file) != 0) {
        pb_assert(PBIO_ERROR_IO);
    }
    // Open file for appending
    log_file = fopen(file_path, "a");
    if (log_file == NULL) {
        pb_assert(PBIO_ERROR_IO);
    }
#else
    tools_Logger_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
#endif //PYBRICKS_HUB_EV3

    // Read log size information
    int32_t data[MAX_LOG_VALUES];
    pb_thread_enter();
    uint8_t num_values = self->log->num_values;
    int32_t samples =  self->log->sampled;
    pb_thread_exit();

    // Allocate space for one row of data
    char row_str[max_val_strln*MAX_LOG_VALUES+1];

    pbio_error_t err = PBIO_SUCCESS;

    // Write data to file line by line
    for (int32_t i = 0; i < samples; i++) {

        // Read one line inside lock
        pb_thread_enter();
        err = pbio_logger_read(self->log, i, data);
        pb_thread_exit();

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
#endif // PYBRICKS_HUB_EV3

    pb_assert(err);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_Logger_save_obj, 0, tools_Logger_save);

STATIC mp_obj_t tools_Logger_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    tools_Logger_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_LEN:
            pb_thread_enter();
            uint32_t len = self->log->sampled;
            pb_thread_exit();
            return MP_OBJ_NEW_SMALL_INT(len);
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
    .locals_dict = (mp_obj_dict_t*)&tools_Logger_locals_dict,
    .unary_op = tools_Logger_unary_op,
};

mp_obj_t logger_obj_make_new(pbio_log_t *log) {
    // Create new light instance
    tools_Logger_obj_t *logger = m_new_obj(tools_Logger_obj_t);
    // Set type and iodev
    logger->base.type = (mp_obj_type_t*) &tools_Logger_type;
    logger->log = log;
    return logger;
}
