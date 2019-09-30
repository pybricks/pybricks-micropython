// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <pbio/error.h>
#include <pbio/logger.h>

#include "sys/clock.h"

static void pbio_logger_delete(pbio_log_t *log) {
    // Free log if any
    if (log->len > 0) {
        free(log->data);
    }
    log->sampled = 0;
    log->len = 0;
    log->active = false;
}

static pbio_error_t pbio_logger_empty(pbio_log_t *log, int32_t time_now, uint32_t duration) {
    // Free any existing log
    pbio_logger_delete(log);

    // Minimal log length
    uint32_t len = duration / MIN_PERIOD;

    // Assert length is allowed
    if (len > MAX_LOG_LEN) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Allocate memory for the logs
    log->data = malloc(len * log->num_values * sizeof(int32_t));
    if (log->data == NULL) {
        return PBIO_ERROR_FAILED;
    }

    // (re-)initialize logger status for this servo
    log->len = len;
    log->start = time_now;
    log->active = true;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_logger_start(pbio_log_t *log, int32_t duration) {
    // Allocate memory for log
    return pbio_logger_empty(log, clock_usecs(), duration);
}

pbio_error_t pbio_logger_stop(pbio_log_t *log) {
    // Release the logger for re-use
    log->active = false;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_logger_update(pbio_log_t *log, int32_t *buf) {

    // Log nothing if logger is inactive
    if (!log->active) {
        return PBIO_SUCCESS;
    }

    // Raise error if log is full, which should not happen
    if (log->sampled > log->len) {
        log->active = false;
        return PBIO_ERROR_FAILED;
    }    

    // Stop successfully when done
    if (log->sampled == log->len) {
        log->active = false;
        return PBIO_SUCCESS;
    }

    // Write the data
    for (uint8_t i = 0; i < log->num_values; i++) {
        log->data[log->sampled*log->num_values + i] = buf[i];
    }

    // Increment number of written samples
    log->sampled++;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_logger_read(pbio_log_t *log, int32_t sindex, int32_t *buf) {

    // Validate index value
    if (sindex < -1) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Get index or latest sample if requested index is -1
    uint32_t index = sindex == -1 ? log->sampled - 1 : sindex;

    // Read the data
    for (uint8_t i = 0; i < log->num_values; i++) {
        buf[i] = log->data[index*log->num_values + i];
    }

    return PBIO_SUCCESS;
}
