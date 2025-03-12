// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2023 The Pybricks Authors

#include <stdbool.h>

#include <pbdrv/clock.h>
#include <pbdrv/ioport.h>
#include <pbio/port_interface.h>

#include "../../drv/adc/adc_stm32_hal.h"
#include "../../drv/battery/battery_adc.h"
#include "../../drv/bluetooth/bluetooth_stm32_cc2640.h"
#include "../../drv/button/button_gpio.h"
#include "../../drv/imu/imu_lsm6ds3tr_c_stm32.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/motor_driver/motor_driver_hbridge_pwm.h"
#include "../../drv/pwm/pwm_stm32_tim.h"
#include "../../drv/uart/uart_stm32l4_ll_dma.h"

#include "stm32l4xx_hal.h"
#include "stm32l4xx_ll_dma.h"
#include "stm32l4xx_ll_rcc.h"

enum {
    COUNTER_PORT_A,
    COUNTER_PORT_B,
    COUNTER_PORT_C,
    COUNTER_PORT_D,
};

enum {
    LED_DEV_0,
};

enum {
    PWM_DEV_0,
    PWM_DEV_1,
    PWM_DEV_2,
    PWM_DEV_3,
};

enum {
    UART_PORT_A,
    UART_PORT_B,
    UART_PORT_C,
    UART_PORT_D,
};

// PBIO driver data

// Battery

const pbdrv_battery_adc_platform_data_t pbdrv_battery_adc_platform_data = {
    .gpio = { .bank = GPIOA, .pin = 12 },
    .pull = PBDRV_GPIO_PULL_NONE,
};

// Bluetooth

// REVISIT: more of this could be in driver if we enabled HAL on City hub.

// bluetooth address is set at factory at this address
#define FLASH_BD_ADDR ((const uint8_t *)0x08007ff0)

static SPI_HandleTypeDef bt_spi;

static void bt_spi_init(void) {
    GPIO_InitTypeDef gpio_init;

    // Implied defaults: no pull, low speed, alternate function 0

    // nSRDY
    gpio_init.Pin = GPIO_PIN_13;
    gpio_init.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 2);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    bt_spi.Instance = SPI1;
    bt_spi.Init.Mode = SPI_MODE_MASTER;
    bt_spi.Init.Direction = SPI_DIRECTION_2LINES;
    bt_spi.Init.DataSize = SPI_DATASIZE_8BIT;
    bt_spi.Init.CLKPolarity = SPI_POLARITY_LOW;
    bt_spi.Init.CLKPhase = SPI_PHASE_1EDGE;
    bt_spi.Init.NSS = SPI_NSS_SOFT;
    bt_spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    bt_spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    bt_spi.Init.TIMode = SPI_TIMODE_DISABLE;
    bt_spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    bt_spi.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    HAL_SPI_Init(&bt_spi);
}

static void bt_spi_start_xfer(const uint8_t *tx_buf, uint8_t *rx_buf, uint8_t xfer_size) {
    HAL_SPI_TransmitReceive_DMA(&bt_spi, (uint8_t *)tx_buf, rx_buf, xfer_size);
}

const pbdrv_bluetooth_stm32_cc2640_platform_data_t pbdrv_bluetooth_stm32_cc2640_platform_data = {
    .bd_addr = FLASH_BD_ADDR,
    .reset_gpio = { .bank = GPIOD, .pin = 2 },
    .mrdy_gpio = { .bank = GPIOA, .pin = 15 },
    .spi_init = bt_spi_init,
    .spi_start_xfer = bt_spi_start_xfer,
};

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
    GPIO_InitTypeDef gpio_init;
    static DMA_HandleTypeDef rx_dma, tx_dma;

    gpio_init.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Alternate = GPIO_AF5_SPI1;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    tx_dma.Instance = DMA2_Channel4;
    tx_dma.Init.Request = DMA_REQUEST_4;
    tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    tx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    tx_dma.Init.MemInc = DMA_MINC_ENABLE;
    tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    tx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    tx_dma.Init.Mode = DMA_NORMAL;
    tx_dma.Init.Priority = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&tx_dma);
    __HAL_LINKDMA(hspi, hdmatx, tx_dma);

    HAL_NVIC_SetPriority(DMA2_Channel4_IRQn, 5, 1);
    HAL_NVIC_EnableIRQ(DMA2_Channel4_IRQn);

    rx_dma.Instance = DMA2_Channel3;
    rx_dma.Init.Request = DMA_REQUEST_4;
    rx_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    rx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    rx_dma.Init.MemInc = DMA_MINC_ENABLE;
    rx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    rx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    rx_dma.Init.Mode = DMA_NORMAL;
    rx_dma.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&rx_dma);
    __HAL_LINKDMA(hspi, hdmarx, rx_dma);

    HAL_NVIC_SetPriority(DMA2_Channel3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Channel3_IRQn);
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi) {
    HAL_NVIC_DisableIRQ(DMA2_Channel3_IRQn);
    HAL_NVIC_DisableIRQ(DMA2_Channel4_IRQn);
    HAL_DMA_DeInit(hspi->hdmarx);
    HAL_DMA_DeInit(hspi->hdmatx);
}

