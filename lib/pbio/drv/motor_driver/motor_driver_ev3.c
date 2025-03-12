// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors
//
// ECAP and EHRPWM init config based on the ev3ninja/osek project:
//   SPDX-License-Identifier: MPL-1.0
//   Copyright (c) 2016 Bektur Marat uulu and Bektur Toktosunov.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_EV3

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/motor_driver.h>
#include <pbio/error.h>

#include <pbdrv/gpio.h>
#include "../gpio/gpio_ev3.h"

#include <tiam1808/ehrpwm.h>
#include <tiam1808/ecap.h>
#include <tiam1808/psc.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_tmr.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/interrupt.h>

#include <pbio/int_math.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#include <inttypes.h>
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define debug_pr pbdrv_uart_debug_printf
#define DBG_ERR(expr) expr
#else
#define debug_pr(...)
#define DBG_ERR(expr)
#endif

typedef struct {
    uint32_t write_address;
    pbdrv_gpio_t pin1;
    pbdrv_gpio_t pin2;
} pbdrv_motor_driver_pwm_t;

typedef struct {
    pbdrv_gpio_t enable;
} pbdrv_motor_driver_sensor_t;

struct _pbdrv_motor_driver_dev_t {
    /**
     * True if the motor driver supports PWM.
     *
     * When true, the `pwm` field is used, otherwise the `sensor` field is
     * used.
     */
    bool supports_pwm;
    union {
        pbdrv_motor_driver_pwm_t pwm;
        pbdrv_motor_driver_sensor_t sensor;
    };
};

// Motors A/B/C/D, then sensors 1/2/3/4 (power on-off only).
static pbdrv_motor_driver_dev_t motor_drivers[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    [0] = {
        .supports_pwm = true,
        .pwm = {
            .write_address = SOC_EHRPWM_1_REGS + EHRPWM_CMPB,
            .pin1 = PBDRV_GPIO_EV3_PIN(7, 3, 0, 3, 15),
            .pin2 = PBDRV_GPIO_EV3_PIN(8, 7, 4, 3, 6),
        },
    },
    [1] = {
        .supports_pwm = true,
        .pwm = {
            .write_address = SOC_EHRPWM_1_REGS + EHRPWM_CMPA,
            .pin1 = PBDRV_GPIO_EV3_PIN(6, 27, 24, 2, 1),
            .pin2 = PBDRV_GPIO_EV3_PIN(1, 19, 16, 0, 3),
        },
    },
    [2] = {
        .supports_pwm = true,
        .pwm = {
            .write_address = SOC_ECAP_0_REGS + ECAP_CAP2,
            .pin1 = PBDRV_GPIO_EV3_PIN(13, 31, 28, 6, 8),
            .pin2 = PBDRV_GPIO_EV3_PIN(11, 27, 24, 5, 9),
        },
    },
    [3] = {
        .supports_pwm = true,
        .pwm = {
            .write_address = SOC_ECAP_1_REGS + ECAP_CAP2,
            .pin1 = PBDRV_GPIO_EV3_PIN(12, 19, 16, 5, 3),
            .pin2 = PBDRV_GPIO_EV3_PIN(11, 23, 20, 5, 10),
        },
    },
    [4] = {
        .supports_pwm = false,
        .sensor = {
            .enable = PBDRV_GPIO_EV3_PIN(18, 31, 28, 8, 10),
        },
    },
    [5] = {
        .supports_pwm = false,
        .sensor = {
            .enable = PBDRV_GPIO_EV3_PIN(18, 23, 20, 8, 12),
        },
    },
    [6] = {
        .supports_pwm = false,
        .sensor = {
            .enable = PBDRV_GPIO_EV3_PIN(19, 3, 0, 8, 9),
        },
    },
    [7] = {
        .supports_pwm = false,
        .sensor = {
            .enable = PBDRV_GPIO_EV3_PIN(19, 11, 8, 6, 4),
        },
    },
};

