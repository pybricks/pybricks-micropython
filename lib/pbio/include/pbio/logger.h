// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_LOGGER_H_
#define _PBIO_LOGGER_H_

#include <inttypes.h>

// FIXME: Move to port configuration
#define MIN_PERIOD 10

// FIXME: move upper limit to port config
#define MAX_LOG_MEM_KB 2*1024 // 2 MB on EV3

#define MAX_LOG_LEN ((MAX_LOG_MEM_KB*1024)/sizeof(pbio_log_data_t))

typedef struct _pbio_log_data_t {
    int32_t time;
    int32_t count;
    int32_t rate;
} pbio_log_data_t;

typedef struct _pbio_log_t {
    bool active;
    uint32_t sampled;
    uint32_t len;
    int32_t start;
    pbio_log_data_t *data;
} pbio_log_t;

#endif // _PBIO_LOGGER_H_
