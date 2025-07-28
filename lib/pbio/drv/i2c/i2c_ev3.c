// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// I2C driver for EV3

#include <pbdrv/config.h>

#if PBDRV_CONFIG_I2C_EV3

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <tiam1808/hw/hw_syscfg0_AM1808.h>

#include "../drv/gpio/gpio_ev3.h"

#include <pbdrv/gpio.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/util.h>

#include <pbdrv/gpio.h>
#include <pbdrv/i2c.h>
#include "i2c_ev3.h"

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

struct _pbdrv_i2c_dev_t {
    /** Platform-specific data */
    const pbdrv_i2c_ev3_platform_data_t *pdata;
    //
    // TODO: i2c state goes here.
    //
};

static pbdrv_i2c_dev_t i2c_devs[PBDRV_CONFIG_I2C_EV3_NUM_DEV];

pbio_error_t pbdrv_i2c_get_instance(uint8_t id, pbdrv_i2c_dev_t **i2c_dev) {
    if (id >= PBDRV_CONFIG_I2C_EV3_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }
    pbdrv_i2c_dev_t *dev = &i2c_devs[id];
    if (!dev->pdata) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }
    *i2c_dev = dev;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_i2c_placeholder_operation(pbdrv_i2c_dev_t *i2c_dev, const char *operation) {
    debug_pr("I2C placeholder operation %s\n", operation);
    return PBIO_SUCCESS;
}

static pbio_os_process_t ev3_i2c_wip_process;

static pbdrv_gpio_t test_scl = PBDRV_GPIO_EV3_PIN(0, 15, 12, 0, 12);
static pbdrv_gpio_t test_sda = PBDRV_GPIO_EV3_PIN(2, 7, 4, 1, 14);

static inline void delaydelay() {
    for (int i = 0; i < 100; i++) __asm__ volatile("");
}