pbio_error_t pbdrv_motor_driver_get_dev(uint8_t id, pbdrv_motor_driver_dev_t **driver) {
    if (id >= PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *driver = &motor_drivers[id];

    return PBIO_SUCCESS;
}

#define PWM_COUNT_MAX (10000)

pbio_error_t pbdrv_motor_driver_coast(pbdrv_motor_driver_dev_t *driver) {

    // For sensor ports coast turns off the power.
    if (!driver->supports_pwm) {
        pbdrv_gpio_out_low(&driver->sensor.enable);
        return PBIO_SUCCESS;
    }

    HWREGH(driver->pwm.write_address) = 0;
    pbdrv_gpio_out_low(&driver->pwm.pin1);
    pbdrv_gpio_out_low(&driver->pwm.pin2);
    return PBIO_SUCCESS;
}


pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbdrv_motor_driver_dev_t *driver, int16_t duty_cycle) {

    // EV3 sensors ports support power in only one direction.
    if (driver > &motor_drivers[3]) {
        if (duty_cycle == -PBDRV_MOTOR_DRIVER_MAX_DUTY) {
            pbdrv_gpio_out_high(&driver->sensor.enable);
        } else {
            pbdrv_gpio_out_low(&driver->sensor.enable);
        }
        return PBIO_SUCCESS;
    }

    // Cap duty cycle.
    duty_cycle = pbio_int_math_clamp(duty_cycle, PBDRV_MOTOR_DRIVER_MAX_DUTY);

    // Apply PWM and low to the respective pins depending on direction.
    // When one pin is set to input, the separate PWM pin takes over.
    if (duty_cycle == 0) {
        pbdrv_gpio_out_high(&driver->pwm.pin1);
        pbdrv_gpio_out_high(&driver->pwm.pin2);
    } else if (duty_cycle > 0) {
        pbdrv_gpio_out_high(&driver->pwm.pin1);
        pbdrv_gpio_input(&driver->pwm.pin2);
    } else {
        pbdrv_gpio_input(&driver->pwm.pin1);
        pbdrv_gpio_out_high(&driver->pwm.pin2);
    }

    // Apply duty cycle to PWM device.
    HWREGH(driver->pwm.write_address) = (uint16_t)(pbio_int_math_abs(duty_cycle) * (PWM_COUNT_MAX / PBDRV_MOTOR_DRIVER_MAX_DUTY));
    return PBIO_SUCCESS;
}

