// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// PWM driver using TI LP50XX LED driver connected to STM32 MCU via FMPI2C.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_LP50XX_STM32

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <contiki.h>
#include STM32_HAL_H

#include <pbdrv/pwm.h>
#include <pbio/error.h>
#include <pbio/util.h>

#include "../core.h"
#include "pwm_lp50xx_stm32.h"
#include "pwm.h"

#define I2C_ADDR 0x28

// Registers
#define DEVICE_CONFIG0          0x00
#define DEVICE_CONFIG1          0x01
#define LED_CONFIG0             0x02
#define BANK_BRIGHTNESS         0x03
#define BANK_A_COLOR            0x04
#define BANK_B_COLOR            0x05
#define BANK_C_COLOR            0x06
#define LED0_BRIGHTNESS         0x07
#define LED1_BRIGHTNESS         0x08
#define LED2_BRIGHTNESS         0x09
#define LED3_BRIGHTNESS         0x0A
#define OUT0_COLOR              0x0B
#define OUT1_COLOR              0x0C
#define OUT2_COLOR              0x0D
#define OUT3_COLOR              0x0E
#define OUT4_COLOR              0x0F
#define OUT5_COLOR              0x10
#define OUT6_COLOR              0x11
#define OUT7_COLOR              0x12
#define OUT8_COLOR              0x13
#define OUT9_COLOR              0x14
#define OUT10_COLOR             0x15
#define OUT11_COLOR             0x16
#define RESET                   0x17

// Flags
#define DEVICE_CONFIG0_CHIP_EN              (1 << 6)
#define DEVICE_CONFIG1_LOG_SCALE_EN         (1 << 5)
#define DEVICE_CONFIG1_POWER_SAVE_EN        (1 << 4)
#define DEVICE_CONFIG1_AUTO_INCR_EN         (1 << 3)
#define DEVICE_CONFIG1_PWM_DITHERING_EN     (1 << 2)
#define DEVICE_CONFIG1_MAX_CURRENT_OPTION   (1 << 1)
#define DEVICE_CONFIG1_LED_GLOBAL_OFF       (1 << 0)
#define LED_CONFIG0_LED3_BANK_EN            (1 << 3)
#define LED_CONFIG0_LED2_BANK_EN            (1 << 2)
#define LED_CONFIG0_LED1_BANK_EN            (1 << 1)
#define LED_CONFIG0_LED0_BANK_EN            (1 << 0)

// technically, these chips can have up to 12 channels, but we are currently
// only using 6
#define LP50XX_NUM_CH 6

// REVISIT: this is for STM32F413 - may be different for other CPUs. There is
// currently no equivelent of __LL_I2C_CONVERT_TIMINGS for FMPI2C in the STM32 HAL.
#define FMPI2C_CONVERT_TIMINGS(PRESC, SCLDEL, SDADEL, SCLH, SCLL) \
    (((PRESC) << FMPI2C_TIMINGR_PRESC_Pos) | \
    ((SCLDEL) << FMPI2C_TIMINGR_SCLDEL_Pos) | \
    ((SDADEL) << FMPI2C_TIMINGR_SDADEL_Pos) | \
    ((SCLH) << FMPI2C_TIMINGR_SCLH_Pos) | \
    ((SCLL) << FMPI2C_TIMINGR_SCLL_Pos))

typedef struct {
    /** HAL FMPI2C data */
    FMPI2C_HandleTypeDef hfmpi2c;
    /** HAL Rx DMA data */
    DMA_HandleTypeDef hdma_rx;
    /** HAL Tx DMA data */
    DMA_HandleTypeDef hdma_tx;
    /** Protothread */
    struct pt pt;
    /** Pointer to generic PWM device instance */
    pbdrv_pwm_dev_t *pwm;
    /** PWM values */
    uint8_t values[LP50XX_NUM_CH];
    /** value has changed, update needed */
    bool changed;
} pbdrv_pwm_lp50xx_stm32_priv_t;

PROCESS(pwm_lp50xx_stm32, "pwm_lp50xx_stm32");
static pbdrv_pwm_lp50xx_stm32_priv_t dev_priv[PBDRV_CONFIG_PWM_LP50XX_STM32_NUM_DEV];

static pbio_error_t pbdrv_pwm_lp50xx_stm32_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    pbdrv_pwm_lp50xx_stm32_priv_t *priv = dev->priv;

    assert(ch < LP50XX_NUM_CH);
    assert(value <= UINT16_MAX);

    // Currently all LED PWMs use 16-bit value. This chip only has 8-bit PWM
    // (the data sheet says 12-bit PWM but the I2C registers are only 8-bit).
    priv->values[ch] = value >> 8;
    priv->changed = true;
    process_poll(&pwm_lp50xx_stm32);

    return PBIO_SUCCESS;
}

