// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/ev3sensor.h>
#include <pbdrv/ev3devsysfs.h>
#include <pbio/ev3device.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

#define IN (0)
#define OUT (1)

typedef struct {
    const int digi0; // GPIO on wire 5
    const int digi1; // GPIO on wire 6
    const int adc_val; // ADC on wire 6 for getting reflection data
    const int adc_con; // ADC on wire 1 for detecting sensor
} pbdrv_nxtcolor_pininfo_t;

static const pbdrv_nxtcolor_pininfo_t pininfo[4] = {
    [PBIO_PORT_2 - PBIO_PORT_1] = {
        .digi0 = 14,
        .digi1 = 13,
        .adc_val = 7,
        .adc_con = 8,
    },
};

typedef struct _pbdrv_nxtcolor_t {
    bool initialized;
    const pbdrv_nxtcolor_pininfo_t *pins;
    FILE *f_digi0_val;
    FILE *f_digi0_dir;
    FILE *f_digi1_val;
    FILE *f_digi1_dir;
    bool digi1_dir;
    FILE *f_adc_val;
    FILE *f_adc_con;
} pbdrv_nxtcolor_t;

pbdrv_nxtcolor_t nxtcolorsensors[4];

static pbio_error_t nxtcolor_set_digi0(pbdrv_nxtcolor_t *nxtcolor, bool val) {
    return sysfs_write_int(nxtcolor->f_digi0_val, val);
}

static pbio_error_t nxtcolor_set_digi1(pbdrv_nxtcolor_t *nxtcolor, bool val) {

    pbio_error_t err;

    // First, ensure it is set as a digital out
    if (nxtcolor->digi1_dir == IN) {
        err = sysfs_write_str(nxtcolor->f_digi1_dir, "out");
        if (err != PBIO_SUCCESS) {
            return err;
        }
        nxtcolor->digi1_dir = OUT;
    }
    // Set the requested state
    return sysfs_write_int(nxtcolor->f_digi1_val, val);
}

static pbio_error_t nxtcolor_get_digi1(pbdrv_nxtcolor_t *nxtcolor, bool *val) {

    pbio_error_t err;

    // First, ensure it is set as a digital in
    if (nxtcolor->digi1_dir == OUT) {
        err = sysfs_write_str(nxtcolor->f_digi1_dir, "in");
        if (err != PBIO_SUCCESS) {
            return err;
        }
        nxtcolor->digi1_dir = IN;
    }
    // Get the state
    return sysfs_read_int(nxtcolor->f_digi1_val, (int*) val);
}

static pbio_error_t nxtcolor_get_adc(pbdrv_nxtcolor_t *nxtcolor, int32_t *analog) {

    pbio_error_t err;

    // First, ensure it is set as an input
    if (nxtcolor->digi1_dir == OUT) {
        err = sysfs_write_str(nxtcolor->f_digi1_dir, "in");
        if (err != PBIO_SUCCESS) {
            return err;
        }
        nxtcolor->digi1_dir = IN;
    }
    // Get the state
    return sysfs_read_int(nxtcolor->f_adc_val, analog);
}

static pbio_error_t nxtcolor_init(pbdrv_nxtcolor_t *nxtcolor, pbio_port_t port) {

    pbio_error_t err;

    // Support only port 2 for now
    if (port != PBIO_PORT_2) {
        return PBIO_ERROR_NOT_IMPLEMENTED;
    }

    // Get the pin info for this port
    nxtcolor->pins = &pininfo[port-PBIO_PORT_1];

    // Open the sysfs files for this sensor
    err = sysfs_open(&nxtcolor->f_digi0_val, "/sys/class/gpio/gpio%d/%s", nxtcolor->pins->digi0, "value", "w");
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = sysfs_open(&nxtcolor->f_digi0_dir, "/sys/class/gpio/gpio%d/%s", nxtcolor->pins->digi0, "direction", "w");
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = sysfs_open(&nxtcolor->f_digi1_val, "/sys/class/gpio/gpio%d/%s", nxtcolor->pins->digi1, "value", "r+");
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = sysfs_open(&nxtcolor->f_digi1_dir, "/sys/class/gpio/gpio%d/%s", nxtcolor->pins->digi1, "direction", "w");
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = sysfs_open(&nxtcolor->f_adc_con, "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw%s", nxtcolor->pins->adc_con, "", "r");
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = sysfs_open(&nxtcolor->f_adc_val, "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw%s", nxtcolor->pins->adc_val, "", "r");
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Verify that the sensor is indeed attached
    int32_t adc_con;
    err = sysfs_read_int(nxtcolor->f_adc_con, &adc_con);
    if (adc_con > 50) {
        return PBIO_ERROR_NO_DEV;
    }

    // Digi0 is always an output pin. Init as low
    err = sysfs_write_str(nxtcolor->f_digi0_dir, "out");
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = nxtcolor_set_digi0(nxtcolor, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Digi1 can be set as output, or read as digital, and analog. Init as low.
    err = nxtcolor_set_digi1(nxtcolor, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    nxtcolor->initialized = true;

    return PBIO_SUCCESS;
}

pbio_error_t nxtcolor_get_values_at_mode(pbio_port_t port, uint8_t mode, void *values) {

    pbio_error_t err;

    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbdrv_nxtcolor_t *nxtcolor = &nxtcolorsensors[port-PBIO_PORT_1];

    if (!nxtcolor->initialized) {
        err = nxtcolor_init(nxtcolor, port);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    return PBIO_ERROR_NOT_IMPLEMENTED;
}
