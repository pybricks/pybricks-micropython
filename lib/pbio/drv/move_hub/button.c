/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David Lechner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/port.h>

#include "stm32f070xb.h"

// PC13 is the green button (active low)

void _pbdrv_button_init(void) {
    // set PC13 input with pull up
    GPIOC->PUPDR = (GPIOC->PUPDR & ~GPIO_PUPDR_PUPDR13_Msk) | (1 << GPIO_PUPDR_PUPDR13_Pos);
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER13_Msk) | (0 << GPIO_MODER_MODER13_Pos);
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_button_deinit(void) { }
#endif

pbio_error_t pbdrv_button_is_pressed(pbio_port_t port, pbio_button_flags_t *pressed) {
    if (port != PBIO_PORT_SELF) {
        // TODO: add port for remote control
        return PBIO_ERROR_INVALID_PORT;
    }

    // PC13 is low when the button is pressed
    *pressed = (GPIOC->IDR & GPIO_IDR_13) ? 0 : PBIO_BUTTON_CENTER;

    return PBIO_SUCCESS;
}