void DMA2_Channel3_IRQHandler(void) {
    HAL_DMA_IRQHandler(bt_spi.hdmarx);
}

void DMA2_Channel4_IRQHandler(void) {
    HAL_DMA_IRQHandler(bt_spi.hdmatx);
}

void EXTI15_10_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_9) {
        pbdrv_imu_lsm6ds3tr_c_stm32_handle_int1_irq();
    } else if (pin == GPIO_PIN_13) {
        pbdrv_bluetooth_stm32_cc2640_srdy_irq(!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13));
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    pbdrv_bluetooth_stm32_cc2640_spi_xfer_irq();
}

// Button

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio = { .bank = GPIOC, .pin = 14 },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_CENTER,
        .active_low = true,
    },
};

// IMU

const pbdrv_imu_lsm6s3tr_c_stm32_platform_data_t pbdrv_imu_lsm6s3tr_c_stm32_platform_data = {
    .i2c = I2C1,
};

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    GPIO_InitTypeDef gpio_init;

    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    // do a quick bus reset in case IMU chip is in bad state
    gpio_init.Pin = GPIO_PIN_8; // SCL
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    for (int i = 0; i < 10; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        pbdrv_clock_delay_us(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        pbdrv_clock_delay_us(1);
    }

    // then configure for normal use
    gpio_init.Pin = GPIO_PIN_8 | GPIO_PIN_9; // SCL | SDA
    gpio_init.Mode = GPIO_MODE_AF_OD;
    gpio_init.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 3, 1);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 3, 2);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);

    // INT1
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Pin = GPIO_PIN_9;
    gpio_init.Mode = GPIO_MODE_IT_RISING;
    gpio_init.Alternate = 0;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 3, 3);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void I2C1_ER_IRQHandler(void) {
    pbdrv_imu_lsm6ds3tr_c_stm32_handle_i2c_er_irq();
}

void I2C1_EV_IRQHandler(void) {
    pbdrv_imu_lsm6ds3tr_c_stm32_handle_i2c_ev_irq();
}

void EXTI9_5_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
}

// I/O ports

const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin = {
    .bank = GPIOB,
    .pin = 12
};

