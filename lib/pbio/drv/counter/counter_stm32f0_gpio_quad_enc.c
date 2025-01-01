// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2025 The Pybricks Authors

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
#include <pbdrv/ioport.h>
#include <pbio/util.h>

#include "stm32f0xx.h"
#include "counter.h"
#include "counter_stm32f0_gpio_quad_enc.h"

struct _pbdrv_counter_dev_t {
    const pbdrv_counter_stm32f0_gpio_quad_enc_platform_data_t *pdata;
};

static pbdrv_counter_dev_t counters[PBDRV_CONFIG_COUNTER_NUM_DEV];

static void pbdrv_counter_update_count(pbdrv_counter_dev_t *dev, bool int_pin_state, bool dir_pin_state) {
    // Callback not configured yet.
    if (!pbdrv_ioport_quadrature_increment_callback) {
        return;
    }

    if (int_pin_state ^ dir_pin_state) {
        pbdrv_ioport_quadrature_increment_callback(dev->pdata->counter_id, -1000);

    } else {
        pbdrv_ioport_quadrature_increment_callback(dev->pdata->counter_id, 1000);
    }
}

// irq handler name defined in startup_stm32f0.s
void EXTI0_1_IRQHandler(void) {
    uint32_t exti_pr;

    exti_pr = EXTI->PR & (EXTI_PR_PR0 | EXTI_PR_PR1);
    EXTI->PR = exti_pr; // clear the events we are handling

    // Port A - inverted.
    if (exti_pr & EXTI_PR_PR1) {
        pbdrv_counter_dev_t *dev = &counters[0];
        pbdrv_counter_update_count(dev, pbdrv_gpio_input(&dev->pdata->gpio_int), !pbdrv_gpio_input(&dev->pdata->gpio_dir));
    }

    // Port B
    if (exti_pr & EXTI_PR_PR0) {
        pbdrv_counter_dev_t *dev = &counters[1];
        pbdrv_counter_update_count(dev, pbdrv_gpio_input(&dev->pdata->gpio_int), pbdrv_gpio_input(&dev->pdata->gpio_dir));
    }
}

void pbdrv_counter_init(void) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(counters); i++) {

        pbdrv_counter_dev_t *dev = &counters[i];
        dev->pdata = &pbdrv_counter_stm32f0_gpio_quad_enc_platform_data[i];

        // TODO: may need to add pull to platform data if we add more platforms
        // that use this driver.

        pbdrv_gpio_input(&dev->pdata->gpio_int);
        pbdrv_gpio_set_pull(&dev->pdata->gpio_dir, PBDRV_GPIO_PULL_DOWN);
        pbdrv_gpio_input(&dev->pdata->gpio_dir);
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
