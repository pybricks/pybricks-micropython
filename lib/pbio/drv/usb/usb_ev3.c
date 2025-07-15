// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// EV3 / TI AM1808 / Mentor Graphics MUSBMHDRC driver
// implementing a bespoke USB stack for Pybricks USB protocol

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_EV3

#include <stdint.h>

#include <pbdrv/usb.h>
#include <pbio/os.h>
#include <pbio/protocol.h>
#include <pbio/util.h>

#include <lego/usb.h>
#include "pbdrvconfig.h"

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

// Maximum packet sizes for the USB pipes
#define EP0_BUF_SZ              64
#define PYBRICKS_EP_PKT_SZ_FS   64
#define PYBRICKS_EP_PKT_SZ_HS   512

/**
 * Indices for string descriptors
 */
enum {
    STRING_DESC_LANGID,
    STRING_DESC_MFG,
    STRING_DESC_PRODUCT,
    STRING_DESC_SERIAL,
};

// Begin hardcoded USB descriptors

static const pbdrv_usb_dev_desc_union_t dev_desc = {
    .s = {
        .bLength = sizeof(pbdrv_usb_dev_desc_t),
        .bDescriptorType = DESC_TYPE_DEVICE,
        // A BOS descriptor is needed for Windows driver auto-installation,
        // so this must be at least 2.1
        .bcdUSB = 0x0210,
        .bDeviceClass = PBIO_PYBRICKS_USB_DEVICE_CLASS,
        .bDeviceSubClass = PBIO_PYBRICKS_USB_DEVICE_SUBCLASS,
        .bDeviceProtocol = PBIO_PYBRICKS_USB_DEVICE_PROTOCOL,
        .bMaxPacketSize0 = EP0_BUF_SZ,
        .idVendor = PBDRV_CONFIG_USB_VID,
        .idProduct = PBDRV_CONFIG_USB_PID,
        .bcdDevice = 0x0200,
        .iManufacturer = STRING_DESC_MFG,
        .iProduct = STRING_DESC_PRODUCT,
        .iSerialNumber = STRING_DESC_SERIAL,
        .bNumConfigurations = 1,
    }
};
static const pbdrv_usb_dev_qualifier_desc_union_t dev_qualifier_desc = {
    .s = {
        .bLength = sizeof(pbdrv_usb_dev_qualifier_desc_t),
        .bDescriptorType = DESC_TYPE_DEVICE_QUALIFIER,
        .bcdUSB = 0x0210,
        .bDeviceClass = PBIO_PYBRICKS_USB_DEVICE_CLASS,
        .bDeviceSubClass = PBIO_PYBRICKS_USB_DEVICE_SUBCLASS,
        .bDeviceProtocol = PBIO_PYBRICKS_USB_DEVICE_PROTOCOL,
        .bMaxPacketSize0 = EP0_BUF_SZ,
        .bNumConfigurations = 1,
        .bReserved = 0,
    }
};

typedef struct PBDRV_PACKED {
    pbdrv_usb_conf_desc_t conf_desc;
    pbdrv_usb_iface_desc_t iface_desc;
    pbdrv_usb_ep_desc_t ep_1_out;
    pbdrv_usb_ep_desc_t ep_1_in;
} pbdrv_usb_ev3_conf_1_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_ev3_conf_1);

