// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// PWM driver using TI TLC5955 LED driver connected to STM32 MCU via SPI.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_TLC5955_STM32

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <contiki.h>

#include STM32_HAL_H

#include <pbdrv/core.h>
#include <pbdrv/pwm.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include "pwm_tlc5955_stm32.h"
#include "pwm.h"

// size in bytes to hold 769-bit shift register data
#define TLC5955_DATA_SIZE ((769 + 7) / 8)

// number of PWM channels on TLC5955
#define TLC5955_NUM_CHANNEL 48

/** Values for TLC5955_CONTROL_DATA maximum current parameter. */
enum {
    /** Max current: 3.2 mA */
    TLC5955_MC_3_2 = 0b000,
    /** Max current: 8.0 mA */
    TLC5955_MC_8_0 = 0b001,
    /** Max current: 11.2 mA */
    TLC5955_MC_11_2 = 0b010,
    /** Max current: 15.9 mA */
    TLC5955_MC_15_9 = 0b011,
    /** Max current: 19.1 mA */
    TLC5955_MC_19_1 = 0b100,
    /** Max current: 23.9 mA */
    TLC5955_MC_23_9 = 0b101,
    /** Max current: 27.1 mA */
    TLC5955_MC_27_1 = 0b110,
    /** Max current: 31.9 mA */
    TLC5955_MC_31_9 = 0b111,
};

/**
 * Control data latch initializer
 *
 * Note: we are using this as generic PWM, so red/green/blue are all configured
 * the same.
 *
 * @param name      variable name
 * @param dc        dot correction (7-bit)
 * @param mc        maximum current (3-bit)
 * @param bc        global brightness control (7-bit)
 * @param dsprpt    Auto display repeat mode enable (1-bit)
 * @param tmgrst    Display timing reset mode enable (1-bit)
 * @param rfresh    Auto data refresh mode enable (1-bit)
 * @param espwm     ES-PWM mode enable (1-bit)
 * @param lsdvlt    LSD detection voltage selection (1-bit)
 */