const pbdrv_ioport_platform_data_t pbdrv_ioport_platform_data[PBDRV_CONFIG_IOPORT_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .motor_driver_index = 0,
        .uart_driver_index = UART_PORT_A,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOC, .pin = 5  },
            .p6 = { .bank = GPIOB, .pin = 2  },
            .uart_buf = { .bank = GPIOH, .pin = 0  },
            .uart_tx = { .bank = GPIOB, .pin = 6  },
            .uart_rx = { .bank = GPIOB, .pin = 7  },
            .uart_tx_alt_uart = GPIO_AF7_USART1,
            .uart_rx_alt_uart = GPIO_AF7_USART1,
        },
        .supported_modes = PBIO_PORT_MODE_LEGO_PUP | PBIO_PORT_MODE_UART,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .motor_driver_index = 1,
        .uart_driver_index = UART_PORT_B,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOC, .pin = 3  },
            .p6 = { .bank = GPIOC, .pin = 0  },
            .uart_buf = { .bank = GPIOH, .pin = 1  },
            .uart_tx = { .bank = GPIOA, .pin = 2  },
            .uart_rx = { .bank = GPIOA, .pin = 3  },
            .uart_tx_alt_uart = GPIO_AF7_USART2,
            .uart_rx_alt_uart = GPIO_AF7_USART2,
        },
        .supported_modes = PBIO_PORT_MODE_LEGO_PUP | PBIO_PORT_MODE_UART,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .motor_driver_index = 2,
        .uart_driver_index = UART_PORT_C,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOC, .pin = 4  },
            .p6 = { .bank = GPIOA, .pin = 7  },
            .uart_buf = { .bank = GPIOC, .pin = 8  },
            .uart_tx = { .bank = GPIOC, .pin = 10 },
            .uart_rx = { .bank = GPIOC, .pin = 11 },
            .uart_tx_alt_uart = GPIO_AF7_USART3,
            .uart_rx_alt_uart = GPIO_AF7_USART3,
        },
        .supported_modes = PBIO_PORT_MODE_LEGO_PUP | PBIO_PORT_MODE_UART,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .motor_driver_index = 3,
        .uart_driver_index = UART_PORT_D,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOA, .pin = 4  },
            .p6 = { .bank = GPIOA, .pin = 5  },
            .uart_buf = { .bank = GPIOC, .pin = 7  },
            .uart_tx = { .bank = GPIOB, .pin = 11 },
            .uart_rx = { .bank = GPIOB, .pin = 10  },
            .uart_tx_alt_uart = GPIO_AF8_LPUART1,
            .uart_rx_alt_uart = GPIO_AF8_LPUART1,
        },
        .supported_modes = PBIO_PORT_MODE_LEGO_PUP | PBIO_PORT_MODE_UART,
    },
};

// LED

static const pbdrv_led_pwm_platform_color_t pbdrv_led_pwm_color = {
    .r_factor = 1000,
    .g_factor = 270,
    .b_factor = 200,
    .r_brightness = 174,
    .g_brightness = 1590,
    .b_brightness = 327,
};

const pbdrv_led_pwm_platform_data_t pbdrv_led_pwm_platform_data[PBDRV_CONFIG_LED_PWM_NUM_DEV] = {
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_0,
        .r_id = PWM_DEV_3,
        .r_ch = 2,
        .g_id = PWM_DEV_3,
        .g_ch = 1,
        .b_id = PWM_DEV_2,
        .b_ch = 1,
        .scale_factor = 5,
    }
};

// Motor driver

const pbdrv_motor_driver_hbridge_pwm_platform_data_t
    pbdrv_motor_driver_hbridge_pwm_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    // Port A
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 1,
        .pin1_alt = GPIO_AF14_TIM15,
        .pin1_pwm_id = PWM_DEV_1,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 14,
        .pin2_alt = GPIO_AF14_TIM15,
        .pin2_pwm_id = PWM_DEV_1,
        .pin2_pwm_ch = 1,
    },
    // Port B
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 9,
        .pin1_alt = GPIO_AF1_TIM1,
        .pin1_pwm_id = PWM_DEV_0,
        .pin1_pwm_ch = 2,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 0,
        .pin2_alt = GPIO_AF1_TIM1,
        .pin2_pwm_id = PWM_DEV_0,
        .pin2_pwm_ch = 2,
    },
    // Port C
    {
        .pin1_gpio.bank = GPIOB,
        .pin1_gpio.pin = 13,
        .pin1_alt = GPIO_AF1_TIM1,
        .pin1_pwm_id = PWM_DEV_0,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOA,
        .pin2_gpio.pin = 8,
        .pin2_alt = GPIO_AF1_TIM1,
        .pin2_pwm_id = PWM_DEV_0,
        .pin2_pwm_ch = 1,
    },
    // Port D
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 10,
        .pin1_alt = GPIO_AF1_TIM1,
        .pin1_pwm_id = PWM_DEV_0,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 1,
        .pin2_alt = GPIO_AF1_TIM1,
        .pin2_pwm_id = PWM_DEV_0,
        .pin2_pwm_ch = 3,
    },
};

// PWM

static void pwm_dev_0_platform_init(void) {
}

static void pwm_dev_1_platform_init(void) {
}

static void pwm_dev_2_platform_init(void) {
    // blue LED on PA6 using TIM16 CH1
    GPIO_InitTypeDef gpio_init = {
        .Pin = GPIO_PIN_6,
        .Mode = GPIO_MODE_AF_PP,
        .Alternate = GPIO_AF14_TIM16,
    };
    HAL_GPIO_Init(GPIOA, &gpio_init);
}