static const pbdrv_pwm_driver_funcs_t pbdrv_pwm_lp50xx_stm32_funcs = {
    .set_duty = pbdrv_pwm_lp50xx_stm32_set_duty,
};

void pbdrv_pwm_lp50xx_stm32_init(pbdrv_pwm_dev_t *devs) {
    for (int i = 0; i < PBDRV_CONFIG_PWM_LP50XX_STM32_NUM_DEV; i++) {
        const pbdrv_pwm_lp50xx_stm32_platform_data_t *pdata = &pbdrv_pwm_lp50xx_stm32_platform_data[i];
        pbdrv_pwm_dev_t *pwm = &devs[pdata->id];
        pbdrv_pwm_lp50xx_stm32_priv_t *priv = &dev_priv[i];

        GPIO_InitTypeDef gpio_init;
        gpio_init.Pin = pdata->en_gpio_pin;
        gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_LOW;
        HAL_GPIO_Init(pdata->en_gpio, &gpio_init);
        HAL_GPIO_WritePin(pdata->en_gpio, pdata->en_gpio_pin, GPIO_PIN_SET);

        priv->hdma_rx.Instance = pdata->rx_dma;
        priv->hdma_rx.Init.Channel = pdata->rx_dma_ch;
        priv->hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        priv->hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        priv->hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
        priv->hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        priv->hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        priv->hdma_rx.Init.Mode = DMA_NORMAL;
        priv->hdma_rx.Init.Priority = DMA_PRIORITY_LOW;
        priv->hdma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        priv->hdma_rx.Init.MemBurst = DMA_MBURST_SINGLE;
        priv->hdma_rx.Init.PeriphBurst = DMA_PBURST_SINGLE;
        HAL_DMA_Init(&priv->hdma_rx);

        HAL_NVIC_SetPriority(pdata->rx_dma_irq, 7, 0);
        HAL_NVIC_EnableIRQ(pdata->rx_dma_irq);

        priv->hdma_tx.Instance = pdata->tx_dma;
        priv->hdma_tx.Init.Channel = pdata->tx_dma_ch;
        priv->hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        priv->hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        priv->hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
        priv->hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        priv->hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        priv->hdma_tx.Init.Mode = DMA_NORMAL;
        priv->hdma_tx.Init.Priority = DMA_PRIORITY_LOW;
        priv->hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        priv->hdma_tx.Init.MemBurst = DMA_MBURST_SINGLE;
        priv->hdma_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;
        HAL_DMA_Init(&priv->hdma_tx);

        HAL_NVIC_SetPriority(pdata->tx_dma_irq, 7, 1);
        HAL_NVIC_EnableIRQ(pdata->tx_dma_irq);

        // REVISIT: Timings are hard-coded for 48MHz FMPI2CCLK.
        // Official LEGO firmware/upstream MicroPython (default) has 24MHz FMPI2CCLK
        // so we have changed the prescalar to account for the difference.
        priv->hfmpi2c.Instance = pdata->i2c;
        priv->hfmpi2c.Init.Timing = FMPI2C_CONVERT_TIMINGS(1, 4, 0, 19, 28);
        priv->hfmpi2c.Init.OwnAddress1 = 0;
        priv->hfmpi2c.Init.AddressingMode = FMPI2C_ADDRESSINGMODE_7BIT;
        priv->hfmpi2c.Init.DualAddressMode = FMPI2C_DUALADDRESS_DISABLE;
        priv->hfmpi2c.Init.OwnAddress2 = 0;
        priv->hfmpi2c.Init.OwnAddress2Masks = FMPI2C_OA2_NOMASK;
        priv->hfmpi2c.Init.GeneralCallMode = FMPI2C_GENERALCALL_DISABLE;
        priv->hfmpi2c.Init.NoStretchMode = FMPI2C_NOSTRETCH_DISABLE;
        HAL_FMPI2C_Init(&priv->hfmpi2c);

        __HAL_LINKDMA(&priv->hfmpi2c, hdmarx, priv->hdma_rx);
        __HAL_LINKDMA(&priv->hfmpi2c, hdmatx, priv->hdma_tx);

        HAL_NVIC_SetPriority(pdata->i2c_ev_irq, 7, 2);
        HAL_NVIC_EnableIRQ(pdata->i2c_ev_irq);
        HAL_NVIC_SetPriority(pdata->i2c_er_irq, 7, 3);
        HAL_NVIC_EnableIRQ(pdata->i2c_er_irq);

        PT_INIT(&priv->pt);
        priv->pwm = pwm;
        pwm->pdata = pdata;
        pwm->priv = priv;
        // don't set funcs yet since we are not fully initialized
        pbdrv_init_busy_up();
    }

    process_start(&pwm_lp50xx_stm32);
}

