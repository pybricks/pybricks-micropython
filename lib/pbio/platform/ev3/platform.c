// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors
//
// SPDX-License-Identifier: MPL-1.0
// Copyright (c) 2016 Tobias Schießl (System Init / pinmux / boot order)

#include <tiam1808/psc.h>
#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/evmAM1808.h>

#include <pbdrv/ioport.h>
#include <pbio/port_interface.h>

#include "../../drv/button/button_gpio.h"
#include "../../drv/led/led_array_pwm.h"
#include "../../drv/led/led_dual.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/pwm/pwm_tiam1808.h"

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
        .gpio_red = { .bank = (void *)6, .pin = 13 },
        .gpio_green = { .bank = (void *)6, .pin = 7 },

    },
    {
        .id = PWM_DEV_1_TODO,
        .gpio_red = { .bank = (void *)6, .pin = 12 },
        .gpio_green = { .bank = (void *)6, .pin = 14 },
    },
};

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio = { .bank = (void *)6, .pin = 6 },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_LEFT,
        .active_low = false,
    },
    [1] = {
        .gpio = { .bank = (void *)7, .pin = 12 },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_RIGHT,
        .active_low = false,
    },
    [2] = {
        .gpio = { .bank = (void *)7, .pin = 15 },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_UP,
        .active_low = false,
    },
    [3] = {
        .gpio = { .bank = (void *)7, .pin = 14 },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_DOWN,
        .active_low = false,
    },
    [4] = {
        .gpio = { .bank = (void *)1, .pin = 13 },
        .pull = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_CENTER,
        .active_low = false,
    },
    [5] = {
        .gpio = { .bank = (void *)6, .pin = 10 },
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

static void set_gpio_mux(uint32_t id, uint32_t mask, uint32_t shift, uint32_t value) {
    // Generalized from GPIOBank4Pin0PinMuxSetup in TI evmAM1808 platform example.
    uint32_t mux = value << shift;
    uint32_t keep = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(id)) & ~(mask);
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(id)) = (mux | keep);
}


// Legacy hooks from EV3OSEK. Todo: remove callees from exceptionhandler.S
void user_1ms_isr_type2(void) {
}
uint8_t callevel;
static volatile unsigned long shouldDispatch = 0;
volatile unsigned int addrShouldDispatch = (unsigned int)&shouldDispatch;
void SetDispatch(void) {
}
extern void ExceptionHandler(void);

void copy_vector_table(void) {
    unsigned int *dest = (unsigned int *)0xFFFF0000;
    unsigned int *addrExceptionHandler = (unsigned int *)ExceptionHandler;
    int i = 1;

    for (; i < 8 + 2048; ++i) {
        dest[i] = addrExceptionHandler[i];
    }
}

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

    // Button GPIO.
    set_gpio_mux(14, SYSCFG_PINMUX14_PINMUX14_7_4,   SYSCFG_PINMUX14_PINMUX14_7_4_SHIFT,   SYSCFG_PINMUX14_PINMUX14_7_4_GPIO6_6);
    set_gpio_mux(16, SYSCFG_PINMUX16_PINMUX16_23_20, SYSCFG_PINMUX16_PINMUX16_23_20_SHIFT, SYSCFG_PINMUX16_PINMUX16_23_20_GPIO7_12);
    set_gpio_mux(16, SYSCFG_PINMUX16_PINMUX16_11_8,  SYSCFG_PINMUX16_PINMUX16_11_8_SHIFT,  SYSCFG_PINMUX16_PINMUX16_11_8_GPIO7_15);
    set_gpio_mux(16, SYSCFG_PINMUX16_PINMUX16_15_12, SYSCFG_PINMUX16_PINMUX16_15_12_SHIFT, SYSCFG_PINMUX16_PINMUX16_15_12_GPIO7_14);
    set_gpio_mux(2,  SYSCFG_PINMUX2_PINMUX2_11_8,    SYSCFG_PINMUX2_PINMUX2_11_8_SHIFT,    SYSCFG_PINMUX2_PINMUX2_11_8_GPIO1_13);
    set_gpio_mux(13, SYSCFG_PINMUX13_PINMUX13_23_20, SYSCFG_PINMUX13_PINMUX13_23_20_SHIFT, SYSCFG_PINMUX13_PINMUX13_23_20_GPIO6_10);

    // Status Light LED. TODO: Set as PWM
    set_gpio_mux(13, SYSCFG_PINMUX13_PINMUX13_11_8,  SYSCFG_PINMUX13_PINMUX13_11_8_SHIFT,  SYSCFG_PINMUX13_PINMUX13_11_8_GPIO6_13);
    set_gpio_mux(14, SYSCFG_PINMUX14_PINMUX14_3_0,   SYSCFG_PINMUX14_PINMUX14_3_0_SHIFT,   SYSCFG_PINMUX14_PINMUX14_3_0_GPIO6_7);
    set_gpio_mux(13, SYSCFG_PINMUX13_PINMUX13_15_12, SYSCFG_PINMUX13_PINMUX13_15_12_SHIFT, SYSCFG_PINMUX13_PINMUX13_15_12_GPIO6_12);
    set_gpio_mux(13, SYSCFG_PINMUX13_PINMUX13_7_4,   SYSCFG_PINMUX13_PINMUX13_7_4_SHIFT,   SYSCFG_PINMUX13_PINMUX13_7_4_GPIO6_14);

    // Poweroff pin.
    set_gpio_mux(13, SYSCFG_PINMUX13_PINMUX13_19_16, SYSCFG_PINMUX13_PINMUX13_19_16_SHIFT, SYSCFG_PINMUX13_PINMUX13_19_16_GPIO6_11);
}


#include <pbio/button.h>
#include <pbdrv/reset.h>

/*
 * This is called from the IRQ handler after every systick. This should be
 * removed when the final user interface is implemented. For now this serves
 * as a very convenient way to power off the EV3 for fast iteration.
 */
void check_EV3_buttons(void) {
    pbio_button_flags_t flags;
    pbio_error_t err = pbio_button_is_pressed(&flags);
    if (err == PBIO_SUCCESS && (flags & PBIO_BUTTON_LEFT_UP)) {
        pbdrv_reset_power_off();
        return;
    }
}
