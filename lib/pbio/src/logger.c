// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_LOGGER

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <pbdrv/clock.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/logger.h>

/**
 * Starts logging in the background.
 *
 * @param [in]  log         Pointer to log.
 * @param [in]  buf         Array large enough to hold @p num_rows rows of data.
 * @param [in]  num_rows    Maximum number of rows that can be logged.
 * @param [in]  num_cols    Number of entries in one row.
 * @param [in]  down_sample For every @p down_sample of update calls, only one row is logged.
 */
void pbio_logger_start(pbio_log_t *log, int32_t *buf, uint32_t num_rows, uint32_t num_cols, int32_t down_sample) {
    // (re-)initialize logger status.
    log->num_rows_used = 0;
    log->skipped_samples = 0;
    log->data = buf;
    log->num_rows = num_rows;
    log->num_cols = num_cols;
    log->down_sample = down_sample;
    log->start_time = pbdrv_clock_get_ms();

    // Data may now be logged.
    log->active = true;
}

/**
 * Stops accepting new data from background loops.
 *
 * @param [in]  log         Pointer to log.
 */
void pbio_logger_stop(pbio_log_t *log) {
    log->active = false;
}

/**
 * Add new data from a background loop.
 *
 * @param [in]  log         Pointer to log.
 * @param [in]  row_data    Data to be added.
 */
void pbio_logger_update(pbio_log_t *log, int32_t *row_data) {

    // Log nothing if logger is inactive.
    if (!log->active) {
        return;
    }

    // Skip logging if we are not yet at a multiple of down_sample.
    if (++log->skipped_samples != log->down_sample) {
        return;
    }
    log->skipped_samples = 0;

    // Exit if log is full.
    if (log->num_rows_used >= log->num_rows) {
        log->active = false;
        return;
    }

    // Write time of logging.
    log->data[log->num_rows_used * log->num_cols] = pbdrv_clock_get_ms() - log->start_time;

    // Write the data.
    for (uint8_t i = PBIO_LOGGER_NUM_DEFAULT_COLS; i < log->num_cols; i++) {
        log->data[log->num_rows_used * log->num_cols + i] = row_data[i - PBIO_LOGGER_NUM_DEFAULT_COLS];
    }

    // Increment used row counter.
    log->num_rows_used++;

    return;
}

/**
 * Gets number of used (filled) rows in the log.
 *
 * @param [in]  log         Pointer to log.
 * @return                  Number of used rows.
 */
uint32_t pbio_logger_get_num_rows_used(pbio_log_t *log) {
    return log->num_rows_used;
}

/**
 * Gets row from the log. Caller must ensure that valid index is used.
 *
 * @param [in]  log         Pointer to log.
 * @return                  Pointer to row data.
 */
int32_t *pbio_logger_get_row_data(pbio_log_t *log, uint32_t index) {
    return log->data + index * log->num_cols;
}

#endif // PBIO_CONFIG_LOGGER
