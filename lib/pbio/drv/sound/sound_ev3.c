// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_SOUND_EV3

#include <stdint.h>

#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/ehrpwm.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/psc.h>

#include "../drv/gpio/gpio_ev3.h"

// Audio amplifier enable
static const pbdrv_gpio_t pin_sound_en = PBDRV_GPIO_EV3_PIN(13, 3, 0, 6, 15);
// Audio output pin
#define SYSCFG_PINMUX3_PINMUX3_7_4_GPIO0_0 0
static const pbdrv_gpio_t pin_audio = PBDRV_GPIO_EV3_PIN(3, 7, 4, 0, 0);

// This hardware is not capable of producing 16 bits per sample
// at an acceptable sampling rate. As a trade-off, use 12 bits per sample
// giving a sampling rate of 150 MHz / 2**12 ~= 36 ksps
//
// Unlike other audio drivers, this one runs at a fixed sample rate.
// This is because the AM1808 has relatively few possible clock division ratios,
// and we do not want the usable bit depth to vary as the sample rate changes.
static const unsigned N_BITS_PER_SAMPLE = 12;

static uint64_t sample_timing_accum_numerator;
static uint32_t sample_idx;

static const uint16_t *playing_data;
static uint32_t playing_data_len;
static uint32_t playing_sample_rate;

static void sound_isr() {
    EHRPWMETIntClear(SOC_EHRPWM_0_REGS);
    IntSystemStatusClear(SYS_INT_EHRPWM0);

    // Convert the hardware sample index to a desired data sample index
    // (using a naive ratio, rearranged to be computable using integers)
    // TODO: Use a real DSP resampling algorithm
    sample_timing_accum_numerator += (uint64_t)playing_sample_rate * (1ull << N_BITS_PER_SAMPLE);
    if (sample_timing_accum_numerator >= SOC_EHRPWM_0_MODULE_FREQ) {
        sample_timing_accum_numerator -= SOC_EHRPWM_0_MODULE_FREQ;
        sample_idx++;
    }
    if (sample_idx == playing_data_len) {
        sample_idx = 0;
    }

    uint16_t sample = playing_data[sample_idx];
    // TODO: Dither the quantization error
    HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_CMPB) = sample >> (16 - N_BITS_PER_SAMPLE);
}

void pbdrv_sound_stop() {
    // Turn speaker amplifier off
    pbdrv_gpio_out_low(&pin_sound_en);
    // Clean up counter
    HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) |= EHRPWM_TBCTL_CTRMODE_STOPFREEZE;
    EHRPWMWriteTBCount(SOC_EHRPWM_0_REGS, 0);
    EHRPWMETIntDisable(SOC_EHRPWM_0_REGS);
    EHRPWMETIntClear(SOC_EHRPWM_0_REGS);
    // Disable shadowing and set the count to 0
    EHRPWMLoadCMPB(SOC_EHRPWM_0_REGS, 0, true, 0, true);
    // Re-enable shadowing
    EHRPWMLoadCMPB(SOC_EHRPWM_0_REGS, 0, false, EHRPWM_CMPCTL_LOADBMODE_TBCTRPRD, true);
}

void pbdrv_sound_start(const uint16_t *data, uint32_t length, uint32_t sample_rate) {
    // Stop any currently-playing sounds
    pbdrv_sound_stop();

    if (length == 0) {
        return;
    }

    __asm__ volatile ("" ::: "memory");
    playing_data = data;
    playing_data_len = length;
    playing_sample_rate = sample_rate;
    sample_idx = 0;
    sample_timing_accum_numerator = 0;
    __asm__ volatile ("" ::: "memory");

    // Set the first sample
    HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_CMPB) = data[0] >> (16 - N_BITS_PER_SAMPLE);

    // Enable all the sound generation
    pbdrv_gpio_out_high(&pin_sound_en);
    EHRPWMETIntEnable(SOC_EHRPWM_0_REGS);
    HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) = (HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) & ~EHRPWM_TBCTL_CTRMODE) | EHRPWM_TBCTL_CTRMODE_UP;
}

void pbdrv_sound_init() {
    // Turn on EPWM
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_EHRPWM, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    // The stop function performs various initializations
    pbdrv_sound_stop();

    // Set up settings which will stay consistent throughout
    EHRPWMTimebaseClkConfig(SOC_EHRPWM_0_REGS, SOC_EHRPWM_0_MODULE_FREQ, SOC_EHRPWM_0_MODULE_FREQ);
    // Set the period to go up to max of nbits
    HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBCTL) |= EHRPWM_TBCTL_PRDLD;
    HWREGH(SOC_EHRPWM_0_REGS + EHRPWM_TBPRD) = (1 << N_BITS_PER_SAMPLE) - 1;
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

    // Interrupts
    IntRegister(SYS_INT_EHRPWM0, sound_isr);
    IntChannelSet(SYS_INT_EHRPWM0, 1);
    IntSystemEnable(SYS_INT_EHRPWM0);
    EHRPWMETIntSourceSelect(SOC_EHRPWM_0_REGS, EHRPWM_ETSEL_INTSEL_TBCTREQUPRD);
    EHRPWMETIntPrescale(SOC_EHRPWM_0_REGS, EHRPWM_ETPS_INTPRD_FIRSTEVENT);

    // Configure IO pin mode
    pbdrv_gpio_alt(&pin_audio, SYSCFG_PINMUX3_PINMUX3_7_4_EPWM0B);
}

#endif // PBDRV_CONFIG_SOUND_EV3
