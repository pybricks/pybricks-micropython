// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2023 The Pybricks Authors
// Copyright (c) 2007,2008 the NxOS developers
// See AUTHORS for a full list of the developers.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/config.h>
#include <pbdrv/ioport.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbio/os.h>
#include <pbio/port_interface.h>

#include <pbsys/core.h>
#include <pbsys/main.h>
#include <pbsys/program_stop.h>
#include <pbsys/status.h>


#include <nxos/_display.h>
#include <nxos/assert.h>
#include <nxos/drivers/_aic.h>
#include <nxos/drivers/_avr.h>
#include <nxos/drivers/_lcd.h>
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
        .motor_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE, // todo, power pin, see ev3
        .external_port_index = 0,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .i2c_driver_index = 0,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_NONE, // todo, lego mode, nxt dcm
    },
    {
        .port_id = PBIO_PORT_ID_2,
        .motor_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE, // todo, power pin, see ev3
        .external_port_index = 1,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .i2c_driver_index = 1,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_NONE, // todo, lego mode, nxt dcm
    },
    {
        .port_id = PBIO_PORT_ID_3,
        .motor_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE, // todo, power pin, see ev3
        .external_port_index = 2,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .i2c_driver_index = 2,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_NONE, // todo, lego mode, nxt dcm
    },
    {
        .port_id = PBIO_PORT_ID_4,
        .motor_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE, // todo, power pin, see ev3
        .external_port_index = 3,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .i2c_driver_index = 3,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_NONE, // todo, lego mode, nxt dcm
    },
};

const char *pin = "1234";

static void legacy_bluetooth_init_blocking(void) {
    nx_bt_init();

    char *name = "Pybricks NXT";
    nx_bt_set_friendly_name(name);

    nx_display_string("Bluetooth name:\n");
    nx_display_string(name);
    nx_display_string("\n");
    uint8_t local_addr[7];
    if (nx_bt_get_local_addr(local_addr)) {
        for (int i = 0; i < 6; i++) {
            nx_display_hex(local_addr[i]);
            nx_display_string(i < 5 ? ":": "\n");
        }
    }
    nx_display_string("Pin: ");
    nx_display_string(pin);
    nx_display_string("\n\nConnect to me as BT serial port.\n");

    nx_bt_set_discoverable(true);

    nx_bt_open_port();
}

// REVISIT: This process waits for the user to connect to the NXT brick with
// Bluetooth classic (RFCOMM). This allows basic I/O until proper Pybricks USB
// or Bluetooth classic solutions are implemented. Then this process will be
// removed.
static pbio_os_process_t legacy_bluetooth_connect_process;

static pbio_error_t legacy_bluetooth_connect_process_thread(pbio_os_state_t *state, void *context) {

    PBIO_OS_ASYNC_BEGIN(state);

    static pbio_os_timer_t timer;

    static int connection_handle = -1;

    while (!nx_bt_stream_opened()) {

        if (nx_bt_has_dev_waiting_for_pin()) {
            nx_bt_send_pin((char *)pin);
            nx_display_string("Please enter pin.\n");
        } else if (nx_bt_connection_pending()) {
            nx_display_string("Connecting ...\n");
            nx_bt_accept_connection(true);

            while ((connection_handle = nx_bt_connection_established()) < 0) {
                PBIO_OS_AWAIT_MS(state, &timer, 2);
            }

            nx_bt_stream_open(connection_handle);
        }

        PBIO_OS_AWAIT_MS(state, &timer, 100);
    }

    nx_display_clear();
    nx_display_cursor_set_pos(0, 0);

    nx_display_string("RFCOMM ready.\n");

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

bool nx_bt_is_ready(void) {
    return legacy_bluetooth_connect_process.err == PBIO_SUCCESS;
}

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

    // TODO: we should be able to convert these to generic pbio drivers and use
    // pbio_busy_count_busy instead of busy waiting for 100ms.
    nx__avr_init();
    nx__motors_init();
    nx__lcd_init();
    nx__display_init();
    nx__sensors_init();
    nx_i2c_init();

    /* Delay a little post-init, to let all the drivers settle down. */
    nx_systick_wait_ms(100);

    // Blocking Bluetooth setup, then await user connection without blocking,
    // allowing pbio processes to start even if nothing is connected.
    legacy_bluetooth_init_blocking();
    pbio_os_process_start(&legacy_bluetooth_connect_process, legacy_bluetooth_connect_process_thread, NULL);

}