static const pbdrv_usb_ev3_conf_1_union_t configuration_1_desc_hs = {
    .s = {
        .conf_desc = {
            .bLength = sizeof(pbdrv_usb_conf_desc_t),
            .bDescriptorType = DESC_TYPE_CONFIGURATION,
            .wTotalLength = sizeof(pbdrv_usb_ev3_conf_1_t),
            .bNumInterfaces = 1,
            .bConfigurationValue = 1,
            .iConfiguration = 0,
            .bmAttributes = USB_CONF_DESC_BM_ATTR_MUST_BE_SET | USB_CONF_DESC_BM_ATTR_SELF_POWERED,
            .bMaxPower = 0,
        },
        .iface_desc = {
            .bLength = sizeof(pbdrv_usb_iface_desc_t),
            .bDescriptorType = DESC_TYPE_INTERFACE,
            .bInterfaceNumber = 0,
            .bAlternateSetting = 0,
            .bNumEndpoints = 2,
            .bInterfaceClass = PBIO_PYBRICKS_USB_DEVICE_CLASS,
            .bInterfaceSubClass = PBIO_PYBRICKS_USB_DEVICE_SUBCLASS,
            .bInterfaceProtocol = PBIO_PYBRICKS_USB_DEVICE_PROTOCOL,
            .iInterface = 0,
        },
        .ep_1_out = {
            .bLength = sizeof(pbdrv_usb_ep_desc_t),
            .bDescriptorType = DESC_TYPE_ENDPOINT,
            .bEndpointAddress = 0x01,
            .bmAttributes = EP_TYPE_BULK,
            .wMaxPacketSize = PYBRICKS_EP_PKT_SZ_HS,
            .bInterval = 0,
        },
        .ep_1_in = {
            .bLength = sizeof(pbdrv_usb_ep_desc_t),
            .bDescriptorType = DESC_TYPE_ENDPOINT,
            .bEndpointAddress = 0x81,
            .bmAttributes = EP_TYPE_BULK,
            .wMaxPacketSize = PYBRICKS_EP_PKT_SZ_HS,
            .bInterval = 0,
        },
    }
};

static const pbdrv_usb_ev3_conf_1_union_t configuration_1_desc_fs = {
    .s = {
        .conf_desc = {
            .bLength = sizeof(pbdrv_usb_conf_desc_t),
            .bDescriptorType = DESC_TYPE_CONFIGURATION,
            .wTotalLength = sizeof(pbdrv_usb_ev3_conf_1_t),
            .bNumInterfaces = 1,
            .bConfigurationValue = 1,
            .iConfiguration = 0,
            .bmAttributes = USB_CONF_DESC_BM_ATTR_MUST_BE_SET | USB_CONF_DESC_BM_ATTR_SELF_POWERED,
            .bMaxPower = 0,
        },
        .iface_desc = {
            .bLength = sizeof(pbdrv_usb_iface_desc_t),
            .bDescriptorType = DESC_TYPE_INTERFACE,
            .bInterfaceNumber = 0,
            .bAlternateSetting = 0,
            .bNumEndpoints = 2,
            .bInterfaceClass = PBIO_PYBRICKS_USB_DEVICE_CLASS,
            .bInterfaceSubClass = PBIO_PYBRICKS_USB_DEVICE_SUBCLASS,
            .bInterfaceProtocol = PBIO_PYBRICKS_USB_DEVICE_PROTOCOL,
            .iInterface = 0,
        },
        .ep_1_out = {
            .bLength = sizeof(pbdrv_usb_ep_desc_t),
            .bDescriptorType = DESC_TYPE_ENDPOINT,
            .bEndpointAddress = 0x01,
            .bmAttributes = EP_TYPE_BULK,
            .wMaxPacketSize = PYBRICKS_EP_PKT_SZ_FS,
            .bInterval = 0,
        },
        .ep_1_in = {
            .bLength = sizeof(pbdrv_usb_ep_desc_t),
            .bDescriptorType = DESC_TYPE_ENDPOINT,
            .bEndpointAddress = 0x81,
            .bmAttributes = EP_TYPE_BULK,
            .wMaxPacketSize = PYBRICKS_EP_PKT_SZ_FS,
            .bInterval = 0,
        },
    }
};

typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t langID[1];
} pbdrv_usb_langid_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_langid);

pbdrv_usb_langid_union_t usb_str_desc_langid = {
    .s = {
        .bLength = 4,
        .bDescriptorType = DESC_TYPE_STRING,
        .langID = {0x0409},     // English (United States)
    }
};

// We generate string descriptors at runtime, so this dynamic buffer is needed
#define STRING_DESC_MAX_SZ      64
static union {
    uint8_t b[STRING_DESC_MAX_SZ];
    uint32_t u[STRING_DESC_MAX_SZ / 4];
} usb_string_desc_buffer;

// Defined in pbio/platform/ev3/platform.c
extern uint8_t pbdrv_ev3_bluetooth_mac_address[6];

// USB stack state