#define TLC5955_CONTROL_DATA(name, dc, mc, bc, dsprpt, tmgrst, rfresh, espwm, lsdvlt) \
    uint8_t name[TLC5955_DATA_SIZE] = { \
        /* bit 768 */      [0] = 1, \
        /* bits 767-760 */ [1] = 0x96, \
        /* bits 370-368 */ [50] = (((lsdvlt) << 2) | ((espwm) << 1) | (rfresh)) & 0xff, \
        /* bits 367-360 */ [51] = (((tmgrst) << 7) | ((dsprpt) << 6) | ((bc) >> 1)) & 0xff, \
        /* bits 359-352 */ [52] = (((bc) << 7) | ((bc) >> 0)) & 0xff, \
        /* bits 351-344 */ [53] = (((bc) << 1) | ((mc) >> 2)) & 0xff, \
        /* bits 343-336 */ [54] = (((mc) << 6) | ((mc) << 3) | (mc)) & 0xff, \
        /* bits 335-328 */ [55] = (((dc) << 1) | ((dc) >> 6)) & 0xff, \
        /* bits 327-320 */ [56] = (((dc) << 2) | ((dc) >> 5)) & 0xff, \
        /* bits 319-312 */ [57] = (((dc) << 3) | ((dc) >> 4)) & 0xff, \
        /* bits 311-304 */ [58] = (((dc) << 4) | ((dc) >> 3)) & 0xff, \
        /* bits 303-296 */ [59] = (((dc) << 5) | ((dc) >> 2)) & 0xff, \
        /* bits 295-288 */ [60] = (((dc) << 6) | ((dc) >> 1)) & 0xff, \
        /* bits 287-280 */ [61] = (((dc) << 7) | ((dc) >> 0)) & 0xff, \
        /* bits 279-272 */ [62] = (((dc) << 1) | ((dc) >> 6)) & 0xff, \
        /* bits 271-264 */ [63] = (((dc) << 2) | ((dc) >> 5)) & 0xff, \
        /* bits 263-256 */ [64] = (((dc) << 3) | ((dc) >> 4)) & 0xff, \
        /* bits 255-248 */ [65] = (((dc) << 4) | ((dc) >> 3)) & 0xff, \
        /* bits 247-240 */ [66] = (((dc) << 5) | ((dc) >> 2)) & 0xff, \
        /* bits 239-232 */ [67] = (((dc) << 6) | ((dc) >> 1)) & 0xff, \
        /* bits 231-224 */ [68] = (((dc) << 7) | ((dc) >> 0)) & 0xff, \
        /* bits 223-216 */ [69] = (((dc) << 1) | ((dc) >> 6)) & 0xff, \
        /* bits 215-208 */ [70] = (((dc) << 2) | ((dc) >> 5)) & 0xff, \
        /* bits 207-200 */ [71] = (((dc) << 3) | ((dc) >> 4)) & 0xff, \
        /* bits 199-192 */ [72] = (((dc) << 4) | ((dc) >> 3)) & 0xff, \
        /* bits 191-184 */ [73] = (((dc) << 5) | ((dc) >> 2)) & 0xff, \
        /* bits 183-176 */ [74] = (((dc) << 6) | ((dc) >> 1)) & 0xff, \
        /* bits 175-168 */ [75] = (((dc) << 7) | ((dc) >> 0)) & 0xff, \
        /* bits 167-160 */ [76] = (((dc) << 1) | ((dc) >> 6)) & 0xff, \
        /* bits 159-152 */ [77] = (((dc) << 2) | ((dc) >> 5)) & 0xff, \
        /* bits 151-144 */ [78] = (((dc) << 3) | ((dc) >> 4)) & 0xff, \
        /* bits 143-136 */ [79] = (((dc) << 4) | ((dc) >> 3)) & 0xff, \
        /* bits 135-128 */ [80] = (((dc) << 5) | ((dc) >> 2)) & 0xff, \
        /* bits 127-120 */ [81] = (((dc) << 6) | ((dc) >> 1)) & 0xff, \
        /* bits 119-112 */ [82] = (((dc) << 7) | ((dc) >> 0)) & 0xff, \
        /* bits 111-104 */ [83] = (((dc) << 1) | ((dc) >> 6)) & 0xff, \
        /* bits 103-96 */  [84] = (((dc) << 2) | ((dc) >> 5)) & 0xff, \
        /* bits 95-88 */   [85] = (((dc) << 3) | ((dc) >> 4)) & 0xff, \
        /* bits 87-80 */   [86] = (((dc) << 4) | ((dc) >> 3)) & 0xff, \
        /* bits 79-72 */   [87] = (((dc) << 5) | ((dc) >> 2)) & 0xff, \
        /* bits 71-64 */   [88] = (((dc) << 6) | ((dc) >> 1)) & 0xff, \
        /* bits 63-56 */   [89] = (((dc) << 7) | ((dc) >> 0)) & 0xff, \
        /* bits 55-48 */   [90] = (((dc) << 1) | ((dc) >> 6)) & 0xff, \
        /* bits 47-40 */   [91] = (((dc) << 2) | ((dc) >> 5)) & 0xff, \
        /* bits 39-32 */   [92] = (((dc) << 3) | ((dc) >> 4)) & 0xff, \
        /* bits 31-24 */   [93] = (((dc) << 4) | ((dc) >> 3)) & 0xff, \
        /* bits 23-16 */   [94] = (((dc) << 5) | ((dc) >> 2)) & 0xff, \
        /* bits 15-8 */    [95] = (((dc) << 6) | ((dc) >> 1)) & 0xff, \
        /* bits 7-0 */     [96] = (((dc) << 7) | ((dc) >> 0)) & 0xff, \
    }

typedef enum {
    DEINIT_NOT_STARTED,
    DEINIT_STARTED,
    DEINIT_DONE,
} deinit_t;

typedef struct {
    /** HAL SPI data */
    SPI_HandleTypeDef hspi;
    /** HAL Rx DMA data */
    DMA_HandleTypeDef hdma_rx;
    /** HAL Tx DMA data */
    DMA_HandleTypeDef hdma_tx;
    /** Protothread */
    struct pt pt;
    /** Pointer to generic PWM device instance */
    pbdrv_pwm_dev_t *pwm;
    /** Grayscale latch register data */
    uint8_t *grayscale_latch;
    /** grayscale value has changed, update needed */
    bool changed;
    /** syncronization state for deinit */
    deinit_t deinit;
} pbdrv_pwm_tlc5955_stm32_priv_t;

PROCESS(pwm_tlc5955_stm32, "pwm_tlc5955_stm32");
static pbdrv_pwm_tlc5955_stm32_priv_t dev_priv[PBDRV_CONFIG_PWM_TLC5955_STM32_NUM_DEV];

