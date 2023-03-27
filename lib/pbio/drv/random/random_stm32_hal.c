// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

// Random number generator using STM32 RNG via HAL.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RANDOM_STM32_HAL

#include <stdint.h>

#include <pbio/error.h>

#include STM32_HAL_H

static RNG_HandleTypeDef pbdrv_random_hrng;

// platform integration

// REVISIT: this should be moved to platform.c if needed

void RNG_IRQHandler(void) {
    HAL_RNG_IRQHandler(&pbdrv_random_hrng);
}

void HAL_RNG_MspInit(RNG_HandleTypeDef *hrng) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    // Select PLLQ output as RNG clock source
    // NB: default HSI48 clock doesn't seem to work
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RNG;
    PeriphClkInitStruct.RngClockSelection = RCC_RNGCLKSOURCE_PLL;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    __HAL_RCC_RNG_CLK_ENABLE();
}

void HAL_RNG_MspDeInit(RNG_HandleTypeDef *hrng) {
    __HAL_RCC_RNG_FORCE_RESET();
    __HAL_RCC_RNG_RELEASE_RESET();
}

// Internal API implementation

void pbdrv_random_init(void) {
    RNG_HandleTypeDef *hrng = &pbdrv_random_hrng;

    hrng->Instance = RNG;
    #ifdef RNG_CR_CED
    hrng->Init.ClockErrorDetection = RNG_CED_ENABLE;
    #endif
    HAL_RNG_Init(hrng);

    HAL_NVIC_SetPriority(RNG_IRQn, 15, 0);
    HAL_NVIC_EnableIRQ(RNG_IRQn);

    // start now so there is a new number ready ASAP
    HAL_RNG_GenerateRandomNumber_IT(hrng);
}

// Public API implementation

pbio_error_t pbdrv_random_get(uint32_t *random) {
    RNG_HandleTypeDef *hrng = &pbdrv_random_hrng;

    if (HAL_RNG_GetState(hrng) != HAL_RNG_STATE_READY) {
        // REVISIT: this is a hack to debug errors
        // this should be replaced with proper error handling
        if (HAL_RNG_GetState(hrng) == HAL_RNG_STATE_ERROR) {
            *random = (0x01010100 * HAL_RNG_GetError(hrng)) | (hrng->Instance->SR & 0xFF);
        }

        return PBIO_ERROR_AGAIN;
    }

    *random = HAL_RNG_ReadLastRandomNumber(hrng);

    // start now so there is a new number ready ASAP
    HAL_RNG_GenerateRandomNumber_IT(hrng);

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_RANDOM_STM32_HAL
