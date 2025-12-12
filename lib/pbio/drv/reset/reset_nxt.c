// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

// Manages power off and reset for LEGO MINDSTORMS NXT.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESET_NXT

#include <at91sam7s256.h>
#include <nxos/drivers/_lcd.h>
#include <nxos/drivers/bt.h>

#include <pbdrv/reset.h>

#include "../usb/usb_nxt.h"
#include "../rproc/rproc_nxt.h"

void pbdrv_reset_init(void) {
}

void pbdrv_reset(pbdrv_reset_action_t action) {
    if (action == PBDRV_RESET_ACTION_RESET) {
        AT91C_BASE_RSTC->RSTC_RCR = (0xA5 << 24) | AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | AT91C_RSTC_EXTRST;
        for (;;) {
        }
    } else {
        // Use the AVR coprocessor to perform power off or reset in update mode.
        // This does not return.
        pbdrv_rproc_nxt_reset_host(action);
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

void pbdrv_reset_power_off(void) {
    pbdrv_reset(PBDRV_RESET_ACTION_POWER_OFF);
}

#endif // PBDRV_CONFIG_RESET_NXT
