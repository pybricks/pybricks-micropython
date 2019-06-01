// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

#include "../../drv/button/button_gpio.h"

#include "stm32f446xx.h"

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio   = { .bank = GPIOC, .pin = 13 },
        .pull   = PBDRV_GPIO_PULL_UP,
        .button = PBIO_BUTTON_CENTER,
    }
};

static struct {
    pbio_iodev_info_t info;
    pbio_iodev_mode_t modes[PBIO_IODEV_MAX_NUM_MODES];
} uart_iodev_info;

pbio_iodev_t uart_iodev = {
    .info = &uart_iodev_info.info,
    .port = PBIO_PORT_1,
};

// HACK: we don't have a generic ioport interface yet so defining this function
// in platform.c
pbio_error_t pbdrv_ioport_get_iodev(pbio_port_t port, pbio_iodev_t **iodev) {
    switch (port) {
    case PBIO_PORT_1:
        *iodev = &uart_iodev;
        break;
    default:
        return PBIO_ERROR_INVALID_PORT;
    }

    return PBIO_SUCCESS;
}
