// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/port.h>

#include "stm32f446xx.h"

// PC13 is the green button (active low)

void _pbdrv_button_init(void) {
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER13_Msk) | (0 << GPIO_MODER_MODER13_Pos);
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_button_deinit(void) { }
#endif

pbio_error_t pbdrv_button_is_pressed(pbio_port_t port, pbio_button_flags_t *pressed) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    *pressed = (GPIOC->IDR & GPIO_IDR_ID13) ? PBIO_BUTTON_CENTER : 0;

    return PBIO_SUCCESS;
}
