// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_STM32F0

#include <stdbool.h>
#include <stdint.h>

#include <contiki.h>

#include <pbio/config.h>
#include <pbio/error.h>

#include "stm32f0xx.h"

PROCESS(pbdrv_adc_process, "ADC");

static void pbdrv_adc_calibrate() {
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

static void pbdrv_adc_init() {
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

// does a single coversion for the specified channel
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

    return PBIO_SUCCESS;
}

#if PBIO_CONFIG_ENABLE_DEINIT
static void pbdrv_adc_exit() {
    // REVISIT: do we need timeouts here?
    ADC1->CR |= ADC_CR_ADSTP;
    while (ADC1->CR & ADC_CR_ADSTP) {
    }

    ADC1->CR |= ADC_CR_ADDIS;
    while (ADC1->CR & ADC_CR_ADEN) {
    }
}
#endif

PROCESS_THREAD(pbdrv_adc_process, ev, data) {
    // TODO: use DMA for background updates and add filtering
    // PROCESS_POLLHANDLER(pbdrv_adc_poll());
    #if PBIO_CONFIG_ENABLE_DEINIT
    PROCESS_EXITHANDLER(pbdrv_adc_exit());
    #endif

    PROCESS_BEGIN();

    pbdrv_adc_init();

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_ADC_STM32F0