static void pwm_dev_3_platform_init(void) {
    GPIO_InitTypeDef gpio_init = { .Mode = GPIO_MODE_OUTPUT_PP };

    // green LED on PA11
    gpio_init.Pin = GPIO_PIN_11;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    // red LED on PB15
    gpio_init.Pin = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    // channel 1: green LED; channel 2: red LED
    TIM2->DIER |= TIM_DIER_CC1IE | TIM_DIER_CC2IE | TIM_DIER_UIE;
    HAL_NVIC_SetPriority(TIM2_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

// NOTE: Official LEGO firmware uses 1 kHz PWM for motors. We have changed to
// 10 kHz to reduce the unpleasant noise (similar to the frequency used by the
// official EV3 firmware).

const pbdrv_pwm_stm32_tim_platform_data_t
    pbdrv_pwm_stm32_tim_platform_data[PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV] = {
    {
        .platform_init = pwm_dev_0_platform_init,
        .TIMx = TIM1,
        .prescalar = 8, // results in 10 MHz clock
        .period = 1000, // 10 MHz divided by 1k makes 10 kHz PWM
        .id = PWM_DEV_0,
        // channel 1: Port C motor driver; channel 2: Port B motor driver
        // channel 3: Port D motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_1_COMPLEMENT | PBDRV_PWM_STM32_TIM_CHANNEL_2_COMPLEMENT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_COMPLEMENT,
    },
    {
        .platform_init = pwm_dev_1_platform_init,
        .TIMx = TIM15,
        .prescalar = 8, // results in 10 MHz clock
        .period = 1000, // 10 MHz divided by 1k makes 10 kHz PWM
        .id = PWM_DEV_1,
        // channel 1: Port A motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_1_COMPLEMENT,
    },
    {
        .platform_init = pwm_dev_2_platform_init,
        .TIMx = TIM16,
        .prescalar = 8, // results in 10 MHz clock
        .period = 10000, // 10 MHz divided by 10k makes 1 kHz PWM
        .id = PWM_DEV_2,
        // channel 1: Blue LED
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE,
    },
    {
        .platform_init = pwm_dev_3_platform_init,
        .TIMx = TIM2,
        .prescalar = 1, // results in 10 MHz clock (5 MHz APB1 clock x2 since prescaler != 1)
        .period = 10000, // 10 MHz divided by 10k makes 1 kHz PWM
        .id = PWM_DEV_3,
        // channel 1: Green LED; channel 2: Red LED
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE,
    },
};

// HACK: Official LEGO firmware uses TIM1 and TIM15 for red and green LEDs.
// However this requires that the PWM period is the same as the motors.
// We want to control the two PWM periods independently. So we are using TIM2
// for the LEDs. The pin mux doesn't work out, so we have to manually write
// the GPIOs in the timer interrupt handler.
void TIM2_IRQHandler(void) {
    uint32_t cnt = TIM2->CNT;
    uint32_t sr = TIM2->SR;
    uint32_t handled = TIM_SR_UIF;

    // NB: there seems to be hardware bug where sometimes we don't get the CCxIF
    // interrupt or only get it 1/2 of the time for certain bands of small duty
    // cycle values. So we are comparing CNT to CCRx in addition to just looking
    // at the interrupt. This works best when the red and green values are near
    // each other since that minimizes the error.

    // green LED
    if (cnt >= TIM2->CCR1 || sr & TIM_SR_CC1IF) {
        // If we have reached the CC1 count, turn the GPIO off
        GPIOA->BRR = GPIO_BRR_BR11;
        handled |= TIM_SR_CC1IF;
    } else if (sr & TIM_SR_UIF) {
        // otherwise if it is the start of the next PWM period turn the GPIO on
        GPIOA->BSRR = GPIO_BSRR_BS11;
    }

    // red LED
    if (cnt >= TIM2->CCR2 || sr & TIM_SR_CC2IF) {
        // If we have reached the CC2 count, turn the GPIO off
        GPIOB->BRR = GPIO_BRR_BR15;
        handled |= TIM_SR_CC2IF;
    } else if (sr & TIM_SR_UIF) {
        // otherwise if it is the start of the next PWM period turn the GPIO on
        GPIOB->BSRR = GPIO_BSRR_BS15;
    }

    // clear interrupts
    TIM2->SR &= ~handled;
}

// Reset

void pbdrv_reset_power_off(void) {
    // setting PC12 low cuts the power
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
}

// UART

const pbdrv_uart_stm32l4_ll_dma_platform_data_t
    pbdrv_uart_stm32l4_ll_dma_platform_data[PBDRV_CONFIG_UART_STM32L4_LL_DMA_NUM_UART] = {
    [UART_PORT_A] = {
        .tx_dma = DMA1,
        .tx_dma_ch = LL_DMA_CHANNEL_4,
        .tx_dma_req = LL_DMA_REQUEST_2,
        .tx_dma_irq = DMA1_Channel4_IRQn,
        .rx_dma = DMA1,
        .rx_dma_ch = LL_DMA_CHANNEL_5,
        .rx_dma_req = LL_DMA_REQUEST_2,
        .rx_dma_irq = DMA1_Channel5_IRQn,
        .uart = USART1,
        .uart_irq = USART1_IRQn,
    },
    [UART_PORT_B] = {
        .tx_dma = DMA1,
        .tx_dma_ch = LL_DMA_CHANNEL_7,
        .tx_dma_req = LL_DMA_REQUEST_2,
        .tx_dma_irq = DMA1_Channel7_IRQn,
        .rx_dma = DMA1,
        .rx_dma_ch = LL_DMA_CHANNEL_6,
        .rx_dma_req = LL_DMA_REQUEST_2,
        .rx_dma_irq = DMA1_Channel6_IRQn,
        .uart = USART2,
        .uart_irq = USART2_IRQn,
    },
    [UART_PORT_C] = {
        .tx_dma = DMA1,
        .tx_dma_ch = LL_DMA_CHANNEL_2,
        .tx_dma_req = LL_DMA_REQUEST_2,
        .tx_dma_irq = DMA1_Channel2_IRQn,
        .rx_dma = DMA1,
        .rx_dma_ch = LL_DMA_CHANNEL_3,
        .rx_dma_req = LL_DMA_REQUEST_2,
        .rx_dma_irq = DMA1_Channel3_IRQn,
        .uart = USART3,
        .uart_irq = USART3_IRQn,
    },
    [UART_PORT_D] = {
        .tx_dma = DMA2,
        .tx_dma_ch = LL_DMA_CHANNEL_6,
        .tx_dma_req = LL_DMA_REQUEST_4,
        .tx_dma_irq = DMA2_Channel6_IRQn,
        .rx_dma = DMA2,
        .rx_dma_ch = LL_DMA_CHANNEL_7,
        .rx_dma_req = LL_DMA_REQUEST_4,
        .rx_dma_irq = DMA2_Channel7_IRQn,
        .uart = LPUART1,
        .uart_irq = LPUART1_IRQn,
    },
};

void DMA1_Channel2_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_tx_dma_irq(UART_PORT_C);
}

void DMA1_Channel3_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_rx_dma_irq(UART_PORT_C);
}

