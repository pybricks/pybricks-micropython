// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

// Button driver using analog inputs.
//
// For now, this is hard-coded for SPIKE Prime. If more platforms that use
// analog input for buttons are added, this could be made generic.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_ADC

#include <pbdrv/adc.h>
#include <pbio/button.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

void _pbdrv_button_init(void) {
}

// SPIKE Prime buttons use analog inputs - 3 "buttons" are connected to a
// single pin in the following arrangement:
//
//  ^ 3.3V
//  |
//  Z 10k
//  |
//  +-------+-------+----> AIN
//  |       |       |
//    [ A     [ B     [ C
//  |       |       |
//  Z 18k   Z 33k   Z 82k
//  |       |       |
//  +-------+-------+
//  V GND
//
// On ADC channel 4, B is the center button and A and C are other intputs (not
// buttons). On channel 5, A is the left button, B is the right button and C is
// the Bluetooth button.

pbio_error_t pbdrv_button_is_pressed(pbio_button_flags_t *pressed) {
    uint16_t value;
    pbio_error_t err;

    *pressed = 0;

    err = pbdrv_adc_get_ch(4, &value);
    if (err) {
        return err;
    }

    if (value > 3642) {
        // not a button
    } else if (value > 3142) {
        // not a button
    } else if (value > 2879) {
        *pressed |= PBIO_BUTTON_CENTER;
    } else if (value > 2634) {
        *pressed |= PBIO_BUTTON_CENTER;
    } else if (value > 2449) {
        // not a button
    } else if (value > 2209) {
        // not a button
    } else if (value > 2072) {
        *pressed |= PBIO_BUTTON_CENTER;
    } else if (value > 1800) {
        *pressed |= PBIO_BUTTON_CENTER;
    } else {
        // hardware failure?
        return PBIO_ERROR_IO;
    }

    err = pbdrv_adc_get_ch(5, &value);
    if (err) {
        return err;
    }

    if (value > 3654) {
        // no buttons pressed
    } else if (value > 3155) {
        *pressed |= PBIO_BUTTON_RIGHT_UP; // Bluetooth
    } else if (value > 2885) {
        *pressed |= PBIO_BUTTON_RIGHT;
    } else if (value > 2645) {
        *pressed |= PBIO_BUTTON_RIGHT_UP; // Bluetooth
        *pressed |= PBIO_BUTTON_RIGHT;
    } else if (value > 2454) {
        *pressed |= PBIO_BUTTON_LEFT;
    } else if (value > 2218) {
        *pressed |= PBIO_BUTTON_RIGHT_UP; // Bluetooth
        *pressed |= PBIO_BUTTON_LEFT;
    } else if (value > 2084) {
        *pressed |= PBIO_BUTTON_RIGHT;
        *pressed |= PBIO_BUTTON_LEFT;
    } else if (value > 1800) {
        *pressed |= PBIO_BUTTON_RIGHT_UP; // Bluetooth
        *pressed |= PBIO_BUTTON_RIGHT;
        *pressed |= PBIO_BUTTON_LEFT;
    } else {
        // hardware failure?
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BUTTON_ADC
