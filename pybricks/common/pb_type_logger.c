// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_LOGGER

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbio/config.h>
#include <pbio/logger.h>
#include <pbio/int_math.h>
#include <pbio/servo.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

/**
 * pybricks.tools.Logger class object
 */
typedef struct _tools_Logger_obj_t {
    mp_obj_base_t base;
    /**
     * Reference to pbio log object, which does not have its own data buffer.
     */
    pbio_log_t *log;
    /**
     * Data buffer that will be allocated on MicroPython heap.
     */
    int32_t *buf;
    /**
     * Number of columns, needed when starting log which happens after object creation.
     */
    uint32_t num_cols;
    /**
     * Buffer size. Used to free (renew) old data when resetting logger.
     */
    uint32_t last_size;
} tools_Logger_obj_t;

STATIC mp_obj_t tools_Logger_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        tools_Logger_obj_t, self,
        PB_ARG_REQUIRED(duration),
        PB_ARG_DEFAULT_INT(down_sample, 1));

    // Log only one row per divisor samples.
    mp_uint_t down_sample = pbio_int_math_max(pb_obj_get_int(down_sample_in), 1);
    mp_uint_t num_rows = pb_obj_get_int(duration_in) / PBIO_CONFIG_CONTROL_LOOP_TIME_MS / down_sample;

    // Size is number of rows times column width. All data are int32.
    mp_int_t size = num_rows * self->num_cols;
    self->buf = m_renew(int32_t, self->buf, self->last_size, size);
    self->last_size = size;

    // Indicates that background control loops may enter data in log.
    pbio_logger_start(self->log, self->buf, num_rows, self->num_cols, down_sample);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_Logger_start_obj, 1, tools_Logger_start);

STATIC mp_obj_t tools_Logger_stop(mp_obj_t self_in) {
    tools_Logger_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Indicates that background control loops log write more data.
    pbio_logger_stop(self->log);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(tools_Logger_stop_obj, tools_Logger_stop);

STATIC mp_obj_t tools_Logger_save(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        tools_Logger_obj_t, self,
        PB_ARG_DEFAULT_NONE(path));

    // Don't allow any more data to be added to logs.
    pbio_logger_stop(self->log);

    // Get log file path.
    const char *path = path_in == mp_const_none ? "log.txt" : mp_obj_str_get_str(path_in);

    #if PYBRICKS_PY_COMMON_LOGGER_REAL_FILE
    // Create an empty log file locally.
    FILE *log_file = fopen(path, "w");
    if (log_file == NULL) {
        pb_assert(PBIO_ERROR_IO);
    }
    #else

    // Tell IDE to open remote file.
    mp_printf(&mp_plat_print, "PB_OF:%s\n", path);
    #endif // PYBRICKS_PY_COMMON_LOGGER_REAL_FILE

    pbio_error_t err = PBIO_SUCCESS;

    // Write data to file line by line
    for (uint32_t row = 0; row < pbio_logger_get_num_rows_used(self->log); row++) {

        int32_t *row_data = pbio_logger_get_row_data(self->log, row);

        for (uint32_t col = 0; col < self->log->num_cols; col++) {

            // Write "-12345, " or "-12345\n" for last value on row.
            const char *format = col + 1 < self->log->num_cols ? "%d, " : "%d\n";

            // Write one value.
            #if PYBRICKS_PY_COMMON_LOGGER_REAL_FILE
            if (fprintf(log_file, format, row_data[col]) < 0) {
                break;
            }
            #else
            mp_printf(&mp_plat_print, format, row_data[col]);
            #endif // PYBRICKS_PY_COMMON_LOGGER_REAL_FILE
        }

        // Writing data can take a while, so give system some time too.
        MICROPY_VM_HOOK_LOOP
        mp_handle_pending(true);
    }

    #if PYBRICKS_PY_COMMON_LOGGER_REAL_FILE
    // Close the file
    if (fclose(log_file) != 0) {
        err = PBIO_ERROR_IO;
    }
    #else
    mp_print_str(&mp_plat_print, "PB_EOF\n");
    #endif // PYBRICKS_PY_COMMON_LOGGER_REAL_FILE

    pb_assert(err);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_Logger_save_obj, 1, tools_Logger_save);

// dir(pybricks.tools.Logger)
STATIC const mp_rom_map_elem_t tools_Logger_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&tools_Logger_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&tools_Logger_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_save), MP_ROM_PTR(&tools_Logger_save_obj) },
};
STATIC MP_DEFINE_CONST_DICT(tools_Logger_locals_dict, tools_Logger_locals_dict_table);

// type(pybricks.tools.Logger)
STATIC const mp_obj_type_t tools_Logger_type = {
    { &mp_type_type },
    .locals_dict = (mp_obj_dict_t *)&tools_Logger_locals_dict,
};

mp_obj_t common_Logger_obj_make_new(pbio_log_t *log, uint8_t num_values) {
    tools_Logger_obj_t *logger = m_new_obj(tools_Logger_obj_t);
    logger->base.type = (mp_obj_type_t *)&tools_Logger_type;
    logger->log = log;
    logger->num_cols = num_values + PBIO_LOGGER_NUM_DEFAULT_COLS;
    return logger;
}

#endif // PYBRICKS_PY_COMMON_LOGGER