void pbdrv_motor_driver_init_ehrpwm(void) {

    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_EHRPWM, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    const pbdrv_gpio_t epwm1a = PBDRV_GPIO_EV3_PIN(5, 3, 0, 2, 15);
    pbdrv_gpio_alt(&epwm1a, SYSCFG_PINMUX5_PINMUX5_3_0_EPWM1A);

    const pbdrv_gpio_t epwm1b = PBDRV_GPIO_EV3_PIN(5, 7, 4, 2, 14);
    pbdrv_gpio_alt(&epwm1b, SYSCFG_PINMUX5_PINMUX5_7_4_EPWM1B);

    /* Enable PWM Clock in chip config reg 1 */
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_CFGCHIP1) |= SYSCFG_CFGCHIP1_TBCLKSYNC;

    /* Setting write_address of EHRPWM1 */
    uint32_t write_address = SOC_EHRPWM_1_REGS;

    /* TimeBase configuration */
    /* Setting time base period counter (PwmFrequency) to 10000-1. TBPRD = 10000-1*/
    HWREGH(write_address + EHRPWM_TBPRD) = (uint16_t)PWM_COUNT_MAX - 1; /** for Motor A and Motor B */

    /* Set TBCTL to (TB_UP | TB_DISABLE | TB_SHADOW | TB_SYNC_DISABLE | TB_HDIV1 | TB_DIV1 | TB_COUNT_UP) */
    /* Setting counter direction to up. CTRMODE = TB_UP */
    HWREGH(write_address + EHRPWM_TBCTL) = (HWREGH(write_address + EHRPWM_TBCTL) & (~EHRPWM_COUNTER_MODE_MASK)) | ((EHRPWM_COUNT_UP <<
            EHRPWM_TBCTL_CTRMODE_SHIFT) & EHRPWM_COUNTER_MODE_MASK);

    /* Disable synchronization. PHSEN = TB_DISABLE*/
    EHRPWMTimebaseSyncDisable(SOC_EHRPWM_1_REGS);

    /* Disable shadow write. PRDLD = TB_SHADOW*/
    HWREGH(write_address + EHRPWM_TBCTL) = (HWREGH(write_address + EHRPWM_TBCTL) & (~EHRPWM_PRD_LOAD_SHADOW_MASK)) | ((EHRPWM_SHADOW_WRITE_DISABLE <<
            EHRPWM_TBCTL_PRDLD_SHIFT) & EHRPWM_PRD_LOAD_SHADOW_MASK);

    /* Disable syncout. SYNCOSEL = TB_SYNC_DISABLE*/
    EHRPWMSyncOutModeSet(SOC_EHRPWM_1_REGS, EHRPWM_SYNCOUT_DISABLE);

    /* Set HSPCLKDIV = TB_DIV1 */
    HWREGH(write_address + EHRPWM_TBCTL) = (HWREGH(write_address + EHRPWM_TBCTL) &
        (~EHRPWM_TBCTL_HSPCLKDIV)) | ((EHRPWM_TBCTL_HSPCLKDIV_DIVBY1 <<
                EHRPWM_TBCTL_HSPCLKDIV_SHIFT) & EHRPWM_TBCTL_HSPCLKDIV);

    /* Set CLKDIV = TB_DIV1 */
    HWREGH(write_address + EHRPWM_TBCTL) = (HWREGH(write_address + EHRPWM_TBCTL) &
        (~EHRPWM_TBCTL_CLKDIV)) | ((EHRPWM_TBCTL_CLKDIV_DIVBY1 <<
                EHRPWM_TBCTL_CLKDIV_SHIFT) & EHRPWM_TBCTL_CLKDIV);

    /* Configure CMPCTL to
    SHDWAMODE = CC_SHADOW
    SHDWBMODE = CC_SHADOW
    LOADAMODE = CC_CTR_ZERO
    LOADBMODE = CC_CTR_ZERO */

    HWREGH(write_address + EHRPWM_CMPCTL) = (HWREGH(write_address + EHRPWM_CMPCTL) &
        (~EHRPWM_CMPCTL_SHDWAMODE)) | ((EHRPWM_SHADOW_WRITE_ENABLE <<
                EHRPWM_CMPCTL_SHDWAMODE_SHIFT) & EHRPWM_CMPCTL_SHDWAMODE);

    HWREGH(write_address + EHRPWM_CMPCTL) = (HWREGH(write_address + EHRPWM_CMPCTL)
        & (~EHRPWM_CMPCTL_SHDWBMODE)) | ((EHRPWM_SHADOW_WRITE_ENABLE <<
                EHRPWM_CMPCTL_SHDWBMODE_SHIFT) & EHRPWM_CMPCTL_SHDWBMODE);

    HWREGH(write_address + EHRPWM_CMPCTL) = (HWREGH(write_address + EHRPWM_CMPCTL) &
        (~EHRPWM_COMPA_LOAD_MASK)) | ((EHRPWM_COMPA_LOAD_COUNT_EQUAL_ZERO <<
                EHRPWM_CMPCTL_LOADAMODE_SHIFT) & EHRPWM_COMPA_LOAD_MASK);

    HWREGH(write_address + EHRPWM_CMPCTL) = (HWREGH(write_address + EHRPWM_CMPCTL) &
        (~EHRPWM_COMPB_LOAD_MASK)) | ((EHRPWM_COMPB_LOAD_COUNT_EQUAL_ZERO <<
                EHRPWM_CMPCTL_LOADBMODE_SHIFT) & EHRPWM_COMPB_LOAD_MASK);


    /* Configure Action qualifier */
    /* Configure AQCTLA */
    EHRPWMConfigureAQActionOnA(SOC_EHRPWM_1_REGS, EHRPWM_AQCTLA_ZRO_EPWMXALOW, EHRPWM_AQCTLA_PRD_DONOTHING,
        EHRPWM_AQCTLA_CAU_EPWMXAHIGH,  EHRPWM_AQCTLA_CAD_DONOTHING,  EHRPWM_AQCTLA_CBU_DONOTHING,
        EHRPWM_AQCTLA_CBD_DONOTHING, EHRPWM_AQSFRC_ACTSFA_DONOTHING);

    /* Configure AQCTLB */
    EHRPWMConfigureAQActionOnB(SOC_EHRPWM_1_REGS, EHRPWM_AQCTLB_ZRO_EPWMXBLOW, EHRPWM_AQCTLB_PRD_DONOTHING,
        EHRPWM_AQCTLB_CAU_DONOTHING,  EHRPWM_AQCTLB_CAD_DONOTHING,  EHRPWM_AQCTLB_CBU_EPWMXBHIGH,
        EHRPWM_AQCTLB_CBD_DONOTHING, EHRPWM_AQSFRC_ACTSFB_DONOTHING);
}

