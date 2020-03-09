// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_NXT

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <pbio/util.h>
#include <pbio/port.h>
#include "counter.h"

#include <nxt/nxt_motors.h>

typedef struct {
    pbdrv_counter_dev_t dev;
    pbio_port_t port;
} private_data_t;

static private_data_t private_data[PBDRV_CONFIG_COUNTER_NXT_NUM_DEV];

static pbio_error_t pbdrv_counter_nxt_get_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    private_data_t *data = PBIO_CONTAINER_OF(dev, private_data_t, dev);

    *count = nxt_motor_get_count(data->port);

    return PBIO_SUCCESS;
}

static pbio_error_t pbdrv_counter_nxt_get_rate(pbdrv_counter_dev_t *dev, int32_t *rate) {
    *rate = 0;

    return PBIO_SUCCESS;
}

static pbio_error_t counter_nxt_init() {
    for (int i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        private_data_t *data = &private_data[i];

        data->port = i;
        data->dev.get_count = pbdrv_counter_nxt_get_count;
        data->dev.get_rate = pbdrv_counter_nxt_get_rate;
        data->dev.initalized = true;

        // FIXME: assuming that these are the only counter devices
        // counter_id should be passed from platform data instead
        pbdrv_counter_register(i, &data->dev);
    }

    return PBIO_SUCCESS;
}

static pbio_error_t counter_nxt_exit() {
    for (int i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        private_data_t *data = &private_data[i];

        data->dev.initalized = false;
        pbdrv_counter_unregister(&data->dev);
    }
    return PBIO_SUCCESS;
}

const pbdrv_counter_drv_t pbdrv_counter_nxt_drv = {
    .init = counter_nxt_init,
    .exit = counter_nxt_exit,
};

#endif // PBDRV_CONFIG_COUNTER_NXT