// Set to true if the USB address needs to be set after a SET_ADDRESS
static bool pbdrv_usb_addr_needs_setting;
// The USB device address to use
static uint8_t pbdrv_usb_addr;
// The active USB configuration index (either 0 if unconfigured, or else 1)
static uint8_t pbdrv_usb_config;
// Data which remains to be sent for a CONTROL IN request
static const uint32_t *pbdrv_usb_setup_data_to_send;
// Size of data remaining for a CONTROL IN
static unsigned int pbdrv_usb_setup_data_to_send_sz;
// Used to send one (or a few) bytes back, for simple GET commands on EP0.
static uint32_t pbdrv_usb_setup_misc_tx_byte;
// Whether the device is using USB high-speed mode or not
static bool pbdrv_usb_is_usb_hs;

// Helper function for dividing up buffers for EP0 and feeding the FIFO
static void usb_setup_send_chunk(void) {
    unsigned int this_chunk_sz = pbdrv_usb_setup_data_to_send_sz;
    if (this_chunk_sz > EP0_BUF_SZ) {
        this_chunk_sz = EP0_BUF_SZ;
    }

    // 4-byte-at-a-time copy
    unsigned int this_chunk_sz_words = this_chunk_sz / 4;
    for (unsigned int i = 0; i < this_chunk_sz_words; i++) {
        HWREG(USB0_BASE + USB_O_FIFO0) = pbdrv_usb_setup_data_to_send[i];
    }
    pbdrv_usb_setup_data_to_send += this_chunk_sz_words;

    // Copy remainder
    for (unsigned int i = 0; i < this_chunk_sz % 4; i++) {
        HWREGB(USB0_BASE + USB_O_FIFO0) = ((const uint8_t *)pbdrv_usb_setup_data_to_send)[i];
    }

    pbdrv_usb_setup_data_to_send_sz -= this_chunk_sz;
}

// Helper function for GET_DESCRIPTOR requests
static bool usb_get_descriptor(uint16_t wValue) {
    uint8_t desc_ty = wValue >> 8;
    uint8_t desc_idx = wValue;
    int i;

    switch (desc_ty) {
        case DESC_TYPE_DEVICE:
            pbdrv_usb_setup_data_to_send = dev_desc.u;
            pbdrv_usb_setup_data_to_send_sz = sizeof(pbdrv_usb_dev_desc_t);
            return true;

        case DESC_TYPE_DEVICE_QUALIFIER:
            pbdrv_usb_setup_data_to_send = dev_qualifier_desc.u;
            pbdrv_usb_setup_data_to_send_sz = sizeof(pbdrv_usb_dev_qualifier_desc_t);
            return true;

        case DESC_TYPE_CONFIGURATION:
            if (pbdrv_usb_is_usb_hs) {
                pbdrv_usb_setup_data_to_send = configuration_1_desc_hs.u;
            } else {
                pbdrv_usb_setup_data_to_send = configuration_1_desc_fs.u;
            }
            pbdrv_usb_setup_data_to_send_sz = sizeof(pbdrv_usb_ev3_conf_1_t);
            return true;

        case DESC_TYPE_OTHER_SPEED_CONFIGURATION:
            if (pbdrv_usb_is_usb_hs) {
                pbdrv_usb_setup_data_to_send = configuration_1_desc_fs.u;
            } else {
                pbdrv_usb_setup_data_to_send = configuration_1_desc_hs.u;
            }
            pbdrv_usb_setup_data_to_send_sz = sizeof(pbdrv_usb_ev3_conf_1_t);
            return true;

        case DESC_TYPE_STRING:
            switch (desc_idx) {
                case STRING_DESC_LANGID:
                    pbdrv_usb_setup_data_to_send = usb_str_desc_langid.u;
                    pbdrv_usb_setup_data_to_send_sz = sizeof(usb_str_desc_langid);
                    return true;

                case STRING_DESC_MFG:
                    usb_string_desc_buffer.b[1] = DESC_TYPE_STRING;
                    i = 0;
                    while (PBDRV_CONFIG_USB_MFG_STR[i]) {
                        usb_string_desc_buffer.b[2 + 2 * i] = PBDRV_CONFIG_USB_MFG_STR[i];
                        usb_string_desc_buffer.b[2 + 2 * i + 1] = 0;
                        i++;
                    }
                    usb_string_desc_buffer.b[0] = 2 * i + 2;

                    pbdrv_usb_setup_data_to_send = usb_string_desc_buffer.u;
                    pbdrv_usb_setup_data_to_send_sz = usb_string_desc_buffer.b[0];
                    return true;

                case STRING_DESC_PRODUCT:
                    usb_string_desc_buffer.b[1] = DESC_TYPE_STRING;
                    i = 0;
                    while (PBDRV_CONFIG_USB_PROD_STR[i]) {
                        usb_string_desc_buffer.b[2 + 2 * i] = PBDRV_CONFIG_USB_PROD_STR[i];
                        usb_string_desc_buffer.b[2 + 2 * i + 1] = 0;
                        i++;
                    }
                    usb_string_desc_buffer.b[0] = 2 * i + 2;

                    pbdrv_usb_setup_data_to_send = usb_string_desc_buffer.u;
                    pbdrv_usb_setup_data_to_send_sz = usb_string_desc_buffer.b[0];
                    return true;

                case STRING_DESC_SERIAL:
                    usb_string_desc_buffer.b[0] = 2 * 2 * 6 + 2;
                    usb_string_desc_buffer.b[1] = DESC_TYPE_STRING;
                    for (i = 0; i < 6; i++) {
                        usb_string_desc_buffer.b[2 + 4 * i + 0] = "0123456789ABCDEF"[pbdrv_ev3_bluetooth_mac_address[i] >> 4];
                        usb_string_desc_buffer.b[2 + 4 * i + 1] = 0;
                        usb_string_desc_buffer.b[2 + 4 * i + 2] = "0123456789ABCDEF"[pbdrv_ev3_bluetooth_mac_address[i] & 0xf];
                        usb_string_desc_buffer.b[2 + 4 * i + 3] = 0;
                    }

                    pbdrv_usb_setup_data_to_send = usb_string_desc_buffer.u;
                    pbdrv_usb_setup_data_to_send_sz = usb_string_desc_buffer.b[0];
                    return true;
            }
            break;
    }

    return false;
}