// Control latch data setting dot correction to 100%, max current to 3.2 mA,
// global brightness to 100%, auto repeat enabled, display timing reset disabled,
// auto refresh disabled, ES-PWM mode enabled, LSD detection voltage 90%.
static const TLC5955_CONTROL_DATA(control_latch_3mA, 127, TLC5955_MC_3_2, 127, 1, 0, 0, 1, 1);

static uint8_t grayscale_latch[PBDRV_CONFIG_PWM_TLC5955_STM32_NUM_DEV][TLC5955_DATA_SIZE];

// channels are mapped to GS registers in reverse order. CH 0: GSB15, CH 1: GSG15,
// CH 2: GSR15 ... CH 45: GSB0, CH 46: GSG0, CH 47: GSR0
static pbio_error_t pbdrv_pwm_tlc5955_stm32_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    pbdrv_pwm_tlc5955_stm32_priv_t *priv = dev->priv;

    assert(ch < TLC5955_NUM_CHANNEL);
    assert(value <= UINT16_MAX);

    priv->grayscale_latch[ch * 2 + 1] = value >> 8;
    priv->grayscale_latch[ch * 2 + 2] = value;
    priv->changed = true;
    process_poll(&pwm_tlc5955_stm32);

    return PBIO_SUCCESS;
}

static const pbdrv_pwm_driver_funcs_t pbdrv_pwm_tlc5955_stm32_funcs = {
    .set_duty = pbdrv_pwm_tlc5955_stm32_set_duty,
};

void pbdrv_pwm_tlc5955_stm32_init(pbdrv_pwm_dev_t *devs) {
    for (int i = 0; i < PBDRV_CONFIG_PWM_TLC5955_STM32_NUM_DEV; i++) {
        const pbdrv_pwm_tlc5955_stm32_platform_data_t *pdata = &pbdrv_pwm_tlc5955_stm32_platform_data[i];
        pbdrv_pwm_dev_t *pwm = &devs[pdata->id];
        pbdrv_pwm_tlc5955_stm32_priv_t *priv = &dev_priv[i];

        HAL_GPIO_WritePin(pdata->lat_gpio, pdata->lat_gpio_pin, GPIO_PIN_RESET);
        GPIO_InitTypeDef gpio_init;
        gpio_init.Pin = pdata->lat_gpio_pin;
        gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_LOW;
        HAL_GPIO_Init(pdata->lat_gpio, &gpio_init);

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

        priv->hspi.Instance = pdata->spi;
        priv->hspi.Init.Mode = SPI_MODE_MASTER;
        priv->hspi.Init.Direction = SPI_DIRECTION_2LINES;
        priv->hspi.Init.DataSize = SPI_DATASIZE_8BIT;
        priv->hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
        priv->hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
        priv->hspi.Init.NSS = SPI_NSS_SOFT;
        priv->hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
        priv->hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
        priv->hspi.Init.TIMode = SPI_TIMODE_DISABLE;
        priv->hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
        priv->hspi.Init.CRCPolynomial = 0;
        HAL_SPI_Init(&priv->hspi);

        __HAL_LINKDMA(&priv->hspi, hdmarx, priv->hdma_rx);
        __HAL_LINKDMA(&priv->hspi, hdmatx, priv->hdma_tx);

        HAL_NVIC_SetPriority(pdata->spi_irq, 7, 2);
        HAL_NVIC_EnableIRQ(pdata->spi_irq);

        PT_INIT(&priv->pt);
        priv->pwm = pwm;
        priv->grayscale_latch = grayscale_latch[i];
        pwm->pdata = pdata;
        pwm->priv = priv;
        // don't set funcs yet since we are not fully intialized
        pbdrv_init_busy_up();
    }

    process_start(&pwm_tlc5955_stm32, NULL);
}

void pbdrv_pwm_tlc5955_stm32_deinit(pbdrv_pwm_dev_t *devs) {

    for (int i = 0; i < PBDRV_CONFIG_PWM_TLC5955_STM32_NUM_DEV; i++) {
        pbdrv_pwm_tlc5955_stm32_priv_t *priv = &dev_priv[i];
        for (int ch = 0; ch < TLC5955_NUM_CHANNEL; ch++) {
            pbdrv_pwm_tlc5955_stm32_set_duty(priv->pwm, ch, 0);
        }
        priv->deinit = DEINIT_STARTED;
        pbdrv_deinit_busy_up();
    }
}

