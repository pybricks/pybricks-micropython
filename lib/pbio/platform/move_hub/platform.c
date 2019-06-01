// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include "../../drv/ioport/ioport_lpf2.h"

#include "stm32f070xb.h"

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
