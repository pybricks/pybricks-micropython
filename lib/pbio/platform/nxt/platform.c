// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2023 The Pybricks Authors
// Copyright (c) 2007,2008 the NxOS developers
// See AUTHORS for a full list of the developers.

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/reset.h>
#include <pbio/button.h>
#include <pbio/main.h>
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
#include <nxos/drivers/_usb.h>
#include <nxos/drivers/bt.h>
#include <nxos/drivers/i2c.h>
#include <nxos/drivers/systick.h>
#include <nxos/interrupts.h>

#include "../../drv/legodev/legodev_nxt.h"

const pbdrv_legodev_nxt_motor_platform_data_t pbdrv_legodev_nxt_motor_platform_data[PBDRV_CONFIG_LEGODEV_NXT_NUM_MOTOR] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .motor_driver_index = 0,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .motor_driver_index = 1,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .motor_driver_index = 2,
    },
};

const pbdrv_legodev_nxt_sensor_platform_data_t pbdrv_legodev_nxt_sensor_platform_data[PBDRV_CONFIG_LEGODEV_NXT_NUM_SENSOR] = {
    {
        .port_id = PBIO_PORT_ID_1,
    },
    {
        .port_id = PBIO_PORT_ID_2,
    },
    {
        .port_id = PBIO_PORT_ID_3,
    },
    {
        .port_id = PBIO_PORT_ID_4,
    },
};



// FIXME: Needs to use a process very similar to pbsys/bluetooth
static bool bluetooth_connect(void) {

    int port_handle = -1;
    int connection_handle = -1;

    nx_bt_init();

    char *name = "Pybricks NXT";
    char *pin = "1234";
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

    port_handle = nx_bt_open_port();
    (void)port_handle;

    while (!nx_bt_stream_opened()) {
        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
            return false;
        }

        if (nx_bt_has_dev_waiting_for_pin()) {
            nx_bt_send_pin(pin);
            nx_display_string("Please enter pin.\n");
        } else if (nx_bt_connection_pending()) {
            nx_display_string("Connecting ...\n");
            nx_bt_accept_connection(true);

            while ((connection_handle = nx_bt_connection_established()) < 0) {
                if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
                    return false;
                }

                pbio_do_one_event();
            }

            nx_bt_stream_open(connection_handle);
        }

        pbio_do_one_event();
    }

    nx_display_clear();
    nx_display_cursor_set_pos(0, 0);

    return true;
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
    // pbdrv_init_busy instead of busy waiting for 100ms.
    nx__avr_init();
    nx__motors_init();
    nx__lcd_init();
    nx__display_init();
    nx__sensors_init();
    nx__usb_init();
    nx_i2c_init();

    /* Delay a little post-init, to let all the drivers settle down. */
    nx_systick_wait_ms(100);
}

// For now, this file is the main entry point for NXT. Eventually, this
// can be dropped and we can use main() in pbsys/main.
// For now it enters the MicroPython REPL directly for convenient debugging.

/**
 * Initializes the PBIO library, runs custom main program, and handles shutdown.
 *
 * @param [in]  main    The main program.
 */
int main(int argc, char **argv) {

    pbio_init();
    pbsys_init();

    // Accept incoming serial connection and get ready to read first byte.
    if (bluetooth_connect()) {
        // Receive one character to get going...
        uint8_t flush_buf[1];
        nx_display_string("Press a key.\n");
        nx_bt_stream_read(flush_buf, sizeof(flush_buf));

        while (nx_bt_stream_data_read() != sizeof(flush_buf)) {
            if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
                goto out;
            }

            pbio_do_one_event();
        }

        nx_display_string("Connected. REPL.\n");
    out:;
    }

    // Keep loading and running user programs until shutdown is requested.
    while (!pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {

        // Receive a program. This cancels itself on shutdown.
        static pbsys_main_program_t program;
        #if 0
        pbio_error_t err = pbsys_program_load_wait_command(&program);
        if (err != PBIO_SUCCESS) {
            continue;
        }
        #endif

        static char heap[32 * 1024];
        program.run_builtin = true,
        program.code_end = heap,
        program.data_end = heap + sizeof(heap),

        // Prepare pbsys for running the program.
        pbsys_status_set(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
        // pbsys_bluetooth_rx_set_callback(pbsys_main_stdin_event);

        // Handle pending events triggered by the status change, such as
        // starting status light animation.
        while (pbio_do_one_event()) {
        }

        // Run the main application.
        pbsys_main_run_program(&program);

        // Get system back in idle state.
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
        // pbsys_bluetooth_rx_set_callback(NULL);
        pbsys_program_stop_set_buttons(PBIO_BUTTON_CENTER);
        pbio_stop_all(true);
    }

    // Stop system processes and save user data before we shutdown.
    pbsys_deinit();

    // Now lower-level processes may shutdown and/or power off.
    pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN);

    // The power could be held on due to someone pressing the center button
    // or USB being plugged in, so we have this loop to keep pumping events
    // to turn off most of the peripherals and keep the battery charger running.
    for (;;) {
        // We must handle all pending events before turning the power off the
        // first time, otherwise the city hub turns itself back on sometimes.
        while (pbio_do_one_event()) {
        }

        #if PBSYS_CONFIG_BATTERY_CHARGER
        // On hubs with USB battery chargers, we can't turn off power while
        // USB is connected, otherwise it disables the op-amp that provides
        // the battery voltage to the ADC.
        if (pbdrv_usb_get_bcd() != PBDRV_USB_BCD_NONE) {
            continue;
        }
        #endif

        pbdrv_reset_power_off();
    }
}
