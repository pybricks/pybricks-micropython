// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors
//
// SPDX-License-Identifier: MPL-1.0
// Copyright (c) 2016 Tobias Schießl (System Init / partial pinmux / boot order / exceptionhandler)

#include <tiam1808/psc.h>
#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/evmAM1808.h>

#include <pbdrv/ioport.h>
#include <pbio/port_interface.h>

#include "../../drv/button/button_gpio.h"
#include "../../drv/gpio/gpio_tiam1808.h"
#include "../../drv/led/led_array_pwm.h"
#include "../../drv/led/led_dual.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/pwm/pwm_tiam1808.h"
#include "../../drv/uart/uart_ev3.h"

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
        .gpio_red = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 13,
                .mux_mask = SYSCFG_PINMUX13_PINMUX13_11_8,
                .mux_shift = SYSCFG_PINMUX13_PINMUX13_11_8_SHIFT,
                .gpio_bank_id = 6,
                .gpio_mode = SYSCFG_PINMUX13_PINMUX13_11_8_GPIO6_13,
            },
            .pin = 13
        },
        .gpio_green = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 14,
                .mux_mask = SYSCFG_PINMUX14_PINMUX14_3_0,
                .mux_shift = SYSCFG_PINMUX14_PINMUX14_3_0_SHIFT,
                .gpio_bank_id = 6,
                .gpio_mode = SYSCFG_PINMUX14_PINMUX14_3_0_GPIO6_7,
            },
            .pin = 7
        },

    },
    {
        .id = PWM_DEV_1_TODO,
        .gpio_red = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 13,
                .mux_mask = SYSCFG_PINMUX13_PINMUX13_15_12,
                .mux_shift = SYSCFG_PINMUX13_PINMUX13_15_12_SHIFT,
                .gpio_bank_id = 6,
                .gpio_mode = SYSCFG_PINMUX13_PINMUX13_15_12_GPIO6_12,
            },
            .pin = 12
        },
        .gpio_green = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 13,
                .mux_mask = SYSCFG_PINMUX13_PINMUX13_7_4,
                .mux_shift = SYSCFG_PINMUX13_PINMUX13_7_4_SHIFT,
                .gpio_bank_id = 6,
                .gpio_mode = SYSCFG_PINMUX13_PINMUX13_7_4_GPIO6_14,
            },
            .pin = 14
        },
    },
};

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 14,
                .mux_mask = SYSCFG_PINMUX14_PINMUX14_7_4,
                .mux_shift = SYSCFG_PINMUX14_PINMUX14_7_4_SHIFT,
                .gpio_bank_id = 6,
                .gpio_mode = SYSCFG_PINMUX14_PINMUX14_7_4_GPIO6_6,
            },
            .pin = 6
        },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_LEFT,
        .active_low = false,
    },
    [1] = {
        .gpio = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 16,
                .mux_mask = SYSCFG_PINMUX16_PINMUX16_23_20,
                .mux_shift = SYSCFG_PINMUX16_PINMUX16_23_20_SHIFT,
                .gpio_bank_id = 7,
                .gpio_mode = SYSCFG_PINMUX16_PINMUX16_23_20_GPIO7_12,
            },
            .pin = 12
        },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_RIGHT,
        .active_low = false,
    },
    [2] = {
        .gpio = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 16,
                .mux_mask = SYSCFG_PINMUX16_PINMUX16_11_8,
                .mux_shift = SYSCFG_PINMUX16_PINMUX16_11_8_SHIFT,
                .gpio_bank_id = 7,
                .gpio_mode = SYSCFG_PINMUX16_PINMUX16_11_8_GPIO7_15,
            },
            .pin = 15
        },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_UP,
        .active_low = false,
    },
    [3] = {
        .gpio = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 16,
                .mux_mask = SYSCFG_PINMUX16_PINMUX16_15_12,
                .mux_shift = SYSCFG_PINMUX16_PINMUX16_15_12_SHIFT,
                .gpio_bank_id = 7,
                .gpio_mode = SYSCFG_PINMUX16_PINMUX16_15_12_GPIO7_14,
            },
            .pin = 14
        },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_DOWN,
        .active_low = false,
    },
    [4] = {
        .gpio = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 2,
                .mux_mask = SYSCFG_PINMUX2_PINMUX2_11_8,
                .mux_shift = SYSCFG_PINMUX2_PINMUX2_11_8_SHIFT,
                .gpio_bank_id = 1,
                .gpio_mode = SYSCFG_PINMUX2_PINMUX2_11_8_GPIO1_13,
            },
            .pin = 13
        },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_CENTER,
        .active_low = false,
    },
    [5] = {
        .gpio = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 13,
                .mux_mask = SYSCFG_PINMUX13_PINMUX13_23_20,
                .mux_shift = SYSCFG_PINMUX13_PINMUX13_23_20_SHIFT,
                .gpio_bank_id = 6,
                .gpio_mode = SYSCFG_PINMUX13_PINMUX13_23_20_GPIO6_10,
            },
            .pin = 10
        },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_LEFT_UP,
        .active_low = false,
    },
};

// TODO.
const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin = {
    .bank = NULL,
    .pin = 0,
};

// UART

enum {
    UART_PORT_S1,
    UART_PORT_S2,
};

static void pbdrv_uart_ev3_handle_irq_s1(void) {
    pbdrv_uart_ev3_handle_irq(UART_PORT_S1);
}

static void pbdrv_uart_ev3_handle_irq_s2(void) {
    pbdrv_uart_ev3_handle_irq(UART_PORT_S2);
}