static void usb_device_intr(void) {
    IntSystemStatusClear(SYS_INT_USB0);
    uint32_t intr_src = HWREG(USB_0_OTGBASE + USB_0_INTR_SRC);

    if (intr_src & USBOTG_INTR_RESET) {
        // USB reset

        // Reset state variables
        pbdrv_usb_addr = 0;
        pbdrv_usb_config = 0;
        pbdrv_usb_setup_data_to_send = 0;
        pbdrv_usb_addr_needs_setting = false;

        if (HWREGH(USB0_BASE + USB_O_POWER) & USB_POWER_HSMODE) {
            pbdrv_usb_is_usb_hs = true;
        } else {
            pbdrv_usb_is_usb_hs = false;
        }

        // TODO: More tasks in the future
    }

    if (intr_src & USBOTG_INTR_EP0) {
        // USB EP0
        uint16_t peri_csr = HWREGH(USB0_BASE + USB_O_CSRL0);

        if (peri_csr & USB_CSRL0_STALLED) {
            // If this is a sent-stall confirmation, clear the bit
            HWREGH(USB0_BASE + USB_O_CSRL0) = 0;
            pbdrv_usb_setup_data_to_send = 0;
            pbdrv_usb_addr_needs_setting = false;
        } else if (peri_csr & USB_CSRL0_SETEND) {
            // Error in SETUP transaction
            HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_SETENDC;
            pbdrv_usb_setup_data_to_send = 0;
            pbdrv_usb_addr_needs_setting = false;
        } else {
            if (pbdrv_usb_addr_needs_setting) {
                USBDevAddrSet(USB0_BASE, pbdrv_usb_addr);
                pbdrv_usb_addr_needs_setting = false;
            }

            if (peri_csr & USB_CSRL0_RXRDY) {
                // Got a new setup packet
                pbdrv_usb_setup_packet_union_t setup_pkt;
                bool handled = false;
                pbdrv_usb_setup_data_to_send = 0;

                setup_pkt.u[0] = HWREG(USB0_BASE + USB_O_FIFO0);
                setup_pkt.u[1] = HWREG(USB0_BASE + USB_O_FIFO0);
                HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_RXRDYC;

                if ((setup_pkt.s.bmRequestType & BM_REQ_TYPE_MASK) == BM_REQ_TYPE_STANDARD) {
                    switch (setup_pkt.s.bmRequestType & BM_REQ_RECIP_MASK) {
                        case BM_REQ_RECIP_DEV:
                            switch (setup_pkt.s.bRequest) {
                                case SET_ADDRESS:
                                    pbdrv_usb_addr = setup_pkt.s.wValue;
                                    pbdrv_usb_addr_needs_setting = true;
                                    handled = true;
                                    break;

                                case SET_CONFIGURATION:
                                    if (setup_pkt.s.wValue <= 1) {
                                        pbdrv_usb_config = setup_pkt.s.wValue;

                                        if (pbdrv_usb_config == 1) {
                                            // configuring

                                            // TODO: Handle configuring
                                        } else {
                                            // deconfiguring

                                            // TODO: Handle deconfiguring
                                        }
                                        handled = true;
                                    }
                                    break;

                                case GET_CONFIGURATION:
                                    pbdrv_usb_setup_misc_tx_byte = pbdrv_usb_config;
                                    pbdrv_usb_setup_data_to_send = &pbdrv_usb_setup_misc_tx_byte;
                                    pbdrv_usb_setup_data_to_send_sz = 1;
                                    handled = true;
                                    break;

                                case GET_STATUS:
                                    pbdrv_usb_setup_misc_tx_byte = 1;     // self-powered
                                    pbdrv_usb_setup_data_to_send = &pbdrv_usb_setup_misc_tx_byte;
                                    pbdrv_usb_setup_data_to_send_sz = 2;
                                    handled = true;
                                    break;

                                case GET_DESCRIPTOR:
                                    if (usb_get_descriptor(setup_pkt.s.wValue)) {
                                        handled = true;
                                    }
                                    break;
                            }
                            break;

                        case BM_REQ_RECIP_IF:
                            if (setup_pkt.s.wIndex == 0) {
                                switch (setup_pkt.s.bRequest) {
                                    case GET_INTERFACE:
                                        pbdrv_usb_setup_misc_tx_byte = 0;
                                        pbdrv_usb_setup_data_to_send = &pbdrv_usb_setup_misc_tx_byte;
                                        pbdrv_usb_setup_data_to_send_sz = 1;
                                        handled = true;
                                        break;

                                    case GET_STATUS:
                                        pbdrv_usb_setup_misc_tx_byte = 0;
                                        pbdrv_usb_setup_data_to_send = &pbdrv_usb_setup_misc_tx_byte;
                                        pbdrv_usb_setup_data_to_send_sz = 2;
                                        handled = true;
                                        break;
                                }
                            }
                            break;
                    }
                }

                if (!handled) {
                    // send stall
                    HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_STALL;
                } else {
                    if (pbdrv_usb_setup_data_to_send) {
                        // Clamp by host request size
                        if (setup_pkt.s.wLength < pbdrv_usb_setup_data_to_send_sz) {
                            pbdrv_usb_setup_data_to_send_sz = setup_pkt.s.wLength;
                        }

                        // Send as much as we can in one chunk
                        usb_setup_send_chunk();
                        if (pbdrv_usb_setup_data_to_send_sz == 0) {
                            pbdrv_usb_setup_data_to_send = 0;
                            HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_TXRDY | USB_CSRL0_DATAEND;
                        } else {
                            HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_TXRDY;
                        }
                    } else {
                        // Just get ready to send ACK, no data
                        HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_DATAEND;
                    }
                }
            } else if (pbdrv_usb_setup_data_to_send) {
                // Need to continue to TX data
                usb_setup_send_chunk();
                if (pbdrv_usb_setup_data_to_send_sz == 0) {
                    pbdrv_usb_setup_data_to_send = 0;
                    HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_TXRDY | USB_CSRL0_DATAEND;
                } else {
                    HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_TXRDY;
                }
            }
        }
    }

    HWREG(USB_0_OTGBASE + USB_0_INTR_SRC_CLEAR) = intr_src;
    HWREG(USB_0_OTGBASE + USB_0_END_OF_INTR) = 0;
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