// toggles LAT signal on and off to latch data in shift register
static void pbdrv_pwm_tlc5955_toggle_latch(pbdrv_pwm_tlc5955_stm32_priv_t *priv) {
    const pbdrv_pwm_tlc5955_stm32_platform_data_t *pdata = priv->pwm->pdata;
    HAL_GPIO_WritePin(pdata->lat_gpio, pdata->lat_gpio_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(pdata->lat_gpio, pdata->lat_gpio_pin, GPIO_PIN_RESET);
}

static PT_THREAD(pbdrv_pwm_tlc5955_stm32_handle_event(pbdrv_pwm_tlc5955_stm32_priv_t * priv, process_event_t ev)) {
    PT_BEGIN(&priv->pt);

    HAL_SPI_Transmit_DMA(&priv->hspi, (uint8_t *)control_latch_3mA, TLC5955_DATA_SIZE);
    PT_WAIT_UNTIL(&priv->pt, priv->hspi.State == HAL_SPI_STATE_READY);
    pbdrv_pwm_tlc5955_toggle_latch(priv);

    // have to send twice for max current to take effect
    HAL_SPI_Transmit_DMA(&priv->hspi, (uint8_t *)control_latch_3mA, TLC5955_DATA_SIZE);
    PT_WAIT_UNTIL(&priv->pt, priv->hspi.State == HAL_SPI_STATE_READY);
    pbdrv_pwm_tlc5955_toggle_latch(priv);

    // initialization is finished so consumers can use this PWM device now.
    priv->pwm->funcs = &pbdrv_pwm_tlc5955_stm32_funcs;
    pbdrv_init_busy_down();

    for (;;) {
        PT_WAIT_UNTIL(&priv->pt, priv->changed);
        HAL_SPI_Transmit_DMA(&priv->hspi, priv->grayscale_latch, TLC5955_DATA_SIZE);
        priv->changed = false;
        PT_WAIT_UNTIL(&priv->pt, priv->hspi.State == HAL_SPI_STATE_READY);
        pbdrv_pwm_tlc5955_toggle_latch(priv);
        if (priv->deinit == DEINIT_STARTED && !priv->changed) {
            // if deinit has been requested and there are no more pending changes
            // then we can say deint is done
            priv->deinit = DEINIT_DONE;
            pbdrv_deinit_busy_down();
            break;
        }
    }

    PT_END(&priv->pt);
}

/**
 * Interupt handler for Rx DMA IRQ. Needs to be called from IRQ handler in platform.c.
 */
void pbdrv_pwm_tlc5955_stm32_rx_dma_irq(uint8_t index) {
    pbdrv_pwm_tlc5955_stm32_priv_t *priv = &dev_priv[index];

    HAL_DMA_IRQHandler(&priv->hdma_rx);
}

/**
 * Interupt handler for Tx DMA IRQ. Needs to be called from IRQ handler in platform.c.
 */
void pbdrv_pwm_tlc5955_stm32_tx_dma_irq(uint8_t index) {
    pbdrv_pwm_tlc5955_stm32_priv_t *priv = &dev_priv[index];

    HAL_DMA_IRQHandler(&priv->hdma_tx);
}

/**
 * Interupt handler for SPI IRQ. Needs to be called from IRQ handler in platform.c.
 */
void pbdrv_pwm_tlc5955_stm32_spi_irq(uint8_t index) {
    pbdrv_pwm_tlc5955_stm32_priv_t *priv = &dev_priv[index];

    HAL_SPI_IRQHandler(&priv->hspi);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    process_poll(&pwm_tlc5955_stm32);
}

PROCESS_THREAD(pwm_tlc5955_stm32, ev, data) {
    PROCESS_BEGIN();

    // need to allow all drivers to init first
    PROCESS_PAUSE();

    for (;;) {
        for (int i = 0; i < PBDRV_CONFIG_PWM_TLC5955_STM32_NUM_DEV; i++) {
            pbdrv_pwm_tlc5955_stm32_priv_t *priv = &dev_priv[i];
            if (priv->deinit != DEINIT_DONE) {
                pbdrv_pwm_tlc5955_stm32_handle_event(priv, ev);
            }
        }
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_PWM_TLC5955_STM32
