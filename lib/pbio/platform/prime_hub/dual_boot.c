// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

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
static void jump_to_other_firmware() {
    // Deinit HAL
    HAL_RCC_DeInit();
    HAL_DeInit();

    // Hint to compiler that variables should be saved in registers since
    // we are moving the stack pointer.
    register void (*reset)();

    // Reset the stack pointer to the stack pointer of the other firmware.
    // The official LEGO firmware doesn't do this by itself.
    __set_MSP(*(uint32_t *)OTHER_FIRMWARE_BASE);

    // This probably not needed, but the bootloader does this when jumping
    // to firmware, so we will too just to be safe.
    SCB->VTOR = OTHER_FIRMWARE_BASE;

    // The compiler tools swap the Reset_Handler addresses in the vector
    // tables of this firmware and the other firmware. So the value in this
    // firmware's vector table will be Reset_Handler for the other firmware.
    reset = (void *)_fw_isr_vector_src[1];

    // jump to the other firmware
    reset();
}

/**
 * Checks if the center button is pressed for a long time (~2 s) or just briefly (<< 2 s).
 */
static bool check_for_long_press() {
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // NB: HAL_ADC_MspInit() is shared with the PBIO ADC driver, so the ADC is
    // configured for all 6 channels. Fortunately, the channel we need here
    // is the last channel (by rank) so we can do a single synchronous poll
    // and HAL_ADC_GetValue() will return the one value we need.

    // FIXME: Fix comment above, or the implementation below.

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

    uint32_t value = 3000;
    uint32_t iterations = 30000; // Approximately 2 seconds

    // Keep reading ADC until center button is released or until fixed number of tries
    while (--iterations && value > 2885 && value < 3155) {
        HAL_ADC_Start(&hadc);
        HAL_ADC_PollForConversion(&hadc, 100);
        value = HAL_ADC_GetValue(&hadc);

        HAL_ADC_Stop(&hadc);
    }

    HAL_ADC_DeInit(&hadc);

    __HAL_RCC_GPIOC_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_ADC1_CLK_DISABLE();

    // If iterations ran out, return true to indicate a long press
    return iterations == 0;
}

void pbio_platform_dual_boot() {
    // FIXME: Invert me. It's currently the opposite because the stock firmware
    // does not boot if the center button is not pressed, possibly because power
    // is not yet on at that point.
    if (check_for_long_press()) {
        jump_to_other_firmware();
    }
}
