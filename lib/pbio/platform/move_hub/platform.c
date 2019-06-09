// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/uartdev.h>

#include "../../drv/button/button_gpio.h"
#include "../../drv/ioport/ioport_lpf2.h"
#include "../../drv/uart/uart_stm32f0.h"

#include "stm32f070xb.h"

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio   = { .bank = GPIOC, .pin = 13 },
        .pull   = PBDRV_GPIO_PULL_UP,
        .button = PBIO_BUTTON_CENTER,
        .active_low = true,
    },
};

// Port C - USART4
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_0 = {
    .id1        = { .bank = GPIOB, .pin = 7  },
    .id2        = { .bank = GPIOC, .pin = 15 },
    .uart_buf   = { .bank = GPIOB, .pin = 4  },
    .uart_tx    = { .bank = GPIOC, .pin = 10 },
    .uart_rx    = { .bank = GPIOC, .pin = 11 },
    .alt        = 0,
};

// Port D - USART3
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_1 = {
    .id1        = { .bank = GPIOB, .pin = 10 },
    .id2        = { .bank = GPIOA, .pin = 12 },
    .uart_buf   = { .bank = GPIOB, .pin = 0  },
    .uart_tx    = { .bank = GPIOC, .pin = 4  },
    .uart_rx    = { .bank = GPIOC, .pin = 5  },
    .alt        = 1,
};

// UART

enum {
    UART_ID_0,
    UART_ID_1,
};

const pbdrv_uart_stm32f0_platform_data_t pbdrv_uart_stm32f0_platform_data[PBDRV_CONFIG_UART_STM32F0_NUM_UART] = {
    [UART_ID_0] = {
        .uart   = USART4,
        .irq    = USART3_4_IRQn,
    },
    [UART_ID_1] = {
        .uart   = USART3,
        .irq    = USART3_4_IRQn,
    },
};

// overrides weak function in setup_*.m
void USART3_4_IRQHandler(void) {
    pbdrv_uart_stm32f0_handle_irq(UART_ID_0);
    pbdrv_uart_stm32f0_handle_irq(UART_ID_1);
}

#if PBIO_CONFIG_UARTDEV
const pbio_uartdev_platform_data_t pbio_uartdev_platform_data[PBIO_CONFIG_UARTDEV_NUM_DEV] = {
    [0] = {
        .uart_id    = UART_ID_0,
    },
    [1] = {
        .uart_id    = UART_ID_1,
    },
};
#endif // PBIO_CONFIG_UARTDEV
