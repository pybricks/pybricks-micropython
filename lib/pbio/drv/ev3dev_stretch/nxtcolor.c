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
#include <pbio/light.h>

#include "sys/clock.h"

#define IN (0)
#define OUT (1)

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

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

static const pbio_light_color_t lamp_colors[] = {
    PBIO_LIGHT_COLOR_RED,
    PBIO_LIGHT_COLOR_GREEN,
    PBIO_LIGHT_COLOR_BLUE,
    PBIO_LIGHT_COLOR_NONE,
};

typedef struct _pbdrv_nxtcolor_t {
    bool ready;
    bool fs_initialized;
    bool waiting;
    pbio_light_color_t state;
    pbio_light_color_t lamp;
    uint32_t calibration[3][4];
    uint16_t threshold[2];
    uint32_t raw_min;
    uint32_t raw_max;
    uint16_t crc;
    uint32_t wait_start;
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

// Simplistic nonbusy wait. May be called only once per blocking operation.
pbio_error_t nxtcolor_wait(pbdrv_nxtcolor_t *nxtcolor, uint32_t ms) {

    uint32_t now = clock_usecs();

    // Wait for existing wait to complete
    if (nxtcolor->waiting) {
        if (now - nxtcolor->wait_start > 1000*ms) {
            nxtcolor->waiting = false;
            return PBIO_SUCCESS;
        }
        else {
            return PBIO_ERROR_AGAIN;
        }
    }
    // We are not waiting, so start a new wait
    else {
        nxtcolor->waiting = true;
        nxtcolor->wait_start = now;
        return PBIO_ERROR_AGAIN;
    }
}

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
    int bit;
    err = sysfs_read_int(nxtcolor->f_digi1_val, &bit);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    *val = bit == 1;
    return PBIO_SUCCESS;
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

static pbio_error_t nxtcolor_reset(pbdrv_nxtcolor_t *nxtcolor)
{
    pbio_error_t err;

    // Reset sequence init
    err = nxtcolor_set_digi0(nxtcolor, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = nxtcolor_set_digi1(nxtcolor, 1);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Toggle digi0 several times
    err = nxtcolor_set_digi0(nxtcolor, 1);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = nxtcolor_set_digi0(nxtcolor, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = nxtcolor_set_digi0(nxtcolor, 1);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = nxtcolor_set_digi0(nxtcolor, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}

static pbio_error_t nxtcolor_read_byte(pbdrv_nxtcolor_t *nxtcolor, uint8_t *msg)
{
    pbio_error_t err;
    *msg = 0;

    // Set data back to input
    bool bit;
    err = nxtcolor_get_digi1(nxtcolor, &bit);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Read 8 bits while toggling the "clock"
    for (uint8_t i = 0; i < 8; i++) {
        err = nxtcolor_set_digi0(nxtcolor, 1);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        *msg = *msg >> 1;
        err = nxtcolor_get_digi1(nxtcolor, &bit);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        if (bit) {
            *msg |= 0x80;
        }
        err = nxtcolor_set_digi0(nxtcolor, 0);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    return PBIO_SUCCESS;
}

static pbio_error_t nxtcolor_send_byte(pbdrv_nxtcolor_t *nxtcolor, uint8_t msg)
{
    pbio_error_t err;

    // Init both pins as low
    err = nxtcolor_set_digi0(nxtcolor, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = nxtcolor_set_digi1(nxtcolor, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    for (uint8_t i = 0; i < 8; i++)
	{
        // Set data pin
        err = nxtcolor_set_digi1(nxtcolor, msg & 1);
        if (err != PBIO_SUCCESS) {
            return err;
        }
		msg = msg >> 1;

        // Set clock high
        err = nxtcolor_set_digi0(nxtcolor, 1);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // Set clock low
        err = nxtcolor_set_digi0(nxtcolor, 0);
        if (err != PBIO_SUCCESS) {
            return err;
        }
	}

    return PBIO_SUCCESS;
}

static pbio_error_t nxtcolor_init_fs(pbdrv_nxtcolor_t *nxtcolor, pbio_port_t port) {

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

    return PBIO_SUCCESS;
}

static pbio_error_t nxtcolor_init(pbdrv_nxtcolor_t *nxtcolor, pbio_port_t port) {
    pbio_error_t err;

    // Init the file system
    if (!nxtcolor->fs_initialized) {
        err = nxtcolor_init_fs(nxtcolor, port);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        nxtcolor->fs_initialized = true;

        // Reset the sensor
        err = nxtcolor_reset(nxtcolor);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Wait 100 ms
    err = nxtcolor_wait(nxtcolor, 100);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Set sensor to full color mode
    err = nxtcolor_send_byte(nxtcolor, 13);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Read calibration data and crc bytes
    uint8_t buf[sizeof(nxtcolor->calibration) + sizeof(nxtcolor->threshold) + sizeof(nxtcolor->crc)];
    for (uint8_t i = 0; i < sizeof(buf); i++) {
        err = nxtcolor_read_byte(nxtcolor, &buf[i]);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Process first table
    for (uint32_t row = 0; row < 3; row++) {
        for (uint32_t col = 0; col < 4; col++) {
            uint32_t val = 0;
            uint32_t idx = row*4+col;
            for (uint32_t b = 0; b < 4; b++) {
                val += buf[idx*4+b] << 8*b;
            }
            nxtcolor->calibration[row][col] = val;
        }
    }

    // Process second table
    uint32_t start = sizeof(buf) - sizeof(nxtcolor->crc) - sizeof(nxtcolor->threshold);
    nxtcolor->threshold[0] = (buf[start+1] << 8) + buf[start+0];
    nxtcolor->threshold[1] = (buf[start+3] << 8) + buf[start+2];

    // Other analog calibration values from NXT firmware / experiments
    nxtcolor->raw_max = 750;
    nxtcolor->raw_min = 50;

    // The sensor is now in the full-color-ambient state
    nxtcolor->state = PBIO_LIGHT_COLOR_NONE;

    return PBIO_SUCCESS;
}

pbio_error_t nxtcolor_toggle_color(pbdrv_nxtcolor_t *nxtcolor) {
    bool set = 0;
    switch(nxtcolor->state) {
        case PBIO_LIGHT_COLOR_NONE:
            nxtcolor->state = PBIO_LIGHT_COLOR_RED;
            set = 1;
            break;
        case PBIO_LIGHT_COLOR_RED:
            nxtcolor->state = PBIO_LIGHT_COLOR_GREEN;
            set = 0;
            break;
        case PBIO_LIGHT_COLOR_GREEN:
            nxtcolor->state = PBIO_LIGHT_COLOR_BLUE;
            set = 1;
            break;
        case PBIO_LIGHT_COLOR_BLUE:
            set = 0;
            nxtcolor->state = PBIO_LIGHT_COLOR_NONE;
            break;
        default:
            return PBIO_ERROR_FAILED;
    }
    return nxtcolor_set_digi0(nxtcolor, set);
}

pbio_error_t nxtcolor_set_light(pbdrv_nxtcolor_t *nxtcolor, pbio_light_color_t color) {
    pbio_error_t err;

    // Default unknown colors to no color
    if (color != PBIO_LIGHT_COLOR_RED && color != PBIO_LIGHT_COLOR_GREEN && color != PBIO_LIGHT_COLOR_BLUE) {
        color = PBIO_LIGHT_COLOR_NONE;
    }

    while (nxtcolor->state != color) {
        err = nxtcolor_toggle_color(nxtcolor);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    return PBIO_SUCCESS;
}

pbio_error_t nxtcolor_get_values_at_mode(pbio_port_t port, uint8_t mode, void *values) {

    pbio_error_t err;

    if (port < PBIO_PORT_1 || port > PBIO_PORT_4) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbdrv_nxtcolor_t *nxtcolor = &nxtcolorsensors[port-PBIO_PORT_1];

    // We don't have a formal "get" function since the higher level code
    // does not know about the color sensor being a special case. So instead
    // initialize the first time the sensor is called.
    if (!nxtcolor->ready) {
        err = nxtcolor_init(nxtcolor, port);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        nxtcolor->ready = true;
    }

    // In lamp mode, just set the right color
    if (mode == PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP) {
        nxtcolor->lamp = *((pbio_light_color_t*) values);
        return nxtcolor_set_light(nxtcolor, nxtcolor->lamp);
    }

    // In measure mode, cycle through the colors and calculate color id
    int32_t rgba[4];

    // Read analog for each color
    for (uint8_t i = 0; i < 4; i++) {
        // Set the light
        err = nxtcolor_set_light(nxtcolor, lamp_colors[i]);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        err = nxtcolor_get_adc(nxtcolor, &rgba[i]);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Select calibration row based on ambient light
    uint8_t row = 0;
    if (rgba[3] < nxtcolor->threshold[1]) {
        row = 2;
    }
    else if (rgba[3] < nxtcolor->threshold[0]) {
        row = 1;
    }

    uint8_t rgba_pct[5];

    // Adjust analog to percentage for each color
    for (uint8_t i = 0; i < 3; i++) {
        if (rgba[i] < rgba[3]){
            // If rgb is less than ambient, assume zero
            rgba_pct[i] = 0;
        }
        else {
            // Otherwise, scale by calibration multiplier
            rgba_pct[i] = ( ( (uint32_t) (rgba[i] - rgba[3])) * nxtcolor->calibration[row][i] ) / 111410;
            rgba_pct[i] = rgba_pct[i] > 100 ? 100 : rgba_pct[i];
        }
    }

    // Clamp ambient between estimated max and min raw value
    int32_t amb = max(nxtcolor->raw_min, min(rgba[3], nxtcolor->raw_max));

    // Scale ambient to percentage
    rgba_pct[3] = ((amb-nxtcolor->raw_min)*100)/(nxtcolor->raw_max-nxtcolor->raw_min);

    // TODO: Calculate color index
    pbio_light_color_t calculated_color = PBIO_LIGHT_COLOR_BLACK;
    rgba_pct[4] = calculated_color;

    // Return RGB and Color data
    memcpy(values, rgba_pct, sizeof(rgba_pct));

    // Set the light back to the configured lamp status
    return nxtcolor_set_light(nxtcolor, nxtcolor->lamp);
}
