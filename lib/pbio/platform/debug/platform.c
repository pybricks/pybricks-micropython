// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>
#include <pbio/uartdev.h>

#define USE_HAL_DRIVER
#include "stm32f4xx.h"

#include "../../drv/button/button_gpio.h"
#include "../../drv/uart/uart_stm32f4.h"


const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio   = { .bank = GPIOC, .pin = 13 },
        .button = PBIO_BUTTON_CENTER,
    }
};

// UART Config
//
// Currently using pins labeled USART on the Nucleo board as a UART sensor port.
// UART sensors can be wired to the port with 22Ω series resistors on Tx and Rx.

enum {
    UART_ID_0,
};

const pbdrv_uart_stm32f4_platform_data_t pbdrv_uart_stm32f4_platform_data[PBDRV_CONFIG_UART_STM32F4_NUM_UART] = {
    [UART_ID_0] = {
        .uart   = USART2,
        .irq    = USART2_IRQn,
    },
};

const pbio_uartdev_platform_data_t pbio_uartdev_platform_data[PBIO_CONFIG_UARTDEV_NUM_DEV] = {
    [0] = {
        .uart_id    = UART_ID_0,
    },
};

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef gpio_init;

    // clocks are enabled in sys.c

    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = 7;

    gpio_init.Pin = GPIO_PIN_5;
    HAL_GPIO_Init(GPIOD, &gpio_init);

    gpio_init.Pin = GPIO_PIN_6;
    HAL_GPIO_Init(GPIOD, &gpio_init);
}

// overrides weak function in setup.m
void USART2_IRQHandler(void) {
    pbdrv_uart_stm32f4_handle_irq(UART_ID_0);
}

// HACK: we don't have a generic ioport interface yet so defining this function
// in platform.c
pbio_error_t pbdrv_ioport_get_iodev(pbio_port_t port, pbio_iodev_t **iodev) {
    if (port != PBIO_PORT_1) {
        return PBIO_ERROR_INVALID_PORT;
    }

    return pbio_uartdev_get(0, iodev);
}
