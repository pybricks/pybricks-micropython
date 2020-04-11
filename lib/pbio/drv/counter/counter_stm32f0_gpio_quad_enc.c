// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

// GPIO Quadrature Encoder Counter driver
//
// This driver uses GPIOs and a timer to create a quadrature encoder.
//
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

#define RING_BUF_SIZE 32 // must be power of 2!

typedef struct {
    pbdrv_counter_dev_t dev;
    int32_t counts[RING_BUF_SIZE];
    uint16_t timestamps[RING_BUF_SIZE];
    int32_t count;
    volatile uint8_t head;
    const pbdrv_gpio_t *gpio_int;
    const pbdrv_gpio_t *gpio_dir;
} private_data_t;

static private_data_t private_data[PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC_NUM_DEV];

static pbio_error_t pbdrv_counter_stm32f0_gpio_quad_enc_get_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    private_data_t *data = PBIO_CONTAINER_OF(dev, private_data_t, dev);

    *count = data->count;

    return PBIO_SUCCESS;
}

static pbio_error_t pbdrv_counter_stm32f0_gpio_quad_enc_get_rate(pbdrv_counter_dev_t *dev, int32_t *rate) {
    private_data_t *data = PBIO_CONTAINER_OF(dev, private_data_t, dev);
    int32_t head_count, tail_count = 0;
    uint16_t now, head_time, tail_time = 0;
    uint8_t head, tail, x = 0;

    // head can be updated in interrupt, so only read it once
    head = data->head;
    head_count = data->counts[head];
    head_time = data->timestamps[head];

    now = TIM7->CNT;

    // if it has been more than 50ms since last timestamp, we are not moving.
    if ((uint16_t)(now - head_time) > 50 * 100) {
        *rate = 0;
        return PBIO_SUCCESS;
    }

     while (x++ < RING_BUF_SIZE) {
        tail = (head - x) & (RING_BUF_SIZE - 1);

        tail_count = data->counts[tail];
        tail_time = data->timestamps[tail];

        // if count hasn't changed, then we are not moving
        if (head_count == tail_count) {
            *rate = 0;
            return PBIO_SUCCESS;
        }

        /*
         * we need delta_t to be >= 20ms to be reasonably accurate.
         * timer is 10us, thus * 100 to get ms.
         */
        if ((uint16_t)(head_time - tail_time) >= 20 * 100) {
            break;
        }
    }

    /* avoid divide by 0 - motor probably hasn't moved yet */
    if (head_time == tail_time) {
        *rate = 0;
        return PBIO_SUCCESS;
    }

    /* timer is 100000kHz */
    *rate = (head_count - tail_count) * 100000 / (uint16_t)(head_time - tail_time);
    return PBIO_SUCCESS;
}

static void pbdrv_motor_tacho_update_count(private_data_t *data,
                                           bool int_pin_state, bool dir_pin_state, uint16_t timestamp) {
    if (int_pin_state ^ dir_pin_state) {
        data->count--;
    }
    else {
        data->count++;
    }

    // log timestamp on rising edge for rate calculation
    if (int_pin_state) {
        uint8_t new_head = (data->head + 1) & (RING_BUF_SIZE - 1);

        data->counts[new_head] = data->count;
        data->timestamps[new_head] = timestamp;
        data->head = new_head;
    }
}

// irq handler name defined in startup_stm32f0.s
void EXTI0_1_IRQHandler(void) {
    uint32_t exti_pr;
    uint16_t timestamp;

    exti_pr = EXTI->PR & (EXTI_PR_PR0 | EXTI_PR_PR1);
    EXTI->PR = exti_pr; // clear the events we are handling

    timestamp = TIM7->CNT;

    if (exti_pr & EXTI_PR_PR1) {
        private_data_t *data = &private_data[0];
        pbdrv_motor_tacho_update_count(data, pbdrv_gpio_input(data->gpio_int), pbdrv_gpio_input(data->gpio_dir), timestamp);
    }

    if (exti_pr & EXTI_PR_PR0) {
        private_data_t *data = &private_data[1];
        pbdrv_motor_tacho_update_count(data, pbdrv_gpio_input(data->gpio_int), pbdrv_gpio_input(data->gpio_dir), timestamp);
    }
}

void TIM7_IRQHandler(void) {
    uint16_t timestamp;
    uint8_t i, new_head;

    TIM7->SR &= ~TIM_SR_UIF; // clear interrupt

    timestamp = TIM7->CNT;

    // log a new timestamp when the timer recycles to avoid rate calculation
    // problems when the motor is not moving
    for (i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        private_data_t *data = &private_data[i];
        new_head = (data->head + 1) & (RING_BUF_SIZE - 1);
        data->counts[new_head] = data->count;
        data->timestamps[new_head] = timestamp;
        data->head = new_head;
    }
}

static pbio_error_t counter_stm32f0_gpio_quad_enc_init() {
    for (int i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        const pbdrv_counter_stm32f0_gpio_quad_enc_platform_data_t *pdata =
            &pbdrv_counter_stm32f0_gpio_quad_enc_platform_data[i];
        private_data_t *data = &private_data[i];

        // TODO: may need to add pull to platform data if we add more platforms
        // that use this driver.

        data->gpio_int = &pdata->gpio_int;
        pbdrv_gpio_input(data->gpio_int);
        data->gpio_dir = &pdata->gpio_dir;
        pbdrv_gpio_set_pull(data->gpio_dir, PBDRV_GPIO_PULL_DOWN);
        pbdrv_gpio_input(data->gpio_dir);
        data->dev.get_count = pbdrv_counter_stm32f0_gpio_quad_enc_get_count;
        data->dev.get_rate = pbdrv_counter_stm32f0_gpio_quad_enc_get_rate;
        data->dev.initalized = true;
        pbdrv_counter_register(pdata->counter_id, &data->dev);
    }

    // TODO: IRQ support should be added to gpio driver
    // for now, hard-coding interrupts for BOOST Move Hub

    // assign tacho pins to interrupts
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA | SYSCFG_EXTICR1_EXTI1_PB;
    EXTI->IMR |= EXTI_EMR_MR0 | EXTI_EMR_MR1;
    EXTI->RTSR |= EXTI_RTSR_RT0 | EXTI_RTSR_RT1;
    EXTI->FTSR |= EXTI_FTSR_FT0 | EXTI_FTSR_FT1;

    NVIC_SetPriority(EXTI0_1_IRQn, 5);
    NVIC_EnableIRQ(EXTI0_1_IRQn);

    // TIM7 is used for clock in speed measurement
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->PSC = (PBDRV_CONFIG_SYS_CLOCK_RATE / 100000) - 1; // 100kHz
    TIM7->CR1 = TIM_CR1_CEN;
    TIM7->DIER = TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM7_IRQn);
    NVIC_SetPriority(TIM7_IRQn, 128);

    return PBIO_SUCCESS;
}

static pbio_error_t counter_stm32f0_gpio_quad_enc_exit() {
    for (int i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        private_data_t *data = &private_data[i];

        data->dev.initalized = false;
        pbdrv_counter_unregister(&data->dev);
        NVIC_DisableIRQ(TIM7_IRQn);
        NVIC_DisableIRQ(EXTI0_1_IRQn);
    }
    return PBIO_SUCCESS;
}

const pbdrv_counter_drv_t pbdrv_counter_stm32f0_gpio_quad_enc_drv = {
    .init   = counter_stm32f0_gpio_quad_enc_init,
    .exit   = counter_stm32f0_gpio_quad_enc_exit,
};

#endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
