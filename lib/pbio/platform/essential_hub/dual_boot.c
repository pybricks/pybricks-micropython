// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

// Provides special handling when the dual-boot version of the firmware is used.

#include <stdbool.h>
#include <stdint.h>

#include <stm32f4xx_hal.h>

// special memory addresses defined in linker script
extern uint32_t _fw_isr_vector_src[];

#define OTHER_FIRMWARE_BASE 0x08008000

/**
 * When this firmware is built as dual-boot, this will jump to the other
 * firmware in the flash memory.
 */
__attribute__((noreturn)) static void jump_to_other_firmware(void) {
    __disable_irq();
    HAL_RCC_DeInit();

    // This probably not needed, but the bootloader does this when jumping
    // to firmware, so we will too just to be safe.
    SCB->VTOR = OTHER_FIRMWARE_BASE;

    // Reset the stack pointer to the stack pointer of the other firmware.
    // The official LEGO firmware doesn't do this by itself.
    __set_MSP(*(uint32_t *)OTHER_FIRMWARE_BASE);

    // The compiler tools swap the Reset_Handler addresses in the vector
    // tables of this firmware and the other firmware. So the value in this
    // firmware's vector table will be Reset_Handler for the other firmware.
    __asm("bx %0" : : "r" (_fw_isr_vector_src[1]));
    __builtin_unreachable();
}

/**
 * Checks if the center button is pressed for more than 2 seconds during early boot.
 */
static bool check_for_long_press(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Keep main power on (PB1)
    GPIO_InitTypeDef gpio_init = {
        .Pin = GPIO_PIN_11,
        .Mode = GPIO_MODE_OUTPUT_PP,
    };
    HAL_GPIO_Init(GPIOB, &gpio_init);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);

    // configure button
    gpio_init.Pin = GPIO_PIN_2,
    gpio_init.Mode = GPIO_MODE_INPUT,
    HAL_GPIO_Init(GPIOB, &gpio_init);

    // at this point, the button should still be pressed, so we wait for it
    // to be released

    bool pressed = true;
    uint32_t start_time = HAL_GetTick();
    while (HAL_GetTick() - start_time < 2000) {
        // if button was released, don't wait full two seconds
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_SET /* active low */) {
            pressed = false;
            break;
        }
    }

    return pressed;
}

void pbio_platform_dual_boot(void) {
    if (!check_for_long_press()) {
        jump_to_other_firmware();
    }
}
