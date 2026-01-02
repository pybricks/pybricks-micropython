// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2023 The Pybricks Authors
// Copyright (c) 2007,2008 the NxOS developers
// See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/config.h>
#include <pbdrv/ioport.h>

#include <umm_malloc.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbio/os.h>
#include <pbio/port_interface.h>

#include <pbsys/core.h>
#include <pbsys/main.h>
#include <pbsys/program_stop.h>
#include <pbsys/status.h>


#include <nxos/assert.h>
#include <nxos/drivers/_aic.h>
#include <nxos/drivers/_motors.h>
#include <nxos/drivers/_sensors.h>
#include <nxos/drivers/bt.h>
#include <nxos/drivers/i2c.h>
#include <nxos/drivers/systick.h>
#include <nxos/interrupts.h>

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
        .port_id = PBIO_PORT_ID_1,
        .motor_driver_index = 3,
        .i2c_driver_index = 0,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .external_port_index = 0,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = {.pin = 23},
            .p6 = {.pin = 18},
            .adc_p1 = 0,
            .adc_p6 = 4,
        },
        .supported_modes = PBIO_PORT_MODE_I2C | PBIO_PORT_MODE_GPIO_ADC | PBIO_PORT_MODE_LEGO_DCM,
    },
    {
        .port_id = PBIO_PORT_ID_2,
        .motor_driver_index = 4,
        .i2c_driver_index = 1,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .external_port_index = 1,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = {.pin = 28},
            .p6 = {.pin = 19},
            .adc_p1 = 1,
            .adc_p6 = 5,
        },
        .supported_modes = PBIO_PORT_MODE_I2C | PBIO_PORT_MODE_GPIO_ADC | PBIO_PORT_MODE_LEGO_DCM,
    },
    {
        .port_id = PBIO_PORT_ID_3,
        .motor_driver_index = 5,
        .i2c_driver_index = 2,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .external_port_index = 2,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = {.pin = 29},
            .p6 = {.pin = 20},
            .adc_p1 = 2,
            .adc_p6 = 6,
        },
        .supported_modes = PBIO_PORT_MODE_I2C | PBIO_PORT_MODE_GPIO_ADC | PBIO_PORT_MODE_LEGO_DCM,
    },
    {
        .port_id = PBIO_PORT_ID_4,
        .motor_driver_index = 6,
        .i2c_driver_index = 3,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .external_port_index = 3,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = {.pin = 30},
            .p6 = {.pin = 2},
            .adc_p1 = 3,
            .adc_p6 = 7,
        },
        .supported_modes = PBIO_PORT_MODE_I2C | PBIO_PORT_MODE_GPIO_ADC | PBIO_PORT_MODE_LEGO_DCM,
    },
};

char bluetooth_address_string[6 * 3]; // 6 hex bytes separated by ':' and ending in 0.

// Called from assembly code in startup.S
void SystemInit(void) {
    nx__aic_init();
    // TODO: can probably move nx_interrupts_enable() to pbdrv/core.c under
    // PBDRV_CONFIG_INIT_ENABLE_INTERRUPTS_ARM after nx_systick_wait_ms()
    // is removed
    nx_interrupts_enable(0);

    // Clock init must be first, since almost everything depends on clocks.
    // This probably should be moved here instead of in pbdrv_clock_init, just
    // as we do on other platforms.
    extern void pbdrv_clock_init(void);
    pbdrv_clock_init();

    // Initialize sensor GPIO.
    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(pbdrv_ioport_platform_data); i++) {
        const pbdrv_ioport_pins_t *pins = pbdrv_ioport_platform_data[i].pins;
        pbdrv_gpio_alt(&pins->p5, false);
        pbdrv_gpio_input(&pins->p5);
        pbdrv_gpio_alt(&pins->p6, false);
        pbdrv_gpio_input(&pins->p6);
    }

    // TODO: we should be able to convert these to generic pbio drivers and use
    // pbio_busy_count_busy instead of busy waiting for 100ms.
    nx__motors_init();
    // nx__sensors_init();
    // nx_i2c_init();

    /* Delay a little post-init, to let all the drivers settle down. */
    nx_systick_wait_ms(100);

    // Get Bluetooth address for use as unique USB serial number.
    nx_bt_init();
    nx_bt_set_friendly_name("Pybricks NXT");
    uint8_t local_addr[7];
    if (nx_bt_get_local_addr(local_addr)) {
        snprintf(bluetooth_address_string, sizeof(bluetooth_address_string),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            local_addr[0], local_addr[1], local_addr[2],
            local_addr[3], local_addr[4], local_addr[5]);
    }

    // Separate heap for large allocations - defined in linker script.
    extern char pb_umm_heap_start;
    extern char pb_umm_heap_end;
    umm_init_heap(&pb_umm_heap_start, &pb_umm_heap_end - &pb_umm_heap_start);
}
