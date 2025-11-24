// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors
//
// P5/6 IRQ config based on the ev3ninja/osek project:
//   SPDX-License-Identifier: MPL-1.0
//   Copyright (c) 2016 Bektur Marat uulu and Bektur Toktosunov.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_EV3

#include <stdbool.h>
#include <stdint.h>

#include "../gpio/gpio_ev3.h"

#include <lego/device.h>

#include <pbdrv/adc.h>
#include <pbdrv/counter.h>
#include <pbdrv/gpio.h>
#include <pbio/int_math.h>
#include <pbio/util.h>

#include <tiam1808/gpio.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_gpio.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/interrupt.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#include <inttypes.h>
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define DEBUG_PRINT pbdrv_uart_debug_printf
#else
#define DEBUG_PRINT(...)
#endif

struct _pbdrv_counter_dev_t {
    int32_t count;
    pbdrv_gpio_t gpio_int;
    pbdrv_gpio_t gpio_dir;
    pbdrv_gpio_t gpio_det;
    uint8_t adc_channel;
    lego_device_type_id_t last_type_id;
    lego_device_type_id_t stable_type_id;
    uint32_t type_id_count;
};

static pbdrv_counter_dev_t counters[] = {
    {
        .count = 0,
        .gpio_int = PBDRV_GPIO_EV3_PIN(11, 19, 16, 5, 11),
        .gpio_dir = PBDRV_GPIO_EV3_PIN(1, 15, 12, 0, 4),
        .gpio_det = PBDRV_GPIO_EV3_PIN(12, 15, 12, 5, 4),
        .adc_channel = 1,
    },
    {
        .count = 0,
        .gpio_int = PBDRV_GPIO_EV3_PIN(11, 31, 28, 5, 8),
        .gpio_dir = PBDRV_GPIO_EV3_PIN(5, 27, 24, 2, 9),
        .gpio_det = PBDRV_GPIO_EV3_PIN(6, 11, 8, 2, 5),
        .adc_channel = 0,
    },
    {
        .count = 0,
        .gpio_int = PBDRV_GPIO_EV3_PIN(11, 11, 8, 5, 13),
        .gpio_dir = PBDRV_GPIO_EV3_PIN(7, 7, 4, 3, 14),
        .gpio_det = PBDRV_GPIO_EV3_PIN(7, 31, 28, 3, 8),
        .adc_channel = 13,
    },
    {
        .count = 0,
        .gpio_int = PBDRV_GPIO_EV3_PIN(13, 27, 24, 6, 9),
        .gpio_dir = PBDRV_GPIO_EV3_PIN(5, 31, 28, 2, 8),
        .gpio_det = PBDRV_GPIO_EV3_PIN(11, 3, 0, 5, 15),
        .adc_channel = 14,
    },
};

pbio_error_t pbdrv_counter_get_dev(uint8_t id, pbdrv_counter_dev_t **dev) {
    if (id >= PBIO_ARRAY_SIZE(counters)) {
        return PBIO_ERROR_NO_DEV;
    }
    *dev = &counters[id];
    return PBIO_SUCCESS;
}

#define ADC_EV3_NONE (505)
#define ADC_EV3_MEDIUM_0 (74)
#define ADC_EV3_MEDIUM_1 (862)
#define ADC_EV3_LARGE_0 (32)
#define ADC_EV3_LARGE_1 (917)
#define ADC_NXT_LARGE_1 (1014)
#define ADC_MINDSENSORS_GLIDEWHEEL_0 (0)
#define ADC_MINDSENSORS_GLIDEWHEEL_1 (1023)

static bool adc_is_close(uint32_t adc, uint32_t reference) {
    uint32_t error = adc > reference ? adc - reference : reference - adc;
    return error <= 21;
}

