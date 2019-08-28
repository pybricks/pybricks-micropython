// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

// ev3dev-stretch PRU/IIO Quadrature Encoder Counter driver
//
// This driver uses the PRU quadrature encoder found in ev3dev-stretch.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <libudev.h>

#include <pbio/util.h>
#include "counter.h"

#define DEBUG 0
#if DEBUG
#define dbg_err(s) perror(s)
#else
#define dbg_err(s)
#endif

typedef struct {
    pbdrv_counter_dev_t dev;
    FILE *count;
    FILE *rate;
} private_data_t;

static private_data_t private_data[PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO_NUM_DEV];

static pbio_error_t pbdrv_counter_ev3dev_stretch_iio_get_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    private_data_t *data = PBIO_CONTAINER_OF(dev, private_data_t, dev);

    if (!data->count) {
        return PBIO_ERROR_NO_DEV;
    }

    if (fseek(data->count, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fscanf(data->count, "%d", count) == EOF) {
        return PBIO_ERROR_IO;
    }

    if (fflush(data->count) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

static pbio_error_t pbdrv_counter_ev3dev_stretch_iio_get_rate(pbdrv_counter_dev_t *dev, int32_t *rate) {
    private_data_t *data = PBIO_CONTAINER_OF(dev, private_data_t, dev);

    if (!data->rate) {
        return PBIO_ERROR_NO_DEV;
    }

    if (fseek(data->rate, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fscanf(data->rate, "%d", rate) == EOF) {
        return PBIO_ERROR_IO;
    }

    if (fflush(data->rate) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

static pbio_error_t counter_ev3dev_stretch_iio_init() {
    char buf[256];
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *entry;
    pbio_error_t err = PBIO_ERROR_FAILED;

    udev = udev_new();
    if (!udev) {
        dbg_err("Failed to get udev context");
        return err;
    }

    enumerate = udev_enumerate_new(udev);
    if (!enumerate) {
        dbg_err("Failed to get udev context");
        goto free_udev;
    }

    if ((errno = udev_enumerate_add_match_subsystem(enumerate, "iio")) < 0) {
        dbg_err("udev_enumerate_add_match_subsystem failed");
        goto free_enumerate;
    }

    if ((errno = udev_enumerate_add_match_property(enumerate, "OF_NAME", "ev3-tacho-rpmsg")) < 0) {
        dbg_err("udev_enumerate_add_match_property failed");
        goto free_enumerate;
    }

    if ((errno = udev_enumerate_scan_devices(enumerate) < 0)) {
        dbg_err("udev_enumerate_scan_devices failed");
        goto free_enumerate;
    }

    entry = udev_enumerate_get_list_entry(enumerate);
    if (!entry) {
        dbg_err("udev_enumerate_get_list_entry failed");
        goto free_enumerate;
    }


    for (int i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        private_data_t *data = &private_data[i];

        snprintf(buf, sizeof(buf), "%s/in_count%d_raw", udev_list_entry_get_name(entry), i);
        data->count = fopen(buf, "r");
        if (!data->count) {
            dbg_err("failed to open count attribute");
            continue;
        }

        snprintf(buf, sizeof(buf), "%s/in_frequency%d_input", udev_list_entry_get_name(entry), i);
        data->rate = fopen(buf, "r");
        if (!data->rate) {
            dbg_err("failed to open rate attribute");
            continue;
        }

        data->dev.get_count = pbdrv_counter_ev3dev_stretch_iio_get_count;
        data->dev.get_rate = pbdrv_counter_ev3dev_stretch_iio_get_rate;
        data->dev.initalized = true;

        // FIXME: assuming that these are the only counter devices
        // counter_id should be passed from platform data instead
        pbdrv_counter_register(i, &data->dev);
    }

    err = PBIO_SUCCESS;

free_enumerate:
    udev_enumerate_unref(enumerate);
free_udev:
    udev_unref(udev);

    return err;
}

static pbio_error_t counter_ev3dev_stretch_iio_exit() {
    for (int i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        private_data_t *data = &private_data[i];

        data->dev.initalized = false;
        if (data->count) {
            fclose(data->count);
        }
        if (data->rate) {
            fclose(data->rate);
        }
        pbdrv_counter_unregister(&data->dev);
    }
    return PBIO_SUCCESS;
}

const pbdrv_counter_drv_t pbdrv_counter_ev3dev_stretch_iio_drv = {
    .init   = counter_ev3dev_stretch_iio_init,
    .exit   = counter_ev3dev_stretch_iio_exit,
};

#endif // PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO
