// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors
//
// SPDX-License-Identifier: MPL-1.0
// Copyright (c) 2016 Tobias Schießl (System Init / partial pinmux / boot order / exceptionhandler)
//
/*
* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <tiam1808/armv5/am1808/edma_event.h>
#include <tiam1808/armv5/am1808/evmAM1808.h>
#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/edma.h>
#include <tiam1808/hw/hw_edma3cc.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_syscfg1_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/psc.h>

#include <pbdrv/ioport.h>
#include <pbio/port_interface.h>

#include "../../drv/block_device/block_device_ev3.h"
#include "../../drv/button/button_gpio.h"
#include "../../drv/display/display_ev3.h"
#include "../../drv/gpio/gpio_ev3.h"
#include "../../drv/led/led_array_pwm.h"
#include "../../drv/led/led_dual.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/pwm/pwm_ev3.h"
#include "../../drv/uart/uart_ev3.h"
#include "../../drv/reset/reset.h"

enum {
    LED_DEV_0_STATUS,
    LED_DEV_1_STATUS_LEFT,
    LED_DEV_2_STATUS_RIGHT,
};

enum {
    PWM_DEV_0_TODO,
    PWM_DEV_1_TODO,
};

// LED

const pbdrv_led_dual_platform_data_t pbdrv_led_dual_platform_data[PBDRV_CONFIG_LED_DUAL_NUM_DEV] = {
    {
        .id = LED_DEV_0_STATUS,
        .id1 = LED_DEV_1_STATUS_LEFT,
        .id2 = LED_DEV_2_STATUS_RIGHT,
    },
};

static const pbdrv_led_pwm_platform_color_t pbdrv_led_pwm_color = {
    // TODO: PWM not yet implemented, so these values not used.
    .r_factor = 1000,
    .g_factor = 170,
    .b_factor = 200,
    .r_brightness = 174,
    .g_brightness = 1590,
    .b_brightness = 327,
};

const pbdrv_led_pwm_platform_data_t pbdrv_led_pwm_platform_data[PBDRV_CONFIG_LED_PWM_NUM_DEV] = {
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_1_STATUS_LEFT,
        .r_id = PWM_DEV_0_TODO,
        .r_ch = 0,
        .g_id = PWM_DEV_0_TODO,
        .g_ch = 1,
        // Blue not available.
        .b_id = PWM_DEV_0_TODO,
        .b_ch = 2,
        // TODO: PWM not yet implemented, so these values not used.
        .scale_factor = 35,
    },
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_2_STATUS_RIGHT,
        .r_id = PWM_DEV_1_TODO,
        .r_ch = 0,
        .g_id = PWM_DEV_1_TODO,
        .g_ch = 1,
        // Blue not available.
        .b_id = PWM_DEV_1_TODO,
        .b_ch = 2,
        // TODO: PWM not yet implemented, so these values not used.
        .scale_factor = 35,
    },
};

const pbdrv_pwm_tiam1808_platform_data_t
    pbdrv_pwm_tiam1808_platform_data[PBDRV_CONFIG_PWM_TIAM1808_NUM_DEV] = {
    {
        .id = PWM_DEV_0_TODO,
        .gpio_red = PBDRV_GPIO_EV3_PIN(13, 11, 8, 6, 13),
        .gpio_green = PBDRV_GPIO_EV3_PIN(14, 3, 0, 6, 7),
    },
    {
        .id = PWM_DEV_1_TODO,
        .gpio_red = PBDRV_GPIO_EV3_PIN(13, 15, 12, 6, 12),
        .gpio_green = PBDRV_GPIO_EV3_PIN(13, 7, 4, 6, 14),
    },
};

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio = PBDRV_GPIO_EV3_PIN(14, 7, 4, 6, 6),
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_LEFT,
        .active_low = false,
    },
    [1] = {
        .gpio = PBDRV_GPIO_EV3_PIN(16, 23, 20, 7, 12),
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_RIGHT,
        .active_low = false,
    },
    [2] = {
        .gpio = PBDRV_GPIO_EV3_PIN(16, 11, 8, 7, 15),
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_UP,
        .active_low = false,
    },
    [3] = {
        .gpio = PBDRV_GPIO_EV3_PIN(16, 15, 12, 7, 14),
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_DOWN,
        .active_low = false,
    },
    [4] = {
        .gpio = PBDRV_GPIO_EV3_PIN(2, 11, 8, 1, 13),
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_CENTER,
        .active_low = false,
    },
    [5] = {
        .gpio = PBDRV_GPIO_EV3_PIN(13, 23, 20, 6, 10),
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_LEFT_UP,
        .active_low = false,
    },
};

// UART.

enum {
    UART1, // sensor 1
    UART0, // sensor 2
    PRU0_LINE1, // sensor 3
    PRU0_LINE0, // sensor 4
    UART2, // Bluetooth
};

static void pbdrv_uart_ev3_handle_irq_uart0(void) {
    pbdrv_uart_ev3_handle_irq(UART0);
}

static void pbdrv_uart_ev3_handle_irq_uart1(void) {
    pbdrv_uart_ev3_handle_irq(UART1);
}

static void pbdrv_uart_ev3_handle_irq_uart2(void) {
    pbdrv_uart_ev3_handle_irq(UART2);
}

static void pbdrv_uart_ev3_handle_irq_pru0_line0(void) {
    pbdrv_uart_ev3_handle_irq(PRU0_LINE0);
}

static void pbdrv_uart_ev3_handle_irq_pru0_line1(void) {
    pbdrv_uart_ev3_handle_irq(PRU0_LINE1);
}

const pbdrv_uart_ev3_platform_data_t
    pbdrv_uart_ev3_platform_data[PBDRV_CONFIG_UART_EV3_NUM_UART] = {
    [UART1] = {
        .uart_kind = EV3_UART_HW,
        .base_address = SOC_UART_1_REGS,
        .peripheral_id = HW_PSC_UART1,
        .sys_int_uart_tx_int_id = EDMA3_CHA_UART1_TX,
        .sys_int_uart_rx_int_id = SYS_INT_UARTINT1,
        .isr_handler = pbdrv_uart_ev3_handle_irq_uart1,
    },
    [UART0] = {
        .uart_kind = EV3_UART_HW,
        .base_address = SOC_UART_0_REGS,
        .peripheral_id = HW_PSC_UART0,
        .sys_int_uart_tx_int_id = EDMA3_CHA_UART0_TX,
        .sys_int_uart_rx_int_id = SYS_INT_UARTINT0,
        .isr_handler = pbdrv_uart_ev3_handle_irq_uart0,
    },
    [PRU0_LINE1] = {
        .uart_kind = EV3_UART_PRU,
        .base_address = 0, // Not used.
        .peripheral_id = 1, // Soft UART line 1.
        .sys_int_uart_tx_int_id = SYS_INT_EVTOUT1, // One common IRQ handler
        .sys_int_uart_rx_int_id = SYS_INT_EVTOUT1,
        .isr_handler = pbdrv_uart_ev3_handle_irq_pru0_line1,
    },
    [PRU0_LINE0] = {
        .uart_kind = EV3_UART_PRU,
        .base_address = 0, // Not used.
        .peripheral_id = 0, // Soft UART line 0.
        .sys_int_uart_tx_int_id = SYS_INT_EVTOUT0, // One common IRQ handler
        .sys_int_uart_rx_int_id = SYS_INT_EVTOUT0,
        .isr_handler = pbdrv_uart_ev3_handle_irq_pru0_line0,
    },
    [UART2] = {
        // TODO: Add CTS/RTS pins.
        .uart_kind = EV3_UART_HW,
        .base_address = SOC_UART_2_REGS,
        .peripheral_id = HW_PSC_UART2,
        .sys_int_uart_tx_int_id = EDMA3_CHA_UART2_TX,
        .sys_int_uart_rx_int_id = SYS_INT_UARTINT2,
        .isr_handler = pbdrv_uart_ev3_handle_irq_uart2,
    },
};

// TODO.
const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin = {
    .bank = NULL,
    .pin = 0,
};

const pbdrv_ioport_platform_data_t pbdrv_ioport_platform_data[PBDRV_CONFIG_IOPORT_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .motor_driver_index = 0,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .external_port_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .counter_driver_index = 0,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .motor_driver_index = 1,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .external_port_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .counter_driver_index = 1,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .motor_driver_index = 2,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .external_port_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .counter_driver_index = 2,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .motor_driver_index = 3,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .external_port_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .counter_driver_index = 3,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE,
    },
    {
        .port_id = PBIO_PORT_ID_1,
        .motor_driver_index = 4,
        .i2c_driver_index = 0,
        .uart_driver_index = UART1,
        .external_port_index = 0,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p1 = PBDRV_GPIO_EV3_PIN(18, 31, 28, 8, 10),
            .p2 = PBDRV_GPIO_EV3_PIN(6, 23, 20, 2, 2),
            .p5 = PBDRV_GPIO_EV3_PIN(1, 23, 20, 0, 2),
            .p6 = PBDRV_GPIO_EV3_PIN(0, 3, 0, 0, 15),
            .uart_buf = PBDRV_GPIO_EV3_PIN(18, 27, 24, 8, 11),
            .uart_tx = PBDRV_GPIO_EV3_PIN(4, 31, 28, 1, 0),
            .uart_rx = PBDRV_GPIO_EV3_PIN(4, 27, 24, 1, 1),
            .uart_tx_alt_uart = SYSCFG_PINMUX4_PINMUX4_31_28_UART1_TXD,
            .uart_rx_alt_uart = SYSCFG_PINMUX4_PINMUX4_27_24_UART1_RXD,
            .adc_p1 = 6,
            .adc_p6 = 5,
        },
        #if PBDRV_CONFIG_UART_DEBUG_FIRST_PORT
        .supported_modes = PBIO_PORT_MODE_UART,
        #else // PBDRV_CONFIG_UART_DEBUG_FIRST_PORT
        .supported_modes = PBIO_PORT_MODE_UART | PBIO_PORT_MODE_LEGO_DCM,
        #endif
    },
    {
        .port_id = PBIO_PORT_ID_2,
        .motor_driver_index = 5,
        .i2c_driver_index = 1,
        .uart_driver_index = UART0,
        .external_port_index = 1,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p1 = PBDRV_GPIO_EV3_PIN(18, 23, 20, 8, 12),
            .p2 = PBDRV_GPIO_EV3_PIN(18, 11, 8, 8, 15),
            .p5 = PBDRV_GPIO_EV3_PIN(0, 7, 4, 0, 14),
            .p6 = PBDRV_GPIO_EV3_PIN(0, 11, 8, 0, 13), // DIGI
            .uart_buf = PBDRV_GPIO_EV3_PIN(18, 15, 12, 8, 14),
            .uart_tx = PBDRV_GPIO_EV3_PIN(3, 23, 20, 8, 3),
            .uart_rx = PBDRV_GPIO_EV3_PIN(3, 19, 16, 8, 4),
            .uart_tx_alt_uart = SYSCFG_PINMUX3_PINMUX3_23_20_UART0_TXD,
            .uart_rx_alt_uart = SYSCFG_PINMUX3_PINMUX3_19_16_UART0_RXD,
            .adc_p1 = 8,
            .adc_p6 = 7,
        },
        .supported_modes = PBIO_PORT_MODE_UART | PBIO_PORT_MODE_LEGO_DCM,
    },
    {
        .port_id = PBIO_PORT_ID_3,
        .motor_driver_index = 6,
        .i2c_driver_index = 2,
        .uart_driver_index = PRU0_LINE1,
        .external_port_index = 2,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p1 = PBDRV_GPIO_EV3_PIN(19, 3, 0, 8, 9),
            .p2 = PBDRV_GPIO_EV3_PIN(16, 27, 24, 7, 11),
            .p5 = PBDRV_GPIO_EV3_PIN(0, 15, 12, 0, 12),
            .p6 = PBDRV_GPIO_EV3_PIN(2, 7, 4, 1, 14),
            .uart_buf = PBDRV_GPIO_EV3_PIN(17, 3, 0, 7, 9),
            .uart_tx = PBDRV_GPIO_EV3_PIN(2, 15, 12, 1, 12),
            .uart_rx = PBDRV_GPIO_EV3_PIN(2, 23, 20, 1, 10),
            .uart_tx_alt_uart = SYSCFG_PINMUX2_PINMUX2_15_12_AXR0_4,
            .uart_rx_alt_uart = SYSCFG_PINMUX2_PINMUX2_23_20_AXR0_2,
            .adc_p1 = 10,
            .adc_p6 = 9,
        },
        .supported_modes = PBIO_PORT_MODE_UART | PBIO_PORT_MODE_LEGO_DCM,
    },
    {
        .port_id = PBIO_PORT_ID_4,
        .motor_driver_index = 7,
        .i2c_driver_index = 3,
        .uart_driver_index = PRU0_LINE0,
        .external_port_index = 3,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p1 = PBDRV_GPIO_EV3_PIN(19, 11, 8, 6, 4),
            .p2 = PBDRV_GPIO_EV3_PIN(17, 7, 4, 7, 8),
            .p5 = PBDRV_GPIO_EV3_PIN(1, 27, 24, 0, 1),
            .p6 = PBDRV_GPIO_EV3_PIN(2, 3, 0, 1, 15),
            .uart_buf = PBDRV_GPIO_EV3_PIN(16, 31, 28, 7, 10),
            .uart_tx = PBDRV_GPIO_EV3_PIN(2, 19, 16, 1, 11),
            .uart_rx = PBDRV_GPIO_EV3_PIN(2, 27, 24, 1, 9),
            .uart_tx_alt_uart = SYSCFG_PINMUX2_PINMUX2_19_16_AXR0_3,
            .uart_rx_alt_uart = SYSCFG_PINMUX2_PINMUX2_27_24_AXR0_1,
            .adc_p1 = 12,
            .adc_p6 = 11,
        },
        .supported_modes = PBIO_PORT_MODE_UART | PBIO_PORT_MODE_LEGO_DCM,
    },
};

// See exceptionhandler.S
extern void ExceptionHandler(void);

void copy_vector_table(void) {
    unsigned int *dest = (unsigned int *)0xFFFF0000;
    unsigned int *addrExceptionHandler = (unsigned int *)ExceptionHandler;
    int i = 1;

    for (; i < 8 + 2048; ++i) {
        dest[i] = addrExceptionHandler[i];
    }
}

unsigned int EDMAVersionGet(void) {
    return 1;
}

/**
 * Callback for completion of all EDMA3 transfers on this platform.
 *
 * @param tccNum [in]   Transfer completion code number.
 * @param status [in]   Status of the transfer. Currently only EDMA3_XFER_COMPLETE.
 */
static void Edma3CompleteCallback(unsigned int tccNum, unsigned int status) {
    switch (tccNum) {
        case EDMA3_CHA_SPI0_RX:
            pbdrv_block_device_ev3_spi_rx_complete();
            return;
        case EDMA3_CHA_SPI0_TX:
            pbdrv_block_device_ev3_spi_tx_complete();
            return;
        case EDMA3_CHA_SPI1_TX:
            pbdrv_display_ev3_spi1_tx_complete(status);
            return;
        case EDMA3_CHA_UART0_TX:
            pbdrv_uart_ev3_handle_tx_complete(UART0);
            return;
        case EDMA3_CHA_UART1_TX:
            pbdrv_uart_ev3_handle_tx_complete(UART1);
            return;
        case EDMA3_CHA_UART2_TX:
            pbdrv_uart_ev3_handle_tx_complete(UART2);
            return;
        default:
            return;
    }
}

/**
 * ISR for completion of all EDMA3 transfers on this platform.
 *
 * This is unchanged from the TI StarterWare example.
 */
static void Edma3ComplHandlerIsr(void) {
    volatile unsigned int pendingIrqs;
    volatile unsigned int isIPR = 0;

    volatile unsigned int indexl;
    volatile unsigned int Cnt = 0;
    indexl = 1;
    IntSystemStatusClear(SYS_INT_CCINT0);
    isIPR = EDMA3GetIntrStatus(SOC_EDMA30CC_0_REGS);
    if (isIPR) {
        while ((Cnt < EDMA3CC_COMPL_HANDLER_RETRY_COUNT) && (indexl != 0)) {
            indexl = 0;
            pendingIrqs = EDMA3GetIntrStatus(SOC_EDMA30CC_0_REGS);
            while (pendingIrqs) {
                if ((pendingIrqs & 1) == TRUE) {
                    // Here write to ICR to clear the corresponding IPR bits.
                    EDMA3ClrIntr(SOC_EDMA30CC_0_REGS, indexl);
                    Edma3CompleteCallback(indexl, EDMA3_XFER_COMPLETE);
                }
                ++indexl;
                pendingIrqs >>= 1;
            }
            Cnt++;
        }
    }

}

/**
 * ISR for error handling of all EDMA3 transfers on this platform.
 *
 * This is unchanged from the TI StarterWare example.
 */
static void Edma3CCErrHandlerIsr(void) {
    volatile unsigned int pendingIrqs = 0;
    unsigned int Cnt = 0;
    unsigned int index = 1;
    unsigned int regionNum = 0;
    unsigned int evtqueNum = 0;

    IntSystemStatusClear(SYS_INT_CCERRINT);

    if ((HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_EMR) != 0)
        || (HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_QEMR) != 0)
        || (HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_CCERR) != 0)) {
        // Loop for EDMA3CC_ERR_HANDLER_RETRY_COUNT number of time, breaks
        // when no pending interrupt is found.
        while ((Cnt < EDMA3CC_ERR_HANDLER_RETRY_COUNT) && (index != 0)) {
            index = 0;
            pendingIrqs = HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_EMR);
            while (pendingIrqs) {
                // Process all the pending interrupts.
                if ((pendingIrqs & 1) == TRUE) {
                    // Write to EMCR to clear the corresponding EMR bits.
                    HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_EMCR) = (1 << index);
                    // Clear any SER
                    HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_S_SECR(regionNum)) = (1 << index);
                }
                ++index;
                pendingIrqs >>= 1;
            }
            index = 0;
            pendingIrqs = HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_QEMR);
            while (pendingIrqs) {
                // Process all the pending interrupts.
                if ((pendingIrqs & 1) == TRUE) {
                    // Here write to QEMCR to clear the corresponding QEMR bits.
                    HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_QEMCR) = (1 << index);
                    // Clear any QSER
                    HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_S_QSECR(0)) = (1 << index);
                }
                ++index;
                pendingIrqs >>= 1;
            }
            index = 0;
            pendingIrqs = HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_CCERR);
            if (pendingIrqs != 0) {
                // Process all the pending CC error interrupts.
                // Queue threshold error for different event queues.
                for (evtqueNum = 0; evtqueNum < EDMA3_0_NUM_EVTQUE; evtqueNum++)
                {
                    if ((pendingIrqs & (1 << evtqueNum)) != 0) {
                        // Clear the error interrupt.
                        HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_CCERRCLR) = (1 << evtqueNum);
                    }
                }

                // Transfer completion code error.
                if ((pendingIrqs & (1 << EDMA3CC_CCERR_TCCERR_SHIFT)) != 0) {
                    HWREG(SOC_EDMA30CC_0_REGS + EDMA3CC_CCERRCLR) = \
                        (0x01 << EDMA3CC_CCERR_TCCERR_SHIFT);
                }
                ++index;
            }
            Cnt++;
        }
    }
}

// Called from assembly code in startup.s. After this, the "main" function in
// lib/pbio/sys/main.c is called. That contains all calls to the driver
// initialization (low level in pbdrv, high level in pbio), and system level
// functions for running user code (currently a hardcoded MicroPython script).
void SystemInit(void) {

    SysCfgRegistersUnlock();

    copy_vector_table();

    // Initialize advanced interrupt controller (AINTC)
    IntAINTCInit();
    IntMasterIRQEnable();
    IntGlobalEnable();
    IntIRQEnable();
    IntMasterFIQEnable();
    IntFIQEnable();

    // Initialization of EDMA3
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_CC0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_TC0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);
    EDMA3Init(SOC_EDMA30CC_0_REGS, 0); // Que num 0
    IntRegister(SYS_INT_CCINT0, Edma3ComplHandlerIsr);
    IntChannelSet(SYS_INT_CCINT0, 2);
    IntSystemEnable(SYS_INT_CCINT0);
    IntRegister(SYS_INT_CCERRINT, Edma3CCErrHandlerIsr);
    IntChannelSet(SYS_INT_CCERRINT, 2);
    IntSystemEnable(SYS_INT_CCERRINT);


    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_GPIO, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    // Must set the power enable bin before disabling the pull up on the power
    // pin below, otherwise the hub will power off.
    pbdrv_reset_init();

    // Disable all pull-up/pull-down groups.
    HWREG(SOC_SYSCFG_1_REGS + SYSCFG1_PUPD_ENA) &= ~0xFFFFFFFF;

    // UART for Bluetooth. Other UARTS are configured by port module.
    // TODO: Add CTS/RTS pins.
    const pbdrv_gpio_t bluetooth_uart_rx = PBDRV_GPIO_EV3_PIN(4, 19, 16, 1, 3);
    const pbdrv_gpio_t bluetooth_uart_tx = PBDRV_GPIO_EV3_PIN(4, 23, 20, 1, 2);
    pbdrv_gpio_alt(&bluetooth_uart_rx, SYSCFG_PINMUX4_PINMUX4_19_16_UART2_RXD);
    pbdrv_gpio_alt(&bluetooth_uart_tx, SYSCFG_PINMUX4_PINMUX4_23_20_UART2_TXD);
}


#include <pbio/button.h>
#include <pbdrv/reset.h>

/*
 * This is called from the IRQ handler after every systick. This should be
 * removed when the final user interface is implemented. For now this serves
 * as a very convenient way to power off the EV3 for fast iteration.
 */
void lazy_poweroff_hook(void) {
    pbio_button_flags_t flags;
    pbio_error_t err = pbio_button_is_pressed(&flags);
    if (err == PBIO_SUCCESS && (flags & PBIO_BUTTON_LEFT_UP)) {
        pbdrv_reset_power_off();
        return;
    }
}