/**
 * Gets the LEGO device type ID for an EV3 motor based on the ADC value.
 *
 * Each motor has two values (low and high) depending on the quadrature encoder
 * state. The large motor is 4000 in the high state but in the low state it
 * is indistinguishable from the EV3 large motor.
 *
 * The original firmware uses a dynamic process to distinguish other non-motor
 * devices. This is not implemented here. It does not appear necessary for
 * motors.
 *
 * If we find that we occasionally get "in-between" values, we can have the
 * adc process poll us to maintain a minium count of unchanged states.
 */
static lego_device_type_id_t pbdrv_counter_ev3_get_type(uint16_t adc) {

    if (adc == ADC_MINDSENSORS_GLIDEWHEEL_0 || adc == ADC_MINDSENSORS_GLIDEWHEEL_1) {
        return LEGO_DEVICE_TYPE_ID_EV3_LARGE_MOTOR;
    }

    if (adc_is_close(adc, ADC_EV3_MEDIUM_0) || adc_is_close(adc, ADC_EV3_MEDIUM_1)) {
        return LEGO_DEVICE_TYPE_ID_EV3_MEDIUM_MOTOR;
    }

    if (adc_is_close(adc, ADC_EV3_LARGE_0) || adc_is_close(adc, ADC_EV3_LARGE_1) || adc_is_close(adc, ADC_NXT_LARGE_1)) {
        return LEGO_DEVICE_TYPE_ID_NXT_MOTOR;
    }

    return LEGO_DEVICE_TYPE_ID_NONE;
}

#define PBDRV_COUNTER_EV3_TYPE_LOOP_TIME (10)
#define PBDRV_COUNTER_EV3_TYPE_MIN_STABLE_COUNT (20)

/**
 * Updates the type of all EV3 motors based on the current ADC values.
 */
static void pbdrv_counter_ev3_update_type(void) {

    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(counters); i++) {

        // Get type detected now.
        pbdrv_counter_dev_t *dev = &counters[i];
        uint16_t adc = 0;
        pbdrv_adc_get_ch(dev->adc_channel, &adc);
        lego_device_type_id_t type_id = pbdrv_counter_ev3_get_type(adc);

        // Update number of consecutive identical detections.
        if (dev->last_type_id == type_id) {
            dev->type_id_count++;
        } else {
            dev->last_type_id = type_id;
            dev->type_id_count = 1;
        }

        // Update stable type if we have seen enough identical detections,
        // including none detections.
        if (dev->type_id_count >= PBDRV_COUNTER_EV3_TYPE_MIN_STABLE_COUNT) {
            dev->stable_type_id = type_id;
        }
    }
}

static pbio_os_process_t pbdrv_counter_device_detect_process;

/**
 * Background process to periodically read the ADC values to detect devices.
 */
static pbio_error_t pbdrv_counter_device_detect_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {
        pbdrv_counter_ev3_update_type();
        PBIO_OS_AWAIT_MS(state, &timer, PBDRV_COUNTER_EV3_TYPE_LOOP_TIME);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}


pbio_error_t pbdrv_counter_assert_type(pbdrv_counter_dev_t *dev, lego_device_type_id_t *expected_type_id) {

    if (dev->stable_type_id == LEGO_DEVICE_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }

    // Success if any encoded motor is allowed or if the detected type matches.
    if (*expected_type_id == LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR || *expected_type_id == dev->stable_type_id) {
        *expected_type_id = dev->stable_type_id;
        return PBIO_SUCCESS;
    }

    return PBIO_ERROR_NO_DEV;
}

