// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LIGHT

#include <pbdrv/light.h>
#include <pbdrv/pwm.h>
#include <pbio/color.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, const pbdrv_light_raw_rgb_t *rgb) {
    if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
        // TODO: check for Powered UP Lights connected to ports C/D
        return PBIO_ERROR_NO_DEV;
    }

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbdrv_pwm_dev_t *dev;
    if (pbdrv_pwm_get_dev(3, &dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(dev, 1, rgb->r);
    }
    if (pbdrv_pwm_get_dev(2, &dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(dev, 1, rgb->g);
        pbdrv_pwm_set_duty(dev, 2, rgb->b);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_color_t color,
    pbdrv_light_raw_rgb_t *raw) {
    if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
        // TODO: check for Powered UP Lights connected to ports C/D
        return PBIO_ERROR_NO_DEV;
    }

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    switch (color) {
        case PBIO_COLOR_WHITE:
            raw->r = 205;
            raw->g = 35;
            raw->b = 23;
            break;
        case PBIO_COLOR_RED:
            raw->r = 733;
            raw->g = 7;
            raw->b = 1;
            break;
        case PBIO_COLOR_ORANGE:
            raw->r = 426;
            raw->g = 26;
            raw->b = 1;
            break;
        case PBIO_COLOR_YELLOW:
            raw->r = 227;
            raw->g = 38;
            raw->b = 1;
            break;
        case PBIO_COLOR_GREEN:
            raw->r = 6;
            raw->g = 52;
            raw->b = 1;
            break;
        case PBIO_COLOR_BLUE:
            raw->r = 42;
            raw->g = 0;
            raw->b = 243;
            break;
        case PBIO_COLOR_PURPLE:
            raw->r = 370;
            raw->g = 3;
            raw->b = 130;
            break;
        default:
            raw->r = 0;
            raw->g = 0;
            raw->b = 0;
            break;
    }

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_LIGHT
