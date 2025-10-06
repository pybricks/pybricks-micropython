// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_STM32F0

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/adc.h>
#include <pbio/config.h>
#include <pbio/error.h>

#include "stm32f0xx.h"

#if PBDRV_CONFIG_ADC_STM32F0_RANDOM
#include "../random/random_adc.h"
#endif

static void pbdrv_adc_calibrate(void) {
    // NB: it takes more than this to make sure ADC is disabled
    // it should be OK though since we only calibrate at init when A/DC is
    // already disabled
    if (ADC1->CR & ADC_CR_ADEN) {
        ADC1->CR &= ~ADC_CR_ADEN;
    }

    ADC1->CR |= ADC_CR_ADCAL;

    while (ADC1->CR & ADC_CR_ADCAL) {
    }
}

void pbdrv_adc_init(void) {
    // enable power domain
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

    // using dedicated 14MHz clock
    RCC->CR2 |= RCC_CR2_HSI14ON;

    do {
    } while (!(RCC->CR2 & RCC_CR2_HSI14RDY));


    pbdrv_adc_calibrate();

    ADC1->ISR = ADC_ISR_ADRDY;

    do {
        ADC1->CR |= ADC_CR_ADEN;
    } while (!(ADC1->ISR & ADC_ISR_ADRDY));

    // set sampling time to 239.5 ADC clock cycles (longest possible)
    ADC1->SMPR = (ADC1->SMPR & ADC_SMPR_SMP_Msk) | (7 << ADC_SMPR_SMP_Pos);
    ADC->CCR |= ADC_CCR_VREFEN;

    // TODO: LEGO firmware reads CH 3 during init 10 times and averages it.
    // Not sure what this is measuring or what it would be used for. Perhaps
    // some kind of ID resistor?
}

pbio_error_t pbdrv_adc_await_new_samples(pbio_os_state_t *state, uint32_t *start_time_us, uint32_t future_us) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

// does a single conversion for the specified channel
pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {
    if (ch > ADC_CHSELR_CHSEL18_Pos) {
        return PBIO_ERROR_INVALID_ARG;
    }

    ADC1->CFGR1 &= ~ADC_CFGR1_CONT;
    ADC1->CHSELR = 1 << ch;

    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) {
    }

    *value = ADC1->DR;

    #if PBDRV_CONFIG_ADC_STM32F0_RANDOM
    pbdrv_random_adc_push_lsb(*value);
    #endif

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_ADC_STM32F0