void DMA1_Channel4_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_tx_dma_irq(UART_PORT_A);
}

void DMA1_Channel5_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_rx_dma_irq(UART_PORT_A);
}

void DMA1_Channel6_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_rx_dma_irq(UART_PORT_B);
}

void DMA1_Channel7_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_tx_dma_irq(UART_PORT_B);
}

void DMA2_Channel6_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_tx_dma_irq(UART_PORT_D);
}

void DMA2_Channel7_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_rx_dma_irq(UART_PORT_D);
}

void USART1_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_uart_irq(UART_PORT_A);
}

void USART2_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_uart_irq(UART_PORT_B);
}

void USART3_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_uart_irq(UART_PORT_C);
}

void LPUART1_IRQHandler(void) {
    pbdrv_uart_stm32l4_ll_dma_handle_uart_irq(UART_PORT_D);
}

// STM32 HAL integration

// Default clock is 4MHz on boot
uint32_t SystemCoreClock = 4000000;

// copied from system_stm32.c in stm32 port
const uint8_t AHBPrescTable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
const uint8_t APBPrescTable[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };
const uint32_t MSIRangeTable[12] = {
    100000, 200000, 400000, 800000, 1000000, 2000000,
    4000000, 8000000, 16000000, 24000000, 32000000, 48000000
};

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef gpio_init = {
        .Pin = 0, // variable
        .Mode = GPIO_MODE_ANALOG_ADC_CONTROL,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0, // not used
    };

    ADC_ChannelConfTypeDef adc_ch_config = {
        .Channel = 0, // variable
        .Rank = 0, // variable
        .SamplingTime = ADC_SAMPLETIME_640CYCLES_5,
        .SingleDiff = ADC_SINGLE_ENDED,
        .OffsetNumber = ADC_OFFSET_NONE,
        .Offset = 0, // not used
    };

    // clocks are enabled in SystemInit
    assert_param(__HAL_RCC_TIM6_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_DMA1_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_ADC1_IS_CLK_ENABLED());

    LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSOURCE_SYSCLK);

    // PC1, battery voltage

    gpio_init.Pin = GPIO_PIN_1;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_2;
    adc_ch_config.Rank = ADC_REGULAR_RANK_1;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PC2, battery current

    gpio_init.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_3;
    adc_ch_config.Rank = ADC_REGULAR_RANK_2;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // internal, temperature

    adc_ch_config.Channel = ADC_CHANNEL_TEMPSENSOR;
    adc_ch_config.Rank = ADC_REGULAR_RANK_3;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);
}

