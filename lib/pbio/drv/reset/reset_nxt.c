// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

// Manages power off and reset for LEGO MINDSTORMS NXT.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESET_NXT

#include <at91sam7s256.h>
#include <nxos/drivers/_avr.h>
#include <nxos/drivers/_lcd.h>
#include <nxos/drivers/_usb.h>
#include <nxos/drivers/bt.h>

#include <pbdrv/reset.h>

void pbdrv_reset_init(void) {
}

void pbdrv_reset(pbdrv_reset_action_t action) {
    // nxt_lcd_enable(0);
    for (;;) {
        switch (action) {
            case PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE:
                nx__avr_firmware_update_mode();
                break;
            // TODO: implement case PBDRV_RESET_ACTION_RESET
            default:
                break;
        }
    }
}

pbdrv_reset_reason_t pbdrv_reset_get_reason(void) {

    int reset_status = *AT91C_RSTC_RSR;
    reset_status &= AT91C_RSTC_RSTTYP;

    switch (reset_status)
    {
        case AT91C_RSTC_RSTTYP_POWERUP:
            // Power-up Reset. VDDCORE rising.
            return PBDRV_RESET_REASON_NONE;
        case AT91C_RSTC_RSTTYP_WAKEUP:
            //  WakeUp Reset. VDDCORE rising.
            return PBDRV_RESET_REASON_NONE;
        case AT91C_RSTC_RSTTYP_WATCHDOG:
            //  Watchdog Reset. Watchdog overflow occured.
            // FIXME: Watchdog reset is disabled on startup, so this doesn't work.
            return PBDRV_RESET_REASON_WATCHDOG;
        case AT91C_RSTC_RSTTYP_SOFTWARE:
            //  Software Reset. Processor reset required by the software.
            return PBDRV_RESET_REASON_SOFTWARE;
        case AT91C_RSTC_RSTTYP_USER:
            //  User Reset. NRST pin detected low.
            return PBDRV_RESET_REASON_SOFTWARE;
        case AT91C_RSTC_RSTTYP_BROWNOUT:
            //  (RSTC) Brownout Reset occured.
            return PBDRV_RESET_REASON_SOFTWARE;
        default:
            return PBDRV_RESET_REASON_NONE;
    }
}

// NOTE: This was nx_core_halt() in NxOS.
void pbdrv_reset_power_off(void) {
    if (nx_bt_stream_opened()) {
        nx_bt_stream_close();
    }

    nx__lcd_shutdown();
    nx__usb_disable();
    nx__avr_power_down();
}

#endif // PBDRV_CONFIG_RESET_NXT
