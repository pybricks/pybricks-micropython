
#include <stdint.h>
#include <stdio.h>

#include "stm32f070xb.h"

#include "adc.h"

void adc_calibrate() {
    // TODO: it takes more than this to make sure ADC is disabled
    if (ADC1->CR & ADC_CR_ADEN) {
        ADC1->CR &= ~ADC_CR_ADEN;
    }

    ADC1->CR |= ADC_CR_ADCAL;

    while (ADC1->CR & ADC_CR_ADCAL) { }
}

void adc_init() {
    // enable power domain
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

    // using dedicated 14MHz clock
    RCC->CR2 |= RCC_CR2_HSI14ON;

    do {
    } while (!(RCC->CR2 & RCC_CR2_HSI14RDY));


    adc_calibrate();

    ADC1->ISR = ADC_ISR_ADRDY;

    do {
        ADC1->CR |= ADC_CR_ADEN;
     } while (!(ADC1->ISR & ADC_ISR_ADRDY));

    // set sampling time to 239.5 ADC clock cycles (longest possible)
    ADC1->SMPR = (ADC1->SMPR & ADC_SMPR_SMP_Msk) | (7 << ADC_SMPR_SMP_Pos);
    ADC->CCR |= ADC_CCR_VREFEN;
}

// does a single coversion for the specified channel
int adc_read_ch(int ch) {
    // TODO: error checking on ch value
    ADC1->CFGR1 &= ~ADC_CFGR1_CONT;
    ADC1->CHSELR = 1 << ch;

    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }

     return ADC1->DR;
}

void adc_deinit() {
    // TODO: do we need timeouts here?
    ADC1->CR |= ADC_CR_ADSTP;
    while (ADC1->CR & ADC_CR_ADSTP) { }

    ADC1->CR |= ADC_CR_ADDIS;
    while (ADC1->CR & ADC_CR_ADEN) { }
}
