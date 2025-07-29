// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// I2C driver for EV3

#include <pbdrv/config.h>

#if PBDRV_CONFIG_I2C_EV3

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/timer.h>

#include "../drv/gpio/gpio_ev3.h"

#include <pbdrv/compiler.h>
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

extern void panic_puts(const char *c);
extern void panic_putu8(uint8_t x);

uint8_t i2c_wip_buf[16];

pbio_error_t ev3_i2c_wip_process_thread(pbio_os_state_t *state, void *context) {
    static pbio_os_timer_t timer;
    static int i;
    // static int bit;
    // static uint8_t byte;

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_gpio_out_low(&test_scl);
    pbdrv_gpio_input(&test_scl);
    pbdrv_gpio_out_low(&test_sda);
    pbdrv_gpio_input(&test_sda);

    PBIO_OS_AWAIT_MS(state, &timer, 100);
    debug_pr("i2c test start C%d D%d\r\n", pbdrv_gpio_input(&test_scl), pbdrv_gpio_input(&test_sda));

    i2c_wip_buf[0] = 0x10;
    i2c_wip_buf[1] = 0xaa;
    i2c_wip_buf[2] = 0x55;
    i2c_wip_buf[3] = 0x12;
    i2c_wip_buf[4] = 0x34;
    pbdrv_compiler_memory_barrier();
    *(volatile uint32_t *)(0x80010008) = (uint32_t)i2c_wip_buf;
    *(volatile uint32_t *)(0x80010004) = 0x17000501;
    // *(volatile uint32_t *)(0x80010004) = 0x01060103;

    pbio_os_timer_set(&timer, 1000);
    PBIO_OS_AWAIT_UNTIL(state, *(volatile uint8_t *)(0x80010004) & 0x80 || pbio_os_timer_is_expired(&timer));
    debug_pr("i2c test done\r\n");
    debug_pr("i2c flags %08x\r\n", *(volatile uint32_t *)(0x80010004));
    debug_pr("i2c max time %d\r\n", *(volatile uint32_t *)(0x8001000c));

    debug_pr("i2c test end C%d D%d\r\n", pbdrv_gpio_input(&test_scl), pbdrv_gpio_input(&test_sda));

    PBIO_OS_AWAIT_MS(state, &timer, 100);

    debug_pr("i2c test end 2 C%d D%d\r\n", pbdrv_gpio_input(&test_scl), pbdrv_gpio_input(&test_sda));

    // NEW
    i2c_wip_buf[0] = 0x0e;
    pbdrv_compiler_memory_barrier();
    *(volatile uint32_t *)(0x80010004) = 0x17060101;

    pbio_os_timer_set(&timer, 1000);
    PBIO_OS_AWAIT_UNTIL(state, *(volatile uint8_t *)(0x80010004) & 0x80 || pbio_os_timer_is_expired(&timer));
    debug_pr("i2c #2 test done\r\n");
    debug_pr("i2c #2 flags %08x\r\n", *(volatile uint32_t *)(0x80010004));
    debug_pr("i2c #2 max time %d\r\n", *(volatile uint32_t *)(0x8001000c));

    PBIO_OS_AWAIT_MS(state, &timer, 100);

    for (i = 0; i < 512; i++) {
        panic_putu8(*(volatile uint8_t *)(0x01C32000 + i));
        if (i % 16 == 15) {
            panic_puts("\r\n");
        }
    }

    for (i = 0; i < 6; i++) {
        debug_pr("i2c get %02x\r\n", i2c_wip_buf[1 + i]);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_i2c_init(void) {
    // for (int i = 0; i < PBDRV_CONFIG_I2C_EV3_NUM_DEV; i++) {
    //     const pbdrv_i2c_ev3_platform_data_t *pdata = &pbdrv_i2c_ev3_platform_data[i];
    //     pbdrv_i2c_dev_t *i2c = &i2c_devs[i];
    //     i2c->pdata = pdata;
    // }

    TimerConfigure(SOC_TMR_2_REGS, TMR_CFG_32BIT_UNCH_CLK_BOTH_INT);
    TimerPeriodSet(SOC_TMR_2_REGS, TMR_TIMER12, 150000000 / 20000 - 1);
    TimerEnable(SOC_TMR_2_REGS, TMR_TIMER12, TMR_ENABLE_CONT);

    pbio_os_process_start(&ev3_i2c_wip_process, ev3_i2c_wip_process_thread, NULL);
}

#endif // PBDRV_CONFIG_I2C_EV3
