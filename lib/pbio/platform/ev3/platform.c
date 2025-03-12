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
#include "../../drv/gpio/gpio_ev3.h"
#include "../../drv/led/led_array_pwm.h"
#include "../../drv/led/led_dual.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/pwm/pwm_ev3.h"
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

// TODO.
const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin = {
    .bank = NULL,
    .pin = 0,
};

// UART

// This order is arbitrary. It is the order of the internal state structures
// in the uart driver. When getting a uart driver handle, these indexes are
// used, but only within this platform file.
enum {
    UART0, // sensor 2
    UART1, // sensor 1
    UART2, // Bluetooth
    PRU0_LINE0, // sensor 4
    PRU0_LINE1, // sensor 3
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
    [UART0] = {
        .uart_kind = EV3_UART_HW,
        .base_address = SOC_UART_0_REGS,
        .peripheral_id = HW_PSC_UART0,
        .sys_int_uart_int_id = SYS_INT_UARTINT0,
        .isr_handler = pbdrv_uart_ev3_handle_irq_uart0,
        .pin_rx = PBDRV_GPIO_EV3_PIN(3, 19, 16, 8, 4),
        .pin_rx_mux = SYSCFG_PINMUX3_PINMUX3_19_16_UART0_RXD,
        .pin_tx = PBDRV_GPIO_EV3_PIN(3, 23, 20, 8, 3),
        .pin_tx_mux = SYSCFG_PINMUX3_PINMUX3_23_20_UART0_TXD,
    },
    [UART1] = {
        .uart_kind = EV3_UART_HW,
        .base_address = SOC_UART_1_REGS,
        .peripheral_id = HW_PSC_UART1,
        .sys_int_uart_int_id = SYS_INT_UARTINT1,
        .isr_handler = pbdrv_uart_ev3_handle_irq_uart1,
        .pin_rx = PBDRV_GPIO_EV3_PIN(4, 27, 24, 1, 1),
        .pin_rx_mux = SYSCFG_PINMUX4_PINMUX4_27_24_UART1_RXD,
        .pin_tx = PBDRV_GPIO_EV3_PIN(4, 31, 28, 1, 0),
        .pin_tx_mux = SYSCFG_PINMUX4_PINMUX4_31_28_UART1_TXD,
    },
    [UART2] = {
        // TODO: Add CTS/RTS pins.
        .uart_kind = EV3_UART_HW,
        .base_address = SOC_UART_2_REGS,
        .peripheral_id = HW_PSC_UART2,
        .sys_int_uart_int_id = SYS_INT_UARTINT2,
        .isr_handler = pbdrv_uart_ev3_handle_irq_uart2,
        .pin_rx = PBDRV_GPIO_EV3_PIN(4, 19, 16, 1, 3),
        .pin_rx_mux = SYSCFG_PINMUX4_PINMUX4_19_16_UART2_RXD,
        .pin_tx = PBDRV_GPIO_EV3_PIN(4, 23, 20, 1, 2),
        .pin_tx_mux = SYSCFG_PINMUX4_PINMUX4_23_20_UART2_TXD,
    },
    [PRU0_LINE0] = {
        .uart_kind = EV3_UART_PRU,
        .base_address = 0, // Not used.
        .peripheral_id = 0, // Soft UART line 0.
        .sys_int_uart_int_id = SYS_INT_EVTOUT0,
        .isr_handler = pbdrv_uart_ev3_handle_irq_pru0_line0,
        .pin_rx = PBDRV_GPIO_EV3_PIN(2, 27, 24, 1, 9),
        .pin_rx_mux = SYSCFG_PINMUX2_PINMUX2_27_24_AXR0_1,
        .pin_tx = PBDRV_GPIO_EV3_PIN(2, 19, 16, 1, 11),
        .pin_tx_mux = SYSCFG_PINMUX2_PINMUX2_19_16_AXR0_3,
    },
    [PRU0_LINE1] = {
        .uart_kind = EV3_UART_PRU,
        .base_address = 0, // Not used.
        .peripheral_id = 1, // Soft UART line 1.
        .sys_int_uart_int_id = SYS_INT_EVTOUT1,
        .isr_handler = pbdrv_uart_ev3_handle_irq_pru0_line1,
        .pin_rx = PBDRV_GPIO_EV3_PIN(2, 23, 20, 1, 10),
        .pin_rx_mux = SYSCFG_PINMUX2_PINMUX2_23_20_AXR0_2,
        .pin_tx = PBDRV_GPIO_EV3_PIN(2, 15, 12, 1, 12),
        .pin_tx_mux = SYSCFG_PINMUX2_PINMUX2_15_12_AXR0_4,
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
static const pbdrv_gpio_t pin_port_s1_buffer_enable = PBDRV_GPIO_EV3_PIN(18, 27, 24, 8, 11);

// port S2 buffer enable. Revisit: Move to legodev/ioport driver.
static const pbdrv_gpio_t pin_port_s2_buffer_enable = PBDRV_GPIO_EV3_PIN(18, 15, 12, 8, 14);

// port S3 buffer enable. Revisit: Move to legodev/ioport driver.
static const pbdrv_gpio_t pin_port_s3_buffer_enable = PBDRV_GPIO_EV3_PIN(17, 3, 0, 7, 9);

// port S4 buffer enable. Revisit: Move to legodev/ioport driver.
static const pbdrv_gpio_t pin_port_s4_buffer_enable = PBDRV_GPIO_EV3_PIN(16, 31, 28, 7, 10);

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
        const pbdrv_gpio_ev3_mux_t *mux_info = gpio->bank;
        pbdrv_gpio_alt(gpio, mux_info->gpio_mode);
    }

    // Allow port buffers to use UART. Revisit: Move to legodev/ioport driver.
    pbdrv_gpio_alt(&pin_port_s1_buffer_enable, ((pbdrv_gpio_ev3_mux_t *)pin_port_s1_buffer_enable.bank)->gpio_mode);
    pbdrv_gpio_out_low(&pin_port_s1_buffer_enable);
    pbdrv_gpio_alt(&pin_port_s2_buffer_enable, ((pbdrv_gpio_ev3_mux_t *)pin_port_s2_buffer_enable.bank)->gpio_mode);
    pbdrv_gpio_out_low(&pin_port_s2_buffer_enable);
    pbdrv_gpio_alt(&pin_port_s3_buffer_enable, ((pbdrv_gpio_ev3_mux_t *)pin_port_s3_buffer_enable.bank)->gpio_mode);
    pbdrv_gpio_out_low(&pin_port_s3_buffer_enable);
    pbdrv_gpio_alt(&pin_port_s4_buffer_enable, ((pbdrv_gpio_ev3_mux_t *)pin_port_s4_buffer_enable.bank)->gpio_mode);
    pbdrv_gpio_out_low(&pin_port_s4_buffer_enable);
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
