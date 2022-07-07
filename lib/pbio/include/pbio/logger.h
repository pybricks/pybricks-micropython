// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup Logger pbio: Logging control loop data
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

typedef struct _pbio_log_t {
    #if PBIO_CONFIG_LOGGER
    bool active;
    uint32_t skipped;
    uint32_t sampled;
    uint32_t len;
    uint32_t start;
    uint8_t num_values;
    int32_t *data;
    uint32_t sample_div;
    #endif
} pbio_log_t;

#if PBIO_CONFIG_LOGGER

// Number of values logged by the logger itself, such as time of call to logger
#define NUM_DEFAULT_LOG_VALUES (1)

// Maximum number of values to be logged per sample
#define MAX_LOG_VALUES (NUM_DEFAULT_LOG_VALUES + 15)

void pbio_logger_start(pbio_log_t *log, int32_t *buf, uint32_t len, int32_t div);
pbio_error_t pbio_logger_read(pbio_log_t *log, int32_t sindex, int32_t *buf);
void pbio_logger_update(pbio_log_t *log, int32_t *buf);
int32_t pbio_logger_rows(pbio_log_t *log);
int32_t pbio_logger_cols(pbio_log_t *log);
void pbio_logger_stop(pbio_log_t *log);

#else

static inline void pbio_logger_start(pbio_log_t *log, int32_t *buf, uint32_t len, int32_t div) {
}
static inline pbio_error_t pbio_logger_read(pbio_log_t *log, int32_t sindex, int32_t *buf) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline void pbio_logger_update(pbio_log_t *log, int32_t *buf) {
}
static inline int32_t pbio_logger_rows(pbio_log_t *log) {
    return 0;
}
static inline int32_t pbio_logger_cols(pbio_log_t *log) {
    return 0;
}
static inline void pbio_logger_stop(pbio_log_t *log) {
}

#endif // PBIO_CONFIG_LOGGER

#endif // _PBIO_LOGGER_H_

/** @} */
