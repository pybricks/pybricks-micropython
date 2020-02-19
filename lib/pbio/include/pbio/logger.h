// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_LOGGER_H_
#define _PBIO_LOGGER_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/error.h>

// FIXME: move upper limit to port config
#define MAX_LOG_MEM_KB 2*1024 // 2 MB on EV3

// Number of values logged by the logger itself, such as time of call to logger
#define NUM_DEFAULT_LOG_VALUES (1)

// Maximum number of values to be logged per sample
#define MAX_LOG_VALUES (20)

// Maximum length (index) of a log
#define MAX_LOG_LEN ((MAX_LOG_MEM_KB*1024) / MAX_LOG_VALUES)

typedef struct _pbio_log_t {
    bool active;
    uint32_t sampled;
    uint32_t len;
    int32_t start;
    uint8_t num_values;
    int32_t *data;
} pbio_log_t;

pbio_error_t pbio_logger_start(pbio_log_t *log, int32_t duration);
pbio_error_t pbio_logger_read(pbio_log_t *log, int32_t sindex, int32_t *buf);
pbio_error_t pbio_logger_update(pbio_log_t *log, int32_t *buf);
pbio_error_t pbio_logger_stop(pbio_log_t *log);

#endif // _PBIO_LOGGER_H_