pbio_error_t ev3_i2c_wip_process_thread(pbio_os_state_t *state, void *context) {
    static pbio_os_timer_t timer;
    static int bit;
    static uint8_t byte;

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_gpio_input(&test_scl);
    pbdrv_gpio_input(&test_sda);

    PBIO_OS_AWAIT_MS(state, &timer, 1000);
    debug_pr("i2c test C%d D%d\r\n", pbdrv_gpio_input(&test_scl), pbdrv_gpio_input(&test_sda));

    pbdrv_gpio_out_low(&test_sda);          // S
    delaydelay();

    byte = 0x02;
    bit = 8;

    while (bit-- > 0) {
        pbdrv_gpio_out_low(&test_scl);
        delaydelay();
        if (byte & (1 << bit)) {
            pbdrv_gpio_input(&test_sda);
        } else {
            pbdrv_gpio_out_low(&test_sda);
        }
        delaydelay();
        pbdrv_gpio_input(&test_scl);
        delaydelay();
    }

    pbdrv_gpio_out_low(&test_scl);          // ACK
    delaydelay();
    pbdrv_gpio_input(&test_sda);
    delaydelay();
    debug_pr("i2c ack D%d\r\n", pbdrv_gpio_input(&test_sda));
    pbdrv_gpio_input(&test_scl);
    delaydelay();

    byte = 0x00;
    bit = 8;

    while (bit-- > 0) {
        pbdrv_gpio_out_low(&test_scl);
        delaydelay();
        if (byte & (1 << bit)) {
            pbdrv_gpio_input(&test_sda);
        } else {
            pbdrv_gpio_out_low(&test_sda);
        }
        delaydelay();
        pbdrv_gpio_input(&test_scl);
        delaydelay();
    }

    pbdrv_gpio_out_low(&test_scl);          // ACK
    delaydelay();
    pbdrv_gpio_input(&test_sda);
    delaydelay();
    debug_pr("i2c ack D%d\r\n", pbdrv_gpio_input(&test_sda));
    pbdrv_gpio_input(&test_scl);
    delaydelay();

    pbdrv_gpio_out_low(&test_scl);          // P
    delaydelay();
    pbdrv_gpio_out_low(&test_sda);
    delaydelay();
    pbdrv_gpio_input(&test_scl);
    delaydelay();
    pbdrv_gpio_input(&test_sda);
    delaydelay();

    debug_pr("i2c test 2 C%d D%d\r\n", pbdrv_gpio_input(&test_scl), pbdrv_gpio_input(&test_sda));

    pbdrv_gpio_out_low(&test_scl);          // NXT wiggle
    delaydelay();
    pbdrv_gpio_input(&test_scl);
    delaydelay();

    debug_pr("i2c test 3 C%d D%d\r\n", pbdrv_gpio_input(&test_scl), pbdrv_gpio_input(&test_sda));

    pbdrv_gpio_out_low(&test_sda);          // S
    delaydelay();

    byte = 0x03;
    bit = 8;

    while (bit-- > 0) {
        // debug_pr("byte %02x bit %d\r\n", byte, bit);
        pbdrv_gpio_out_low(&test_scl);
        delaydelay();
        if (byte & (1 << bit)) {
            pbdrv_gpio_input(&test_sda);
        } else {
            pbdrv_gpio_out_low(&test_sda);
        }
        delaydelay();
        pbdrv_gpio_input(&test_scl);
        delaydelay();
    }

    pbdrv_gpio_out_low(&test_scl);          // ACK
    delaydelay();
    pbdrv_gpio_input(&test_sda);
    delaydelay();
    debug_pr("i2c ack D%d\r\n", pbdrv_gpio_input(&test_sda));
    pbdrv_gpio_input(&test_scl);
    delaydelay();

    byte = 0;
    bit = 8;
    while (bit-- > 0) {
        pbdrv_gpio_out_low(&test_scl);
        pbdrv_gpio_input(&test_sda);
        delaydelay();
        pbdrv_gpio_input(&test_scl);
        if (pbdrv_gpio_input(&test_sda)) {
            byte |= (1 << bit);
        }
        delaydelay();
    }

    pbdrv_gpio_out_low(&test_scl);          // ACK send
    delaydelay();
    pbdrv_gpio_out_low(&test_sda);
    delaydelay();
    pbdrv_gpio_input(&test_scl);
    delaydelay();

    debug_pr("i2c get %02x\r\n", byte);

    byte = 0;
    bit = 8;
    while (bit-- > 0) {
        pbdrv_gpio_out_low(&test_scl);
        pbdrv_gpio_input(&test_sda);
        delaydelay();
        pbdrv_gpio_input(&test_scl);
        if (pbdrv_gpio_input(&test_sda)) {
            byte |= (1 << bit);
        }
        delaydelay();
    }

    pbdrv_gpio_out_low(&test_scl);          // ACK send
    delaydelay();
    pbdrv_gpio_out_low(&test_sda);
    delaydelay();
    pbdrv_gpio_input(&test_scl);
    delaydelay();

    debug_pr("i2c get %02x\r\n", byte);

    byte = 0;
    bit = 8;
    while (bit-- > 0) {
        pbdrv_gpio_out_low(&test_scl);
        pbdrv_gpio_input(&test_sda);
        delaydelay();
        pbdrv_gpio_input(&test_scl);
        if (pbdrv_gpio_input(&test_sda)) {
            byte |= (1 << bit);
        }
        delaydelay();
    }

    pbdrv_gpio_out_low(&test_scl);          // ACK send
    delaydelay();
    pbdrv_gpio_out_low(&test_sda);
    delaydelay();
    pbdrv_gpio_input(&test_scl);
    delaydelay();

    debug_pr("i2c get %02x\r\n", byte);

    byte = 0;
    bit = 8;
    while (bit-- > 0) {
        pbdrv_gpio_out_low(&test_scl);
        pbdrv_gpio_input(&test_sda);
        delaydelay();
        pbdrv_gpio_input(&test_scl);
        if (pbdrv_gpio_input(&test_sda)) {
            byte |= (1 << bit);
        }
        delaydelay();
    }

    pbdrv_gpio_out_low(&test_scl);          // ACK send
    delaydelay();
    pbdrv_gpio_out_low(&test_sda);
    delaydelay();
    pbdrv_gpio_input(&test_scl);
    delaydelay();

    debug_pr("i2c get %02x\r\n", byte);

    byte = 0;
    bit = 8;
    while (bit-- > 0) {
        pbdrv_gpio_out_low(&test_scl);
        pbdrv_gpio_input(&test_sda);
        delaydelay();
        pbdrv_gpio_input(&test_scl);
        if (pbdrv_gpio_input(&test_sda)) {
            byte |= (1 << bit);
        }
        delaydelay();
    }

    pbdrv_gpio_out_low(&test_scl);          // ACK send
    delaydelay();
    // pbdrv_gpio_out_low(&test_sda);
    pbdrv_gpio_input(&test_sda);
    delaydelay();
    // debug_pr("i2c last ack %d\r\n", pbdrv_gpio_input(&test_sda));
    pbdrv_gpio_input(&test_scl);
    delaydelay();

    debug_pr("i2c get %02x\r\n", byte);

    pbdrv_gpio_out_low(&test_scl);          // P
    delaydelay();
    pbdrv_gpio_out_low(&test_sda);
    delaydelay();
    pbdrv_gpio_input(&test_scl);
    delaydelay();
    pbdrv_gpio_input(&test_sda);
    delaydelay();

    debug_pr("i2c test end C%d D%d\r\n", pbdrv_gpio_input(&test_scl), pbdrv_gpio_input(&test_sda));

    PBIO_OS_AWAIT_MS(state, &timer, 100);

    debug_pr("i2c test end 2 C%d D%d\r\n", pbdrv_gpio_input(&test_scl), pbdrv_gpio_input(&test_sda));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_i2c_init(void) {
    // for (int i = 0; i < PBDRV_CONFIG_I2C_EV3_NUM_DEV; i++) {
    //     const pbdrv_i2c_ev3_platform_data_t *pdata = &pbdrv_i2c_ev3_platform_data[i];
    //     pbdrv_i2c_dev_t *i2c = &i2c_devs[i];
    //     i2c->pdata = pdata;
    // }

    pbio_os_process_start(&ev3_i2c_wip_process, ev3_i2c_wip_process_thread, NULL);
}

#endif // PBDRV_CONFIG_I2C_EV3
