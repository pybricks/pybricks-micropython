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
    log->data = malloc(len * sizeof(pbio_log_data_t));
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

pbio_error_t pbio_logger_read(pbio_log_t *log, int32_t sindex, uint8_t *len, int32_t *buf) {

    // Validate index value
    if (sindex < -1) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Get index or latest sample if requested index is -1
    uint32_t index = sindex == -1 ? log->sampled - 1 : sindex;

    // Servo specific data to be returned
    *len = 3; // FIXME: Move to servo setup in log config
    buf[0] = (log->data[index].time - log->start)/1000;
    buf[1] = log->data[index].count;
    buf[2] = log->data[index].rate;

    return PBIO_SUCCESS;
}
