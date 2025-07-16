// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_SOUND_EV3

#include <math.h>
#include <stdint.h>

#include <pbdrv/gpio.h>

#include <tiam1808/ehrpwm.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/psc.h>

#include "../drv/gpio/gpio_ev3.h"

// This module covers the frequency range from 64 Hz to 10 kHz.
// It uses a maximum duty cycle of 1/16, and it controls volume
// by shortening the duty cycle. The hardware has a resolution
// of 16 bits for the period counter, and we want 8 bits for the volume,
// so we use the following timebase division factors to make this work:
//
// 64 Hz - 900 Hz   => /40
// 900 Hz - 8 kHz   => /4
// 8 kHz - 10 kHz   => /1

// Audio amplifier enable
static const pbdrv_gpio_t pin_sound_en = PBDRV_GPIO_EV3_PIN(13, 3, 0, 6, 15);
// Audio output pin
#define SYSCFG_PINMUX3_PINMUX3_7_4_GPIO0_0 0
static const pbdrv_gpio_t pin_audio = PBDRV_GPIO_EV3_PIN(3, 7, 4, 0, 0);

void pbdrv_sound_stop() {
    // Force the output low
    EHRPWMAQContSWForceOnB(SOC_EHRPWM_0_REGS, EHRPWM_AQCSFRC_CSFB_LOW, EHRPWM_AQSFRC_RLDCSF_IMMEDIATE);
    // Clean up counter
    HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) |= EHRPWM_TBCTL_CTRMODE_STOPFREEZE;
    EHRPWMWriteTBCount(SOC_EHRPWM_0_REGS, 0);
}

void pbdrv_beep_start(uint32_t frequency, uint16_t sample_attenuator) {
    // Clamp the frequency into the supported range
    if (frequency < 64) {
        frequency = 64;
    }
    if (frequency > 10000) {
        frequency = 10000;
    }

    // Clamp the volume into the supported range
    if (sample_attenuator > INT16_MAX) {
        sample_attenuator = INT16_MAX;
    }
    // Extract bits[14:7] for our 8-bit volume resolution
    sample_attenuator >>= 7;

    // Configure the timebase depending on which bucket the frequency is in.
    // Don't use EHRPWMTimebaseClkConfig because its calculation algorithm
    // isn't very good and is also tricky to fix.
    uint32_t timebase_div;
    if (frequency < 900) {
        timebase_div = 40;
        HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) = (HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) &
            ~(EHRPWM_TBCTL_CLKDIV | EHRPWM_TBCTL_HSPCLKDIV)) |
            (EHRPWM_TBCTL_CLKDIV_DIVBY4 << EHRPWM_TBCTL_CLKDIV_SHIFT) |
            (EHRPWM_TBCTL_HSPCLKDIV_DIVBY10 << EHRPWM_TBCTL_HSPCLKDIV_SHIFT);
    } else if (frequency < 8000) {
        timebase_div = 4;
        HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) = (HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) &
            ~(EHRPWM_TBCTL_CLKDIV | EHRPWM_TBCTL_HSPCLKDIV)) |
            (EHRPWM_TBCTL_CLKDIV_DIVBY4 << EHRPWM_TBCTL_CLKDIV_SHIFT) |
            (EHRPWM_TBCTL_HSPCLKDIV_DIVBY1 << EHRPWM_TBCTL_HSPCLKDIV_SHIFT);
    } else {
        timebase_div = 1;
        HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) = (HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) &
            ~(EHRPWM_TBCTL_CLKDIV | EHRPWM_TBCTL_HSPCLKDIV)) |
            (EHRPWM_TBCTL_CLKDIV_DIVBY1 << EHRPWM_TBCTL_CLKDIV_SHIFT) |
            (EHRPWM_TBCTL_HSPCLKDIV_DIVBY1 << EHRPWM_TBCTL_HSPCLKDIV_SHIFT);
    }

    // The way that this code controls volume by adjusting the duty cycle
    // is not linear across the frequency range. For a basic beep driver,
    // this empirical formula compensates well enough.
    uint32_t freq_adj = powf(2, log10f(frequency) - 3) * 256;

    uint32_t pwm_period = (SOC_EHRPWM_0_MODULE_FREQ / timebase_div + frequency / 2) / frequency;
    uint32_t pwm_duty_cycle = (pwm_period * sample_attenuator / 256) / 16 * freq_adj / 256;

    // Program PWM to generate a square wave of this frequency + duty cycle
    EHRPWMLoadCMPB(SOC_EHRPWM_0_REGS, pwm_duty_cycle, true, 0, true);
    EHRPWMPWMOpPeriodSet(SOC_EHRPWM_0_REGS, pwm_period, EHRPWM_COUNT_UP, true);

    // Stop forcing output low
    EHRPWMAQContSWForceOnB(SOC_EHRPWM_0_REGS, 0, EHRPWM_AQSFRC_RLDCSF_IMMEDIATE);
}

void pbdrv_sound_init() {
    // Turn on EPWM
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_EHRPWM, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    // The stop function performs various initializations
    pbdrv_sound_stop();

    // Program EPWM to generate the desired wave shape
    // Pulse goes high @ t=0
    // Pulse goes low  @ t=CMPB
    EHRPWMConfigureAQActionOnB(
        SOC_EHRPWM_0_REGS,
        EHRPWM_AQCTLB_ZRO_EPWMXBHIGH,
        EHRPWM_AQCTLB_PRD_DONOTHING,
        EHRPWM_AQCTLB_CAU_DONOTHING,
        EHRPWM_AQCTLB_CAD_DONOTHING,
        EHRPWM_AQCTLB_CBU_EPWMXBLOW,
        EHRPWM_AQCTLB_CBD_DONOTHING,
        EHRPWM_AQSFRC_ACTSFB_DONOTHING
        );
    // Disable unused features
    EHRPWMDBOutput(SOC_EHRPWM_0_REGS, EHRPWM_DBCTL_OUT_MODE_BYPASS);
    EHRPWMChopperDisable(SOC_EHRPWM_0_REGS);
    EHRPWMTZTripEventDisable(SOC_EHRPWM_0_REGS, false);
    EHRPWMTZTripEventDisable(SOC_EHRPWM_0_REGS, true);

    // Configure IO pin mode
    pbdrv_gpio_alt(&pin_audio, SYSCFG_PINMUX3_PINMUX3_7_4_EPWM0B);
    // Turn speaker amplifier on
    // We turn the amplifier on and leave it turned on, because otherwise
    // it will generate a popping sound whenever it is enabled.
    pbdrv_gpio_out_high(&pin_sound_en);
}

#endif // PBDRV_CONFIG_SOUND_EV3
