// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <stdbool.h>
#include <stdio.h>

#include "stm32f070xb.h"

#define WHO_AM_I                    0x0f

#define CTRL_REG1                   0x20
#define CTRL_REG1_ODR_POWER_DOWN    (0 << 4)
#define CTRL_REG1_ODR_1HZ           (1 << 4)
#define CTRL_REG1_ODR_10HZ          (2 << 4)
#define CTRL_REG1_ODR_25HZ          (3 << 4)
#define CTRL_REG1_ODR_50HZ          (4 << 4)
#define CTRL_REG1_ODR_100HZ         (5 << 4)
#define CTRL_REG1_ODR_200HZ         (6 << 4)
#define CTRL_REG1_ODR_400HZ         (7 << 4)
#define CTRL_REG1_ODR_1620HZ        (8 << 4)
#define CTRL_REG1_ODR_5376HZ        (9 << 4)
#define CTRL_REG1_LPEN              (1 << 3)
#define CTRL_REG1_ZEN               (1 << 2)
#define CTRL_REG1_YEN               (1 << 1)
#define CTRL_REG1_XEN               (1 << 0)

#define CLICK_CFG                   0x38
#define CLICK_CFG_ZD                (1 << 5)
#define CLICK_CFG_ZS                (1 << 4)
#define CLICK_CFG_YD                (1 << 3)
#define CLICK_CFG_YS                (1 << 2)
#define CLICK_CFG_XD                (1 << 1)
#define CLICK_CFG_XS                (1 << 0)

#define CLICK_THS                   0x3a
#define CLICK_THS_LIR_CLICK         (1 << 7)
#define CLICK_THS_THS(n)            ((n) & 0x7f)

#define TIME_LIMIT                  0x3b
#define TIME_LIMIT_TLI(n)           ((n) & 0x7f)

#define TIME_LATENCY                0x3c
#define TIME_LATENCY_TLA(n)         ((n) & 0x7f)

#define BYTE_ACCESS(r) (*(volatile uint8_t *)&(r))

static void accel_spi_read(uint8_t reg, uint8_t *value) {
    uint8_t dummy;

    // set chip select low
    GPIOA->BRR = GPIO_BRR_BR_4;

    BYTE_ACCESS(SPI1->DR) = reg;

    // busy wait
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) { }
    } while (SPI1->SR & SPI_SR_BSY);

    while (SPI1->SR & SPI_SR_RXNE) {
        dummy = BYTE_ACCESS(SPI1->DR);
        printf("rx: %x\n", dummy);
    }
    BYTE_ACCESS(SPI1->DR) = 0;

    // busy wait
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) { }
    } while (SPI1->SR & SPI_SR_BSY);

    *value = BYTE_ACCESS(SPI1->DR);

    // clear chip select
    GPIOA->BSRR = GPIO_BSRR_BS_4;
}

static void accel_spi_write(uint8_t reg, uint8_t value) {
    // set chip select low
    GPIOA->BRR = GPIO_BRR_BR_4;

    BYTE_ACCESS(SPI1->DR) = reg;

    // busy wait
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) { }
    } while (SPI1->SR & SPI_SR_BSY);

    BYTE_ACCESS(SPI1->DR) = value;

    // busy wait
    do {
        while (!(SPI1->SR & SPI_SR_RXNE)) { }
    } while (SPI1->SR & SPI_SR_BSY);

    // clear chip select
    GPIOA->BSRR = GPIO_BSRR_BS_4;
}

void accel_init(void) {
    // PA4 gpio output - used for CS
    GPIOA->BSRR = GPIO_BSRR_BS_4;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER4_Msk) | (1 << GPIO_MODER_MODER4_Pos);

    // PA5, PA5, PA7 muxed as SPI1 pins
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER5_Msk) | (2 << GPIO_MODER_MODER5_Pos);
    GPIOA->AFR[0] &= ~GPIO_AFRH_AFRH5;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER6_Msk) | (2 << GPIO_MODER_MODER6_Pos);
    GPIOA->AFR[0] &= ~GPIO_AFRH_AFRH6;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER7_Msk) | (2 << GPIO_MODER_MODER7_Pos);
    GPIOA->AFR[0] &= ~GPIO_AFRH_AFRH7;

    // configure SPI1
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR1 = (2 << SPI_CR1_BR_Pos) | SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA;
    SPI1->CR2 = SPI_CR2_FRXTH | (7 << SPI_CR2_DS_Pos) | SPI_CR2_SSOE;
    SPI1->CR1 |= SPI_CR1_SPE;

    uint8_t x;
    accel_spi_read(WHO_AM_I, &x);
    printf("WHO_AM_I: %x\n", x);

    accel_spi_write(CTRL_REG1, CTRL_REG1_XEN | CTRL_REG1_YEN | CTRL_REG1_ZEN | CTRL_REG1_ODR_5376HZ);
    accel_spi_write(CLICK_CFG, CLICK_CFG_XS | CLICK_CFG_XD | CLICK_CFG_YS | CLICK_CFG_YD | CLICK_CFG_ZS | CLICK_CFG_ZD);
    accel_spi_write(CLICK_THS, CLICK_THS_THS(127));
    accel_spi_write(TIME_LIMIT, TIME_LIMIT_TLI(127));
    accel_spi_write(TIME_LATENCY, TIME_LATENCY_TLA(127));
}

void accel_get_values(int *x, int *y, int *z) {
}

void accel_deinit(void)
{
}