pbio_error_t pbdrv_counter_get_angle(pbdrv_counter_dev_t *dev, int32_t *rotations, int32_t *millidegrees) {

    // Ensures this method raises on becoming disconnected.
    lego_device_type_id_t expected_type_id = LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR;
    pbio_error_t err = pbdrv_counter_assert_type(dev, &expected_type_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *millidegrees = (dev->count % 360) * 1000;
    *rotations = dev->count / 360;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_counter_get_abs_angle(pbdrv_counter_dev_t *dev, int32_t *millidegrees) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static void pbdrv_counter_ev3_irq_handler(uint32_t bank_id, uint32_t bank_int_id) {
    GPIOBankIntDisable(SOC_GPIO_0_REGS, bank_id);
    uint32_t status = HWREG(SOC_GPIO_0_REGS + GPIO_INTSTAT((bank_id / 2)));

    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(counters); i++) {
        pbdrv_counter_dev_t *dev = &counters[i];
        pbdrv_gpio_ev3_mux_t *mux = dev->gpio_int.bank;

        uint32_t mask = 1 << ((mux->gpio_bank_id * 16 + dev->gpio_int.pin) % 32);

        // IRQ is not for this motor.
        if (mux->gpio_bank_id != bank_id || !(status & mask)) {
            continue;
        }

        // Clear the interrupt and update the count.
        HWREG(SOC_GPIO_0_REGS + GPIO_INTSTAT((bank_id / 2))) = mask;
        if (pbdrv_gpio_input(&dev->gpio_int) ^ pbdrv_gpio_input(&dev->gpio_dir)) {
            dev->count++;
        } else {
            dev->count--;
        }
    }

    IntSystemStatusClear(bank_int_id);
    GPIOBankIntEnable(SOC_GPIO_0_REGS, bank_id);
}

static void pbdrv_counter_ev3_irq5(void) {
    pbdrv_counter_ev3_irq_handler(5, SYS_INT_GPIOB5);
}

static void pbdrv_counter_ev3_irq6(void) {
    pbdrv_counter_ev3_irq_handler(6, SYS_INT_GPIOB6);
}

void pbdrv_counter_init(void) {

    for (size_t i = 0; i < PBIO_ARRAY_SIZE(counters); i++) {
        pbdrv_counter_dev_t *dev = &counters[i];
        pbdrv_gpio_input(&dev->gpio_int);
        pbdrv_gpio_input(&dev->gpio_dir);
        pbdrv_gpio_out_low(&dev->gpio_det);
    }

    // AINTC for GPIO bank 5 and 6 interrupts
    IntRegister(SYS_INT_GPIOB5, pbdrv_counter_ev3_irq5);
    IntRegister(SYS_INT_GPIOB6, pbdrv_counter_ev3_irq6);
    IntChannelSet(SYS_INT_GPIOB5, 0);
    IntChannelSet(SYS_INT_GPIOB6, 0);
    IntSystemEnable(SYS_INT_GPIOB5);
    IntSystemEnable(SYS_INT_GPIOB6);
    // GPIO-Controller for GPIO bank 5 and 6 interrupts
    uint32_t baseAddr = SOC_GPIO_0_REGS;
    HWREG(baseAddr + GPIO_BINTEN) = HWREG(baseAddr + GPIO_BINTEN) | 0x00000060; // Enable Interrupt for bank 5 and 6 at the same time
    // For bank 5
    HWREG(baseAddr + GPIO_SET_RIS_TRIG(2)) = HWREG(baseAddr + GPIO_SET_RIS_TRIG(2)) | 0x29000000;
    HWREG(baseAddr + GPIO_SET_FAL_TRIG(2)) = HWREG(baseAddr + GPIO_SET_FAL_TRIG(2)) | 0x29000000;
    // For bank 6
    HWREG(baseAddr + GPIO_SET_RIS_TRIG(3)) = HWREG(baseAddr + GPIO_SET_RIS_TRIG(3)) | 0x00000200;
    HWREG(baseAddr + GPIO_SET_FAL_TRIG(3)) = HWREG(baseAddr + GPIO_SET_FAL_TRIG(3)) | 0x00000200;

    // Monitor attached counter devices
    pbio_os_process_start(&pbdrv_counter_device_detect_process, pbdrv_counter_device_detect_process_thread, NULL);
}

#endif // PBDRV_CONFIG_COUNTER_EV3
