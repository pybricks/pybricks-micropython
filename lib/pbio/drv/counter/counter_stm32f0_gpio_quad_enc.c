// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 The Pybricks Authors

// GPIO Quadrature Encoder Counter driver
//
// This driver uses GPIOs and a timer to create a quadrature encoder.
//
// This driver is currently hard-coded for the LEGO BOOST Move hub internal motors.
// Ideally, this could be made into a generic driver if the following are done:
// - add IRQ support to the gpio driver
// - create a generic timer driver

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/gpio.h>
#include <pbio/util.h>

#include "stm32f0xx.h"
#include "counter.h"
#include "counter_stm32f0_gpio_quad_enc.h"

typedef struct {
    pbdrv_counter_dev_t *dev;
    int32_t count;
} private_data_t;

static private_data_t private_data[PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC_NUM_DEV];

static pbio_error_t pbdrv_counter_stm32f0_gpio_quad_enc_get_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    private_data_t *priv = dev->priv;

    *count = priv->count;

    return PBIO_SUCCESS;
}

static void pbdrv_counter_stm32f0_gpio_quad_enc_update_count(private_data_t *priv,
    bool int_pin_state, bool dir_pin_state) {
    if (int_pin_state ^ dir_pin_state) {
        priv->count--;
    } else {
        priv->count++;
    }
}

// irq handler name defined in startup_stm32f0.s
void EXTI0_1_IRQHandler(void) {
    uint32_t exti_pr;

    exti_pr = EXTI->PR & (EXTI_PR_PR0 | EXTI_PR_PR1);
    EXTI->PR = exti_pr; // clear the events we are handling

    // Port A - inverted.
    if (exti_pr & EXTI_PR_PR1) {
        private_data_t *priv = &private_data[0];
        const pbdrv_counter_stm32f0_gpio_quad_enc_platform_data_t *pdata = priv->dev->pdata;
        pbdrv_counter_stm32f0_gpio_quad_enc_update_count(priv,
            pbdrv_gpio_input(&pdata->gpio_int), !pbdrv_gpio_input(&pdata->gpio_dir));
    }

    // Port B
    if (exti_pr & EXTI_PR_PR0) {
        private_data_t *priv = &private_data[1];
        const pbdrv_counter_stm32f0_gpio_quad_enc_platform_data_t *pdata = priv->dev->pdata;
        pbdrv_counter_stm32f0_gpio_quad_enc_update_count(priv,
            pbdrv_gpio_input(&pdata->gpio_int), pbdrv_gpio_input(&pdata->gpio_dir));
    }
}

static const pbdrv_counter_funcs_t pbdrv_counter_stm32f0_gpio_quad_enc_funcs = {
    .get_count = pbdrv_counter_stm32f0_gpio_quad_enc_get_count,
};

void pbdrv_counter_stm32f0_gpio_quad_enc_init(pbdrv_counter_dev_t *devs) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        const pbdrv_counter_stm32f0_gpio_quad_enc_platform_data_t *pdata =
            &pbdrv_counter_stm32f0_gpio_quad_enc_platform_data[i];
        private_data_t *priv = &private_data[i];

        // TODO: may need to add pull to platform data if we add more platforms
        // that use this driver.

        pbdrv_gpio_input(&pdata->gpio_int);
        pbdrv_gpio_set_pull(&pdata->gpio_dir, PBDRV_GPIO_PULL_DOWN);
        pbdrv_gpio_input(&pdata->gpio_dir);

        priv->dev = &devs[pdata->counter_id];
        priv->dev->pdata = pdata;
        priv->dev->funcs = &pbdrv_counter_stm32f0_gpio_quad_enc_funcs;
        priv->dev->priv = priv;
    }

    // TODO: IRQ support should be added to gpio driver
    // for now, hard-coding interrupts for BOOST Move Hub

    // assign tacho pins to interrupts
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA | SYSCFG_EXTICR1_EXTI1_PB;
    EXTI->IMR |= EXTI_EMR_MR0 | EXTI_EMR_MR1;
    EXTI->RTSR |= EXTI_RTSR_RT0 | EXTI_RTSR_RT1;
    EXTI->FTSR |= EXTI_FTSR_FT0 | EXTI_FTSR_FT1;

    NVIC_SetPriority(EXTI0_1_IRQn, 1);
    NVIC_EnableIRQ(EXTI0_1_IRQn);
}

#endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