static PT_THREAD(pbdrv_pwm_lp50xx_stm32_handle_event(pbdrv_pwm_lp50xx_stm32_priv_t * priv, process_event_t ev)) {
    PT_BEGIN(&priv->pt);

    static const struct {
        uint8_t reg;
        uint8_t values[11];
    } __attribute__((packed)) init_data = {
        .reg = DEVICE_CONFIG0,
        .values = {
            [DEVICE_CONFIG0] = DEVICE_CONFIG0_CHIP_EN,
            [DEVICE_CONFIG1] = DEVICE_CONFIG1_POWER_SAVE_EN | DEVICE_CONFIG1_PWM_DITHERING_EN | DEVICE_CONFIG1_AUTO_INCR_EN,
            [LED_CONFIG0] = 0,
            [BANK_BRIGHTNESS] = 0,
            [BANK_A_COLOR] = 0,
            [BANK_B_COLOR] = 0,
            [BANK_C_COLOR] = 0,
            // Official LEGO firmware has LED0_BRIGHTNESS set to 255 and LED1_BRIGHTNESS
            // set to 190 but then divides the incoming PWM duty cycle by 5. By doing
            // the divide by 5 here, we end up with the same max brightness while
            // allowing full dynamic range of the PWM duty cycle.
            [LED0_BRIGHTNESS] = 51,
            [LED1_BRIGHTNESS] = 38,
            [LED2_BRIGHTNESS] = 0,
            [LED3_BRIGHTNESS] = 0,
        }
    };

    HAL_FMPI2C_Master_Transmit_DMA(&priv->hfmpi2c, I2C_ADDR, (void *)&init_data, sizeof(init_data));
    PT_WAIT_UNTIL(&priv->pt, HAL_FMPI2C_GetState(&priv->hfmpi2c) == HAL_FMPI2C_STATE_READY);

    // initialization is finished so consumers can use this PWM device now.
    priv->pwm->funcs = &pbdrv_pwm_lp50xx_stm32_funcs;
    pbdrv_init_busy_down();

    for (;;) {
        PT_WAIT_UNTIL(&priv->pt, priv->changed);

        static struct {
            uint8_t reg;
            uint8_t values[LP50XX_NUM_CH];
        } __attribute__((packed)) color_data = {
            .reg = OUT0_COLOR,
        };

        memcpy(color_data.values, priv->values, LP50XX_NUM_CH);
        HAL_FMPI2C_Master_Transmit_DMA(&priv->hfmpi2c, I2C_ADDR, (void *)&color_data, sizeof(color_data));
        priv->changed = false;
        PT_WAIT_UNTIL(&priv->pt, HAL_FMPI2C_GetState(&priv->hfmpi2c) == HAL_FMPI2C_STATE_READY);
    }

    PT_END(&priv->pt);
}

/**
 * Interrupt handler for Rx DMA IRQ. Needs to be called from IRQ handler in platform.c.
 */
void pbdrv_pwm_lp50xx_stm32_rx_dma_irq(uint8_t index) {
    pbdrv_pwm_lp50xx_stm32_priv_t *priv = &dev_priv[index];

    HAL_DMA_IRQHandler(&priv->hdma_rx);
}

/**
 * Interrupt handler for Tx DMA IRQ. Needs to be called from IRQ handler in platform.c.
 */
void pbdrv_pwm_lp50xx_stm32_tx_dma_irq(uint8_t index) {
    pbdrv_pwm_lp50xx_stm32_priv_t *priv = &dev_priv[index];

    HAL_DMA_IRQHandler(&priv->hdma_tx);
}

/**
 * Interrupt handler for I2C EV IRQ. Needs to be called from IRQ handler in platform.c.
 */
void pbdrv_pwm_lp50xx_stm32_i2c_ev_irq(uint8_t index) {
    pbdrv_pwm_lp50xx_stm32_priv_t *priv = &dev_priv[index];

    HAL_FMPI2C_EV_IRQHandler(&priv->hfmpi2c);
}

/**
 * Interrupt handler for I2C ER IRQ. Needs to be called from IRQ handler in platform.c.
 */
void pbdrv_pwm_lp50xx_stm32_i2c_er_irq(uint8_t index) {
    pbdrv_pwm_lp50xx_stm32_priv_t *priv = &dev_priv[index];

    HAL_FMPI2C_ER_IRQHandler(&priv->hfmpi2c);
}

void HAL_FMPI2C_MasterTxCpltCallback(FMPI2C_HandleTypeDef *hfmpi2c) {
    process_poll(&pwm_lp50xx_stm32);
}

PROCESS_THREAD(pwm_lp50xx_stm32, ev, data) {
    PROCESS_BEGIN();

    // need to allow all drivers to init first
    PROCESS_PAUSE();

    for (;;) {
        for (int i = 0; i < PBDRV_CONFIG_PWM_LP50XX_STM32_NUM_DEV; i++) {
            pbdrv_pwm_lp50xx_stm32_priv_t *priv = &dev_priv[i];
            pbdrv_pwm_lp50xx_stm32_handle_event(priv, ev);
        }
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_PWM_LP50XX_STM32
