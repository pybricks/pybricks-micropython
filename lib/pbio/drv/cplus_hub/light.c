// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LIGHT

#include <pbdrv/light.h>
#include <pbdrv/pwm.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Timers have period of 10000 and we want period / 5 as max brightness.

    pbdrv_pwm_dev_t *dev;
    if (pbdrv_pwm_get_dev(1, &dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(dev, 2, 10000 - r * 2000 / 255);
    }
    if (pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(dev, 4, 10000 - g * 2000 / 255);
    }
    if (pbdrv_pwm_get_dev(2, &dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(dev, 1, 10000 - b * 2000 / 255);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_light_color_t color,
    uint8_t *r, uint8_t *g, uint8_t *b) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    switch (color) {
        case PBIO_LIGHT_COLOR_WHITE:
            *r = 185;
            *g = 161;
            *b = 22;
            break;
        case PBIO_LIGHT_COLOR_RED:
            *r = 255;
            *g = 0;
            *b = 0;
            break;
        case PBIO_LIGHT_COLOR_ORANGE:
            *r = 255;
            *g = 37;
            *b = 0;
            break;
        case PBIO_LIGHT_COLOR_YELLOW:
            *r = 255;
            *g = 140;
            *b = 0;
            break;
        case PBIO_LIGHT_COLOR_GREEN:
            *r = 0;
            *g = 255;
            *b = 0;
            break;
        case PBIO_LIGHT_COLOR_BLUE:
            *r = 0;
            *g = 0;
            *b = 180;
            break;
        case PBIO_LIGHT_COLOR_PURPLE:
            *r = 220;
            *g = 0;
            *b = 110;
            break;
        default:
            *r = 0;
            *g = 0;
            *b = 0;
            break;
    }

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_LIGHT