void pbdrv_motor_driver_init_ecap(void) {

    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_ECAP0_1_2, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    const pbdrv_gpio_t ecap0 = PBDRV_GPIO_EV3_PIN(2, 31, 28, 8, 7);
    pbdrv_gpio_alt(&ecap0, SYSCFG_PINMUX2_PINMUX2_31_28_ECAP0);

    const pbdrv_gpio_t ecap1 = PBDRV_GPIO_EV3_PIN(1, 31, 28, 0, 0);
    pbdrv_gpio_alt(&ecap1, SYSCFG_PINMUX1_PINMUX1_31_28_ECAP1);

    /* Enable PWM(ecap) Clock in chip config reg 1 */
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_CFGCHIP1) |= SYSCFG_CFGCHIP1_CAP0SRC;
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_CFGCHIP1) |= SYSCFG_CFGCHIP1_CAP1SRC;

    /* eCAP modules - APWM. Copied from official linux implementation */
    HWREGH(SOC_ECAP_0_REGS + ECAP_TSCTR) = 0;
    HWREGH(SOC_ECAP_1_REGS + ECAP_TSCTR) = 0;
    HWREGH(SOC_ECAP_0_REGS + ECAP_CTRPHS) = 0;
    HWREGH(SOC_ECAP_1_REGS + ECAP_CTRPHS) = 0;
    HWREGH(SOC_ECAP_0_REGS + ECAP_ECCTL2) = 0x0690;
    HWREGH(SOC_ECAP_1_REGS + ECAP_ECCTL2) = 0x0690;
    HWREGH(SOC_TMR_3_REGS + TMR_TGCR) = 0x00003304;
    HWREGH(SOC_TMR_3_REGS + TMR_TGCR) |= 0x00000002;

    /* TimeBase configuration */
    /* Setting time base period counter (PwmFrequency) to 10000-1. TBPRD = 10000-1*/
    HWREGH(SOC_ECAP_0_REGS + ECAP_CAP1) = (uint16_t)(PWM_COUNT_MAX - 1);
    HWREGH(SOC_ECAP_1_REGS + ECAP_CAP1) = (uint16_t)(PWM_COUNT_MAX - 1);
}

void pbdrv_motor_driver_init(void) {

    for (int i = 0; i < PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV; i++) {
        pbdrv_motor_driver_dev_t *dev = &motor_drivers[i];
        // Init as coasting. This also initializes the GPIO pins.
        pbdrv_motor_driver_coast(dev);
    }

    // Activate PWM after above pins are safely set to coast.
    pbdrv_motor_driver_init_ehrpwm();
    pbdrv_motor_driver_init_ecap();
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_EV3
