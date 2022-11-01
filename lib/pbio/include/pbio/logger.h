// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup Logger pbio/logger: Logging control loop data
 *
 * Log servo and control data to analyze and debug motor performance.
 * @{
 */

#ifndef _PBIO_LOGGER_H_
#define _PBIO_LOGGER_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>


/**
 * Logger object for storing data from background control loops.
 */
typedef struct _pbio_log_t {
    #if PBIO_CONFIG_LOGGER
    /**
     * Whether data should be logged.
     */
    bool active;
    /**
     * Number of columns.
     */
    uint8_t num_cols;
    /**
     * Number of rows.
     */
    uint32_t num_rows;
    /**
     * How many rows have been used (filled) so far.
     */
    uint32_t num_rows_used;
    /**
     * Data buffer allocated by external application.
     */
    uint32_t start_time;
    /**
     * Data buffer allocated by external application.
     */
    int32_t *data;
    /**
     * For each down_sample calls to log update, log only one row.
     */
    uint32_t down_sample;
    /**
     * How many rows have been skipped so far, counts up to down_sample.
     */
    uint32_t skipped_samples;
    #endif
} pbio_log_t;


#if PBIO_CONFIG_LOGGER

// Number of values logged by the logger itself, such as time of call to logger
#define PBIO_LOGGER_NUM_DEFAULT_COLS (1)

void pbio_logger_start(pbio_log_t *log, int32_t *buf, uint32_t num_rows, uint32_t num_cols, int32_t down_sample);
void pbio_logger_stop(pbio_log_t *log);
bool pbio_logger_is_active(pbio_log_t *log);
void pbio_logger_add_row(pbio_log_t *log, int32_t *row_data);

uint32_t pbio_logger_get_num_rows_used(pbio_log_t *log);
int32_t *pbio_logger_get_row_data(pbio_log_t *log, uint32_t index);

#else

static inline void pbio_logger_start(pbio_log_t *log, int32_t *buf, uint32_t num_rows, uint32_t num_cols, int32_t down_sample) {
}
static inline void pbio_logger_stop(pbio_log_t *log) {
}
static inline bool pbio_logger_is_active(pbio_log_t *log) {
    return false;
}
static inline void pbio_logger_add_row(pbio_log_t *log, int32_t *row_data) {
}
static inline uint32_t pbio_logger_get_num_rows_used(pbio_log_t *log) {
    return 0;
}
static inline int32_t *pbio_logger_get_row_data(pbio_log_t *log, uint32_t index) {
    return NULL;
}

#endif // PBIO_CONFIG_LOGGER

#endif // _PBIO_LOGGER_H_

/** @} */
