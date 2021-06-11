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
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Keep main power on (PA13)
    GPIO_InitTypeDef gpio_init = {
        .Pin = GPIO_PIN_13,
        .Mode = GPIO_MODE_OUTPUT_PP,
    };
    HAL_GPIO_Init(GPIOA, &gpio_init);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_13, GPIO_PIN_SET);

    // NB: HAL_ADC_MspInit() is shared with the PBIO ADC driver, so the ADC is
    // configured for all 6 channels. We need channel 5 to get the center
    // button reading so we set the number of conversions to 5 here. This way
    // HAL_ADC_GetValue() will return the value for channel 5.

    ADC_HandleTypeDef hadc;
    hadc.Instance = ADC1;
    hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc.Init.Resolution = ADC_RESOLUTION_12B;
    hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc.Init.ScanConvMode = ENABLE;
    hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    hadc.Init.ContinuousConvMode = ENABLE;
    hadc.Init.NbrOfConversion = 5;
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.NbrOfDiscConversion = 0;
    hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc.Init.DMAContinuousRequests = DISABLE;
    HAL_ADC_Init(&hadc);


    // at this point, the button should still be pressed, so we wait for it
    // to be released

    bool pressed = true;
    uint32_t start_time = HAL_GetTick();
    while (HAL_GetTick() - start_time < 2000) {
        HAL_ADC_Start(&hadc);
        HAL_ADC_PollForConversion(&hadc, 100);
        uint32_t value = HAL_ADC_GetValue(&hadc);
        HAL_ADC_Stop(&hadc);

        // if button was released, don't wait full two seconds
        if ((value > 2209 && value < 2634) || value > 3142) {
            pressed = false;
            break;
        }
    }

    HAL_ADC_DeInit(&hadc);

    return pressed;
}

void pbio_platform_dual_boot(void) {
    if (!check_for_long_press()) {
        jump_to_other_firmware();
    }
}
