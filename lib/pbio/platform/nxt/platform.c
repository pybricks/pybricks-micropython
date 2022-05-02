/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/interrupts.h"
#include "base/_display.h"
#include "base/assert.h"
#include "base/drivers/_aic.h"
#include "base/drivers/_systick.h"
#include "base/drivers/_sound.h"
#include "base/drivers/_avr.h"
#include "base/drivers/_motors.h"
#include "base/drivers/_lcd.h"
#include "base/drivers/_sensors.h"
#include "base/drivers/_usb.h"
#include "base/drivers/i2c.h"
#include "base/drivers/bt.h"

#include "base/_core.h"

static void core_init(void) {
    nx__aic_init();
    nx_interrupts_enable();
    nx__systick_init();
    nx__sound_init();
    nx__avr_init();
    nx__motors_init();
    nx__lcd_init();
    nx__display_init();
    nx__sensors_init();
    nx__usb_init();
    nx_i2c_init(); // TODO: should be nx__i2c_init().

    /* Delay a little post-init, to let all the drivers settle down. */
    nx_systick_wait_ms(100);
}

void nx_core_halt(void) {
    if (nx_bt_stream_opened()) {
        nx_bt_stream_close();
    }
    nx__lcd_shutdown();
    nx__usb_disable();
    nx__avr_power_down();
}

void pbdrv_reset_power_off(void) {
    nx_core_halt();
}

// FIXME: Needs to use a process very similar to pbsys/bluetooth
static void bluetooth_connect(void) {

    int port_handle = -1;
    int connection_handle = -1;

    nx_bt_init();

    char *name = "Pybricks NXT";
    char *pin = "1234";
    nx_bt_set_friendly_name(name);

    nx_display_string("Bluetooth name:\n");
    nx_display_string(name);
    nx_display_string("\n");
    U8 local_addr[7];
    if (nx_bt_get_local_addr(local_addr)) {
        for (int i = 0; i < 6; i++) {
            nx_display_hex(local_addr[i]);
            nx_display_string(i < 5 ? ":": "\n");
        }
    }
    nx_display_string("Pin: ");
    nx_display_string(pin);
    nx_display_string("\n\nConnect to me as BT serial port.\n");

    nx_bt_set_discoverable(TRUE);

    port_handle = nx_bt_open_port();
    (void)port_handle;

    while (!nx_bt_stream_opened()) {
        if (nx_bt_has_dev_waiting_for_pin()) {
            nx_bt_send_pin(pin);
            nx_display_string("Please enter pin.\n");
        } else if (nx_bt_connection_pending()) {
            nx_display_string("Connecting ...\n");
            nx_bt_accept_connection(TRUE);
            while ((connection_handle = nx_bt_connection_established()) < 0) {
                nx_systick_wait_ms(100);
            }
            nx_bt_stream_open(connection_handle);
        }
        nx_systick_wait_ms(100);
    }
    nx_display_clear();
    nx_display_cursor_set_pos(0, 0);
}

extern int main(int argc, char **argv);

U8 rx_char = 0x20;


void nx__kernel_main(void) {
    // Start the system.
    core_init();

    // Accept incoming serial connection and get ready to read first byte.
    bluetooth_connect();
    nx_bt_stream_read(&rx_char, 1);

    // Start Pybricks.
    main(0, NULL);

    nx_core_halt();
}
