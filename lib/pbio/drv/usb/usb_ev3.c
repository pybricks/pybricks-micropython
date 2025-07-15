// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// EV3 / TI AM1808 / Mentor Graphics MUSBMHDRC driver
// implementing a bespoke USB stack for Pybricks USB protocol

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_EV3

#include <stdint.h>

#include <pbdrv/usb.h>
#include <pbio/os.h>
#include <pbio/util.h>

#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_usbOtg_AM1808.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_usbphyGS60.h>
#include <tiam1808/hw/hw_usb.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_psc_AM1808.h>
#include <tiam1808/psc.h>
#include <tiam1808/usb.h>

#include <pbdrv/clock.h>

#include "usb_ch9.h"

static void usb_device_intr(void) {
    IntSystemStatusClear(SYS_INT_USB0);
}

static pbio_os_process_t pbdrv_usb_ev3_process;

static pbio_error_t pbdrv_usb_ev3_process_thread(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_usb_init(void) {
    // This reset sequence is from Example 34-1 in the AM1808 TRM (spruh82c.pdf)
    // Because PHYs and clocking are... as they tend to be, use the precise sequence
    // of operations specified.

    // Power on and reset the controller
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_USB0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);
    USBReset(USB0_BASE);

    // Reset the PHY
    HWREG(CFGCHIP2_USBPHYCTRL) |= CFGCHIP2_RESET;
    for (int i = 0; i < 50; i++) {
        // Empty delay loop which should not be optimized out.
        // This is the delay amount in the TI datasheet example.
        __asm__ volatile ("");
    }
    HWREG(CFGCHIP2_USBPHYCTRL) &= ~CFGCHIP2_RESET;

    // Set up the PHY and force it into device mode
    HWREG(CFGCHIP2_USBPHYCTRL) =
        (HWREG(CFGCHIP2_USBPHYCTRL) &
            ~CFGCHIP2_OTGMODE &
            ~CFGCHIP2_PHYPWRDN &            // Make sure PHY is on
            ~CFGCHIP2_OTGPWRDN) |           // Make sure OTG subsystem is on
        CFGCHIP2_FORCE_DEVICE |             // We only ever want device operation
        CFGCHIP2_DATPOL |                   // Data lines are *not* inverted
        CFGCHIP2_SESENDEN |                 // Enable various analog comparators
        CFGCHIP2_VBDTCTEN;

    HWREG(CFGCHIP2_USBPHYCTRL) =
        (HWREG(CFGCHIP2_USBPHYCTRL) & ~CFGCHIP2_REFFREQ) |
        CFGCHIP2_REFFREQ_24MHZ |            // Clock is 24 MHz
        CFGCHIP2_USB2PHYCLKMUX;             // Clock comes from PLL

    // Wait for PHY clocks to be ready
    while (!(HWREG(CFGCHIP2_USBPHYCTRL) & CFGCHIP2_PHYCLKGD)) {
    }

    // Enable "PDR" mode for handling interrupts
    //
    // The datasheet doesn't clearly explain what this means,
    // but what it appears TI has done is to wrap some custom interrupt and DMA
    // logic around the Mentor Graphics core. The standard core registers
    // thus now live at offset +0x400, and addresses below that pertain to the wrapper.
    // This leaves some redundancy with how interrupts are set up, and this bit
    // seems to enable accessing everything the TI way (more convenient) rather than
    // the standard Mentor Graphics way (interrupt flags spread across more registers).
    HWREG(USB_0_OTGBASE + USB_0_CTRL) &= ~USBOTG_CTRL_UINT;
    HWREGH(USB0_BASE + USB_O_TXIE) = USB_TXIE_ALL_AM1808;
    HWREGH(USB0_BASE + USB_O_RXIE) = USB_RXIE_ALL_AM1808;
    HWREGB(USB0_BASE + USB_O_IE) = USB_IE_ALL;

    // Enable the interrupts we actually care about
    HWREG(USB_0_OTGBASE + USB_0_INTR_MASK_SET) =
        USBOTG_INTR_RESET |
        USBOTG_INTR_EP1_OUT |
        USBOTG_INTR_EP1_IN |
        USBOTG_INTR_EP0;

    // Clear all the interrupts once
    HWREG(USB_0_OTGBASE + USB_0_INTR_SRC_CLEAR) = HWREG(USB_0_OTGBASE + USB_0_INTR_SRC);

    // Hook up interrupt handler
    IntRegister(SYS_INT_USB0, usb_device_intr);
    IntChannelSet(SYS_INT_USB0, 2);
    IntSystemEnable(SYS_INT_USB0);

    // Finally signal a connection
    USBDevConnect(USB0_BASE);

    // We are basically done. USB is event-driven, and so we don't have to block boot.
    pbio_os_process_start(&pbdrv_usb_ev3_process, pbdrv_usb_ev3_process_thread, NULL);
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    // This function is not used on EV3
    return PBDRV_USB_BCD_NONE;
}

// TODO: Reimplement the following functions as appropriate
// (mphalport.c and the "host" layer are currently being refactored)
uint32_t pbdrv_usb_write(const uint8_t *data, uint32_t size) {
    // Return the size requested so that caller doesn't block.
    return size;
}

uint32_t pbdrv_usb_rx_data_available(void) {
    return 0;
}

int32_t pbdrv_usb_get_char(void) {
    return -1;
}

void pbdrv_usb_tx_flush(void) {
}

bool pbdrv_usb_connection_is_active(void) {
    return false;
}

#endif // PBDRV_CONFIG_USB_EV3