const pbdrv_uart_ev3_platform_data_t
    pbdrv_uart_ev3_platform_data[PBDRV_CONFIG_UART_EV3_NUM_UART] = {
    [UART_PORT_S1] = {
        .base_address = SOC_UART_1_REGS,
        .psc_peripheral_id = HW_PSC_UART1,
        .sys_int_uart_int_id = SYS_INT_UARTINT1,
        .isr_handler = pbdrv_uart_ev3_handle_irq_s1,
        .pin_rx = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 4,
                .mux_mask = SYSCFG_PINMUX4_PINMUX4_27_24,
                .mux_shift = SYSCFG_PINMUX4_PINMUX4_27_24_SHIFT,
                .gpio_bank_id = 1,
                .gpio_mode = SYSCFG_PINMUX4_PINMUX4_27_24_GPIO1_1,
            },
            .pin = 1,
        },
        .pin_rx_mux = SYSCFG_PINMUX4_PINMUX4_27_24_UART1_RXD,
        .pin_tx = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 4,
                .mux_mask = SYSCFG_PINMUX4_PINMUX4_31_28,
                .mux_shift = SYSCFG_PINMUX4_PINMUX4_31_28_SHIFT,
                .gpio_bank_id = 1,
                .gpio_mode = SYSCFG_PINMUX4_PINMUX4_31_28_GPIO1_0,
            },
            .pin = 0,
        },
        .pin_tx_mux = SYSCFG_PINMUX4_PINMUX4_31_28_UART1_TXD,
    },
    [UART_PORT_S2] = {
        .base_address = SOC_UART_0_REGS,
        .psc_peripheral_id = HW_PSC_UART0,
        .sys_int_uart_int_id = SYS_INT_UARTINT0,
        .isr_handler = pbdrv_uart_ev3_handle_irq_s2,
        .pin_rx = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 14,
                .mux_mask = SYSCFG_PINMUX3_PINMUX3_19_16,
                .mux_shift = SYSCFG_PINMUX3_PINMUX3_19_16_SHIFT,
                .gpio_bank_id = 8,
                .gpio_mode = SYSCFG_PINMUX3_PINMUX3_19_16_GPIO8_4,
            },
            .pin = 4,
        },
        .pin_rx_mux = SYSCFG_PINMUX3_PINMUX3_19_16_UART0_RXD,
        .pin_tx = {
            .bank = &(pbdrv_gpio_tiam1808_mux_t) {
                .mux_id = 3,
                .mux_mask = SYSCFG_PINMUX3_PINMUX3_23_20,
                .mux_shift = SYSCFG_PINMUX3_PINMUX3_23_20_SHIFT,
                .gpio_bank_id = 8,
                .gpio_mode = SYSCFG_PINMUX3_PINMUX3_23_20_GPIO8_3,
            },
            .pin = 3,
        },
        .pin_tx_mux = SYSCFG_PINMUX3_PINMUX3_23_20_UART0_TXD,
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

// port S1 buffer enable. Revisit: Move to legodev/ioport driver.
static const pbdrv_gpio_t pin_port_s1_buffer_enable = {
    .bank = &(pbdrv_gpio_tiam1808_mux_t) {
        .mux_id = 18,
        .mux_mask = SYSCFG_PINMUX18_PINMUX18_27_24,
        .mux_shift = SYSCFG_PINMUX18_PINMUX18_27_24_SHIFT,
        .gpio_bank_id = 8,
        .gpio_mode = SYSCFG_PINMUX18_PINMUX18_27_24_GPIO8_11,
    },
    .pin = 11,
};

// port S2 buffer enable. Revisit: Move to legodev/ioport driver.
static const pbdrv_gpio_t pin_port_s2_buffer_enable = {
    .bank = &(pbdrv_gpio_tiam1808_mux_t) {
        .mux_id = 18,
        .mux_mask = SYSCFG_PINMUX18_PINMUX18_15_12,
        .mux_shift = SYSCFG_PINMUX18_PINMUX18_15_12_SHIFT,
        .gpio_bank_id = 8,
        .gpio_mode = SYSCFG_PINMUX18_PINMUX18_15_12_GPIO8_14,
    },
    .pin = 14,
};

// Called from assembly code in startup.s. After this, the "main" function in
// lib/pbio/sys/main.c is called. That contains all calls to the driver
// initialization (low level in pbdrv, high level in pbio), and system level
// functions for running user code (currently a hardcoded MicroPython script).
void SystemInit(void) {

    SysCfgRegistersUnlock();

    copy_vector_table();

    /* Initialize AINTC */
    IntAINTCInit();

    /* Enable IRQ for ARM (in CPSR)*/
    IntMasterIRQEnable();

    /* Enable AINTC interrupts in GER */
    IntGlobalEnable();

    /* Enable IRQ in AINTC */
    IntIRQEnable();

    IntMasterFIQEnable();
    IntFIQEnable();

    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_GPIO, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    // Initialize button GPIO mode. Button gpio driver does not have a hook for this.
    for (int i = 0; i < PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON; i++) {
        const pbdrv_gpio_t *gpio = &pbdrv_button_gpio_platform[i].gpio;
        const pbdrv_gpio_tiam1808_mux_t *mux_info = gpio->bank;
        pbdrv_gpio_alt(gpio, mux_info->gpio_mode);
    }

    // Allow port buffers to use UART. Revisit: Move to legodev/ioport driver.
    pbdrv_gpio_alt(&pin_port_s1_buffer_enable, ((pbdrv_gpio_tiam1808_mux_t *)pin_port_s1_buffer_enable.bank)->gpio_mode);
    pbdrv_gpio_out_low(&pin_port_s1_buffer_enable);
    pbdrv_gpio_alt(&pin_port_s2_buffer_enable, ((pbdrv_gpio_tiam1808_mux_t *)pin_port_s2_buffer_enable.bank)->gpio_mode);
    pbdrv_gpio_out_low(&pin_port_s2_buffer_enable);
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
