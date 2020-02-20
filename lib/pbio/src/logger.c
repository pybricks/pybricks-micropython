// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <contiki.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/logger.h>

static void pbio_logger_delete(pbio_log_t *log) {
    // Free log if any
    if (log->len > 0) {
        free(log->data);
    }
    log->calls = 0;
    log->len = 0;
    log->active = false;
}

pbio_error_t pbio_logger_start(pbio_log_t *log, int32_t duration) {
    // Free any existing log
    pbio_logger_delete(log);

    // FIXME: Make as configurable setting
    log->sample_div = 1;

    // Minimal log length
    uint32_t len = duration / PBIO_CONFIG_SERVO_PERIOD_MS / log->sample_div;

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
    log->start = clock_usecs();
    log->active = true;
    return PBIO_SUCCESS;
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

    // Skip logging unless we are at a multiple of subsample_div
    // and increment call counter.
    if (log->calls++ % log->sample_div != 0) {
        return PBIO_SUCCESS;
    }

    // Number of samples already logged so far
    int32_t sampled = log->calls / log->sample_div;

    // Raise error if log is full, which should not happen
    if (sampled > log->len) {
        log->active = false;
        return PBIO_ERROR_FAILED;
    }

    // Stop successfully when done
    if (sampled == log->len) {
        log->active = false;
        return PBIO_SUCCESS;
    }

    // Write time of logging
    log->data[sampled*log->num_values] = (clock_usecs() - log->start)/1000;

    // Write the data
    for (uint8_t i = NUM_DEFAULT_LOG_VALUES; i < log->num_values; i++) {
        log->data[sampled*log->num_values + i] = buf[i-NUM_DEFAULT_LOG_VALUES];
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_logger_read(pbio_log_t *log, int32_t sindex, int32_t *buf) {

    // Validate index value
    if (sindex < -1) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Number of samples already logged so far
    int32_t sampled = log->calls / log->sample_div;

    // Get index or latest sample if requested index is -1
    uint32_t index = sindex == -1 ? sampled - 1 : sindex;

    // Ensure index is within bounds
    if (index >= sampled) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Read the data
    for (uint8_t i = 0; i < log->num_values; i++) {
        buf[i] = log->data[index*log->num_values + i];
    }

    return PBIO_SUCCESS;
}
