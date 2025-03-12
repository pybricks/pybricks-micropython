// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_EV3

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <contiki.h>

#include <pbdrv/adc.h>
#include <pbdrv/clock.h>
#include <pbdrv/gpio.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include <tiam1808/spi.h>
#include <tiam1808/psc.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/interrupt.h>

#include "../drv/gpio/gpio_ev3.h"

PROCESS(pbdrv_adc_process, "ADC");

#define PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES (4)

static volatile uint16_t channel_data[PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES] = {0};
static volatile uint8_t channel_data_index = 0;
static volatile bool adc_busy = false;

static pbdrv_adc_callback_t pbdrv_adc_callbacks[1];
static uint32_t pbdrv_adc_callback_count = 0;

void pbdrv_adc_set_callback(pbdrv_adc_callback_t callback) {
    if (pbdrv_adc_callback_count < PBIO_ARRAY_SIZE(pbdrv_adc_callbacks)) {
        pbdrv_adc_callbacks[pbdrv_adc_callback_count++] = callback;
    }
}

pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {
    if (ch >= PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Values for the requested channel are received several samples later.
    // The data only appears 12-bit but the last 2 bits are always zero.
    *value = (channel_data[ch + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES] - 4096 * ch) >> 2;
    return PBIO_SUCCESS;
}

static void spi0_isr(void) {
    uint32_t intCode = 0;
    IntSystemStatusClear(SYS_INT_SPINT0);

    while ((intCode = SPIInterruptVectorGet(SOC_SPI_0_REGS))) {
        if (intCode != SPI_TX_BUF_EMPTY) {
            continue;
        }
        // Payload encoding comes from the original EV3 sources, but we
        // use the hardware SPI peripheral instead of bit-banging.
        uint16_t payload = 0x1840 | (((channel_data_index % PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS) & 0x000F) << 7);
        HWREG(SOC_SPI_0_REGS + SPI_SPIDAT0) = payload;
        channel_data[channel_data_index] = SPIDataReceive(SOC_SPI_0_REGS);

        if (++channel_data_index == PBIO_ARRAY_SIZE(channel_data)) {
            SPIIntDisable(SOC_SPI_0_REGS, SPI_TRANSMIT_INT);
            adc_busy = false;
            process_poll(&pbdrv_adc_process);
        }
    }
}

static void pbdrv_adc_exit(void) {
    SPIIntDisable(SOC_SPI_0_REGS, SPI_RECV_INT | SPI_TRANSMIT_INT);
}

// ADC / Flash SPI0 data MOSI
static const pbdrv_gpio_t pin_spi0_mosi = PBDRV_GPIO_EV3_PIN(3, 15, 12, 8, 5);

// ADC / Flash SPI0 data MISO
static const pbdrv_gpio_t pin_spi0_miso = PBDRV_GPIO_EV3_PIN(3, 11, 8, 8, 6);

// LCD SPI0 Clock
static const pbdrv_gpio_t pin_spi0_clk = PBDRV_GPIO_EV3_PIN(3, 3, 0, 1, 8);

// ADC / Flash SPI0 chip select (active low).
static const pbdrv_gpio_t pin_spi0_cs = PBDRV_GPIO_EV3_PIN(3, 27, 24, 8, 2);

// ADCACK PIN
static const pbdrv_gpio_t pin_adc_ack = PBDRV_GPIO_EV3_PIN(19, 19, 16, 6, 2);

// ADCBATEN
static const pbdrv_gpio_t pin_adc_bat_en = PBDRV_GPIO_EV3_PIN(1, 7, 4, 0, 6);

void pbdrv_adc_init(void) {

    // Configure the GPIO pins.
    pbdrv_gpio_alt(&pin_spi0_mosi, SYSCFG_PINMUX3_PINMUX3_15_12_SPI0_SIMO0);
    pbdrv_gpio_alt(&pin_spi0_miso, SYSCFG_PINMUX3_PINMUX3_11_8_SPI0_SOMI0);
    pbdrv_gpio_alt(&pin_spi0_clk, SYSCFG_PINMUX3_PINMUX3_3_0_SPI0_CLK);
    pbdrv_gpio_alt(&pin_spi0_cs, SYSCFG_PINMUX3_PINMUX3_27_24_NSPI0_SCS3);

    pbdrv_gpio_input(&pin_adc_ack);

    pbdrv_gpio_out_high(&pin_adc_bat_en);

    // Waking up the SPI1 instance.
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_SPI0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    // Register the ISR in the Interrupt Vector Table.
    IntRegister(SYS_INT_SPINT0, spi0_isr);
    IntChannelSet(SYS_INT_SPINT0, 2);
    IntSystemEnable(SYS_INT_SPINT0);

    // Reset.
    SPIReset(SOC_SPI_0_REGS);
    SPIOutOfReset(SOC_SPI_0_REGS);

    // Mode.
    uint32_t spipc0 = SPI_SPIPC0_SOMIFUN | SPI_SPIPC0_SIMOFUN | SPI_SPIPC0_CLKFUN | SPI_SPIPC0_SCS0FUN3;
    SPIModeConfigure(SOC_SPI_0_REGS, SPI_MASTER_MODE);
    SPIPinControl(SOC_SPI_0_REGS, 0, 0, (unsigned int *)&spipc0);

    // Config.
    SPIClkConfigure(SOC_SPI_0_REGS, SOC_SYSCLK_2_FREQ, 2000000, SPI_DATA_FORMAT0);
    SPIConfigClkFormat(SOC_SPI_0_REGS, SPI_CLK_OUTOFPHASE | 0x00000010, SPI_DATA_FORMAT0);
    SPIDelayConfigure(SOC_SPI_0_REGS, 10, 10, 10, 10);
    SPIIntLevelSet(SOC_SPI_0_REGS, SPI_RECV_INTLVL | SPI_TRANSMIT_INTLVL);
    SPIDefaultCSSet(SOC_SPI_0_REGS, 8);

    // Enable and loop around all channels.
    SPIEnable(SOC_SPI_0_REGS);

    process_start(&pbdrv_adc_process);
}

void pbdrv_adc_update_soon(void) {
    process_poll(&pbdrv_adc_process);
}

PROCESS_THREAD(pbdrv_adc_process, ev, data) {
    PROCESS_EXITHANDLER(pbdrv_adc_exit());

    static struct etimer etimer;

    PROCESS_BEGIN();

    etimer_set(&etimer, 10);
    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL((ev == PROCESS_EVENT_TIMER && etimer_expired(&etimer)) || ev == PROCESS_EVENT_POLL);

        channel_data_index = 0;
        adc_busy = true;
        SPIEnable(SOC_SPI_0_REGS);
        SPIIntEnable(SOC_SPI_0_REGS, SPI_TRANSMIT_INT);
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL && !adc_busy);

        for (uint32_t i = 0; i < pbdrv_adc_callback_count; i++) {
            pbdrv_adc_callbacks[i]();
        }

        etimer_reset(&etimer);
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_ADC_EV3
