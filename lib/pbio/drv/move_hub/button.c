
#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/port.h>

#include "stm32f070xb.h"

// PC13 is the green button (active low)

void pbdrv_button_init(void) {
    // set PC13 input with pull up
    GPIOC->PUPDR = (GPIOC->PUPDR & ~GPIO_PUPDR_PUPDR13_Msk) | (1 << GPIO_PUPDR_PUPDR13_Pos);
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER13_Msk) | (0 << GPIO_MODER_MODER13_Pos);
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void pbdrv_button_deinit(void) { }
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