void DMA1_Channel1_IRQHandler(void) {
    pbdrv_adc_stm32_hal_handle_irq();
}

// Early initialization

// special memory addresses defined in linker script
extern uint32_t *_fw_isr_vector_src;

// Called from assembly code in startup.s
void SystemInit(void) {
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));  /* set CP10 and CP11 Full Access */
    #endif

    // enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // since the firmware starts at 0x08008000, we need to set the vector table offset
    SCB->VTOR = (uint32_t)&_fw_isr_vector_src;

    // Using internal RC oscillator (MSI)
    RCC_OscInitTypeDef osc_init = { };
    osc_init.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    osc_init.MSIState = RCC_MSI_ON;
    osc_init.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    osc_init.MSIClockRange = RCC_MSIRANGE_6; // 4MHz
    osc_init.PLL.PLLState = RCC_PLL_ON;
    osc_init.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    osc_init.PLL.PLLM = 1; // VCO_IN 4MHz (4MHz / 1)
    osc_init.PLL.PLLN = 40; // VCO_OUT 160MHz (4MHz * 40)
    osc_init.PLL.PLLP = RCC_PLLP_DIV7; // 22.9MHz SAI clock
    osc_init.PLL.PLLQ = RCC_PLLQ_DIV4; // 40MHz SDMMC1, RNG and USB clocks
    osc_init.PLL.PLLR = RCC_PLLR_DIV2; // 80MHz system clock

    HAL_RCC_OscConfig(&osc_init);

    RCC_ClkInitTypeDef clk_init;
    clk_init.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;  // HCLK 80MHz
    // PCLK1 is less than max allowed (80MHz) so that LPUART1 can operate at 2400 baud.
    clk_init.APB1CLKDivider = RCC_HCLK_DIV16; // PCLK1 5Mhz
    clk_init.APB2CLKDivider = RCC_HCLK_DIV1; // PCLK2 80Mhz

    HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_4);

    // enable clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN | RCC_AHB1ENR_FLASHEN |
        RCC_AHB1ENR_CRCEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN |
        RCC_AHB2ENR_GPIODEN | RCC_AHB2ENR_GPIOHEN | RCC_AHB2ENR_ADCEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN | RCC_APB1ENR1_TIM6EN | RCC_APB1ENR1_WWDGEN |
        RCC_APB1ENR1_USART2EN | RCC_APB1ENR1_USART3EN | RCC_APB1ENR1_I2C1EN | RCC_APB1ENR1_PWREN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN | RCC_APB2ENR_TIM1EN | RCC_APB2ENR_SPI1EN |
        RCC_APB2ENR_USART1EN | RCC_APB2ENR_TIM15EN | RCC_APB2ENR_TIM16EN;

    // Keep main power on (PC12)
    GPIO_InitTypeDef gpio_init = { };
    gpio_init.Pin = GPIO_PIN_12;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_GPIO_Init(GPIOC, &gpio_init);
}
