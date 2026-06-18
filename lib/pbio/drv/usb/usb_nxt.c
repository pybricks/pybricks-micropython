// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2007 the NxOS developers
 * Copyright (C) 2025 the Pybricks Authors
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 */

// NXT / Atmel AT91SAM7S256 UDP driver implementing a USB CDC-ACM (virtual
// serial port) device. The byte stream is framed (COBS) by the common USB
// driver in usb.c, and the host's serial port open/close (DTR) is used to
// detect connection state, just like the STM32 and EV3 drivers.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_NXT

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/usb.h>

#include <pbio/os.h>
#include <pbio/util.h>

#include <at91sam7s256.h>

#include "nxos/interrupts.h"
#include "nxos/drivers/systick.h"
#include "nxos/drivers/aic.h"
#include "nxos/util.h"

#include <lego/usb.h>

#include "usb.h"

#include "usb_ch9.h"
#include "usb_common_desc.h"

/* The USB controller supports up to 4 endpoints. */
#define PBDRV_USB_NXT_N_ENDPOINTS 4

/* Physical endpoint numbers used by this driver. */
#define EP_CONTROL  0   /* Control endpoint. */
#define EP_BULK_OUT 1   /* CDC data, host to hub. */
#define EP_BULK_IN  2   /* CDC data, hub to host. */
#define EP_NOTIF    3   /* CDC notification (interrupt IN), never used. */

/* Maximum data packet sizes. Endpoint 0 is a special case (control endpoint). */
#define MAX_EP0_SIZE 8
#define MAX_RCV_SIZE 64
#define MAX_SND_SIZE 64
/* Packet size of the CDC notification (interrupt IN) endpoint. We never send
 * notifications, so this only needs to be large enough to be valid. */
#define NOTIF_EP_PKT_SZ 8

/**
 * Indices for string descriptors
 */
enum {
    STRING_DESC_LANGID,
    STRING_DESC_MFG,
    STRING_DESC_PRODUCT,
    STRING_DESC_SERIAL,
};

// Device descriptor. The device class uses the Interface Association
// Descriptor so that the CDC comm and data interfaces are grouped into one
// function.
static const pbdrv_usb_dev_desc_t pbdrv_usb_nxt_device_descriptor = {
    .bLength = sizeof(pbdrv_usb_dev_desc_t),
    .bDescriptorType = DESC_TYPE_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_MISC,
    .bDeviceSubClass = USB_MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = USB_MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = MAX_EP0_SIZE,
    .idVendor = PBDRV_CONFIG_USB_VID,
    .idProduct = PBDRV_CONFIG_USB_PID,
    .bcdDevice = 0x0200,
    .iManufacturer = STRING_DESC_MFG,
    .iProduct = STRING_DESC_PRODUCT,
    .iSerialNumber = STRING_DESC_SERIAL,
    .bNumConfigurations = 1,
};

typedef struct PBDRV_PACKED {
    pbdrv_usb_conf_desc_t conf_desc;
    pbdrv_usb_iad_desc_t iad;
    pbdrv_usb_iface_desc_t comm_iface;
    pbdrv_usb_cdc_header_desc_t cdc_header;
    pbdrv_usb_cdc_call_mgmt_desc_t cdc_call_mgmt;
    pbdrv_usb_cdc_acm_desc_t cdc_acm;
    pbdrv_usb_cdc_union_desc_t cdc_union;
    pbdrv_usb_ep_desc_t notif_ep;
    pbdrv_usb_iface_desc_t data_iface;
    pbdrv_usb_ep_desc_t ep_out;
    pbdrv_usb_ep_desc_t ep_in;
} pbdrv_usb_nxt_conf_t;

static const pbdrv_usb_nxt_conf_t pbdrv_usb_nxt_full_config = {
    .conf_desc = {
        .bLength = sizeof(pbdrv_usb_conf_desc_t),
        .bDescriptorType = DESC_TYPE_CONFIGURATION,
        .wTotalLength = sizeof(pbdrv_usb_nxt_conf_t),
        .bNumInterfaces = 2,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        /* Configuration attributes bitmap. Bit 7 (MSB) must be 1, bit 6 is
        * 1 because the NXT is self-powered, bit 5 is 0 because the NXT
        * doesn't support remote wakeup, and bits 0-4 are 0 (reserved).
        */
        .bmAttributes = USB_CONF_DESC_BM_ATTR_MUST_BE_SET | USB_CONF_DESC_BM_ATTR_SELF_POWERED,
        .bMaxPower = 0,
    },
    /* Interface Association: groups the comm and data interfaces into one
     * CDC ACM function. */
    .iad = {
        .bLength = sizeof(pbdrv_usb_iad_desc_t),
        .bDescriptorType = DESC_TYPE_INTERFACE_ASSOCIATION,
        .bFirstInterface = 0,
        .bInterfaceCount = 2,
        .bFunctionClass = USB_CLASS_CDC,
        .bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
        .bFunctionProtocol = USB_CDC_PROTOCOL_AT,
        .iFunction = 0,
    },
    /* Communication interface. */
    .comm_iface = {
        .bLength = sizeof(pbdrv_usb_iface_desc_t),
        .bDescriptorType = DESC_TYPE_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CLASS_CDC,
        .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
        .iInterface = 0,
    },
    .cdc_header = {
        .bFunctionLength = sizeof(pbdrv_usb_cdc_header_desc_t),
        .bDescriptorType = USB_CDC_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_FUNC_SUBTYPE_HEADER,
        .bcdCDC = 0x0110,
    },
    .cdc_call_mgmt = {
        .bFunctionLength = sizeof(pbdrv_usb_cdc_call_mgmt_desc_t),
        .bDescriptorType = USB_CDC_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_FUNC_SUBTYPE_CALL_MGMT,
        .bmCapabilities = 0x00,
        .bDataInterface = 1,
    },
    .cdc_acm = {
        .bFunctionLength = sizeof(pbdrv_usb_cdc_acm_desc_t),
        .bDescriptorType = USB_CDC_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_FUNC_SUBTYPE_ACM,
        .bmCapabilities = 0x02,
    },
    .cdc_union = {
        .bFunctionLength = sizeof(pbdrv_usb_cdc_union_desc_t),
        .bDescriptorType = USB_CDC_CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_FUNC_SUBTYPE_UNION,
        .bControlInterface = 0,
        .bSubordinateInterface0 = 1,
    },
    /* Notification endpoint (EP3, interrupt IN). The host (e.g. Linux cdc_acm)
     * requires this endpoint to exist to bind the driver, but we never send
     * notifications, so it just NAKs forever. */
    .notif_ep = {
        .bLength = sizeof(pbdrv_usb_ep_desc_t),
        .bDescriptorType = DESC_TYPE_ENDPOINT,
        .bEndpointAddress = 0x80 | EP_NOTIF,
        .bmAttributes = PBDRV_USB_EP_TYPE_INTR,
        .wMaxPacketSize = NOTIF_EP_PKT_SZ,
        .bInterval = 16,
    },
    /* Data interface. */
    .data_iface = {
        .bLength = sizeof(pbdrv_usb_iface_desc_t),
        .bDescriptorType = DESC_TYPE_INTERFACE,
        .bInterfaceNumber = 1,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_CLASS_CDC_DATA,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface = 0,
    },
    /* Bulk OUT endpoint (EP1, host to hub). */
    .ep_out = {
        .bLength = sizeof(pbdrv_usb_ep_desc_t),
        .bDescriptorType = DESC_TYPE_ENDPOINT,
        .bEndpointAddress = EP_BULK_OUT,
        .bmAttributes = PBDRV_USB_EP_TYPE_BULK,
        .wMaxPacketSize = MAX_RCV_SIZE,
        .bInterval = 0,
    },
    /* Bulk IN endpoint (EP2, hub to host). */
    .ep_in = {
        .bLength = sizeof(pbdrv_usb_ep_desc_t),
        .bDescriptorType = DESC_TYPE_ENDPOINT,
        .bEndpointAddress = 0x80 | EP_BULK_IN,
        .bmAttributes = PBDRV_USB_EP_TYPE_BULK,
        .wMaxPacketSize = MAX_SND_SIZE,
        .bInterval = 0,
    },
};

// Serial number descriptor
typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wString[6 * 3]; // 6 hex bytes separated by ':' and ending in 0.
} pbdrv_usb_serial_number_desc_t;

static pbdrv_usb_serial_number_desc_t pbdrv_usb_str_desc_serial;

// CDC line coding (baud rate, stop bits, parity, data bits). The host can set
// and get it, but we ignore the actual values since this is a virtual port.
// Defaults to 115200 8N1.
static uint8_t pbdrv_usb_nxt_line_coding[USB_CDC_LINE_CODING_SIZE] = {
    0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08,
};

// Set while EP0 is waiting for the host to send the SET_LINE_CODING data stage.
static bool pbdrv_usb_nxt_expect_line_coding;

/* True once the host has selected a configuration (SET_CONFIGURATION). This is
 * the USB analog of being enumerated and ready for data transfers. */
static volatile bool pbdrv_usb_nxt_configured;

/* True while a transmission on the data IN endpoint (EP2) is in progress. */
static volatile bool pbdrv_usb_nxt_transmitting;

/* When the host gives us an address, we must send a null ACK packet back
 * before actually changing addresses. This field stores the address that
 * should be set once the ACK is sent. */
static uint32_t pbdrv_usb_nxt_new_device_address;

/* The currently selected USB configuration. */
static uint8_t pbdrv_usb_nxt_current_config;

/* Holds the state of split (multi-packet) transmissions, indexed by the
 * physical endpoint number (the same numbering used for the CSR/FDR
 * registers). Only EP0 (control) and EP2 (bulk IN) ever transmit, so only
 * those slots are used, but sizing the arrays to the full endpoint count keeps
 * the indexing uniform with the hardware registers and avoids the previous
 * endpoint/2 aliasing.
 */
static uint8_t *pbdrv_usb_nxt_tx_data[PBDRV_USB_NXT_N_ENDPOINTS];
static uint32_t pbdrv_usb_nxt_tx_len[PBDRV_USB_NXT_N_ENDPOINTS];

/* The flags in the UDP_CSR register are a little strange: writing to
 * them does not instantly change their value. Their value will change
 * to reflect the write when the USB controller has taken the change
 * into account. The driver must wait until the controller
 * acknowledges changes to CSR.
 *
 * These helpers set/clear CSR flags, and then loop waiting for the
 * controller to synchronize
 */
static void pbdrv_usb_nxt_csr_clear_flag(uint8_t endpoint, uint32_t flags) {
    AT91C_UDP_CSR[endpoint] &= ~(flags);
    while (AT91C_UDP_CSR[endpoint] & (flags)) {
        ;
    }
}

static void pbdrv_usb_nxt_csr_set_flag(uint8_t endpoint, uint32_t flags) {
    AT91C_UDP_CSR[endpoint] |= (flags);
    while ((AT91C_UDP_CSR[endpoint] & (flags)) != (flags)) {
        ;
    }
}

/* Starts sending data to the host. If the data cannot fit into a
 * single USB packet, the data is split and scheduled to be sent in
 * several packets.
 */
static void pbdrv_usb_nxt_write_data(int endpoint, const void *ptr_, uint32_t length) {
    const uint8_t *ptr = ptr_;
    uint32_t packet_size;

    if (endpoint != EP_CONTROL && endpoint != EP_BULK_IN) {
        return;
    }

    if (endpoint == EP_CONTROL) {
        packet_size = MIN(MAX_EP0_SIZE, length);
    } else {
        packet_size = MIN(MAX_SND_SIZE, length);
    }

    /* If there is more data than can fit in a single packet, queue the
     * rest up.
     */
    if (length > packet_size) {
        length -= packet_size;
        pbdrv_usb_nxt_tx_data[endpoint] = (uint8_t *)(ptr + packet_size);
        pbdrv_usb_nxt_tx_len[endpoint] = length;
    } else {
        if (length == packet_size && endpoint == EP_CONTROL) {
            // If we are sending data to the control pipe, we must terminate the
            // data with a ZLP. In order to do so, we set the data pointer to
            // non-NULL but the length to 0. We do not want to send ZLPs on the
            // CDC data pipe.
            pbdrv_usb_nxt_tx_data[endpoint] = (uint8_t *)(ptr);
        } else {
            pbdrv_usb_nxt_tx_data[endpoint] = NULL;
        }
        pbdrv_usb_nxt_tx_len[endpoint] = 0;
    }

    /* Push a packet into the USB FIFO, and tell the controller to send. */
    while (packet_size) {
        AT91C_UDP_FDR[endpoint] = *ptr;
        ptr++;
        packet_size--;
    }
    pbdrv_usb_nxt_csr_set_flag(endpoint, AT91C_UDP_TXPKTRDY);
}


static uint8_t pbdrv_usb_rx_buf[MAX_RCV_SIZE];
static volatile uint32_t pbdrv_usb_rx_len;

/*
 * Read one data packet from the USB controller.
 */
static void pbdrv_usb_rx_update(int endpoint) {

    // Given our configuration, we should only get packets on the bulk OUT
    // endpoint. Ignore data on any other endpoint. Data from EP0 is handled
    // separately.
    if (endpoint != EP_BULK_OUT) {
        pbdrv_usb_nxt_csr_clear_flag(endpoint, AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1);
        return;
    }

    pbdrv_usb_rx_len = (AT91C_UDP_CSR[endpoint] & AT91C_UDP_RXBYTECNT) >> 16;

    // Read all available bytes.
    for (uint16_t i = 0; i < pbdrv_usb_rx_len; i++) {
        pbdrv_usb_rx_buf[i] = AT91C_UDP_FDR[EP_BULK_OUT];
    }

    // REVISIT: We could switch between RX banks to keep receiving data while
    // we process it. At the moment, the higher level code does not use this.
    pbdrv_usb_nxt_csr_clear_flag(endpoint, AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1);
}

/* On the endpoint 0: A stall is USB's way of sending
 * back an error (either "not understood" or "not handled
 * by this device"). The connexion will be reinitialized
 * by the host.
 * On the other endpoint : Indicates to the host that the endpoint is halted
 */
static void pbdrv_usb_nxt_send_stall(int endpoint) {
    pbdrv_usb_nxt_csr_set_flag(endpoint, AT91C_UDP_FORCESTALL);
}

/* During setup, we need to send packets with null data. */
static void pbdrv_usb_nxt_send_null(void) {
    pbdrv_usb_nxt_write_data(0, NULL, 0);
}

static void pbdrv_usb_handle_std_request(pbdrv_usb_setup_packet_t *packet) {
    uint32_t size;
    uint8_t index;

    switch (packet->bRequest) {
        case GET_STATUS: {
            /* The host wants to know our status.
            *
            * If it wants the device status, just reply that the NXT is still
            * self-powered (as first declared by the setup packets). If it
            * wants endpoint status, reply that the endpoint has not
            * halted. Any other status request types are reserved, which
            * translates to replying zero.
            */
            uint16_t response;

            if ((packet->bmRequestType & BM_REQ_RECIP_MASK) == BM_REQ_RECIP_DEV) {
                response = 1;
            } else {
                response = 0;
            }

            pbdrv_usb_nxt_write_data(EP_CONTROL, &response, 2);
        }
        break;

        case CLEAR_FEATURE:
        case SET_INTERFACE:
        case SET_FEATURE:
            /* TODO: Refer back to the specs and send the right
             * replies. This is wrong, even though it happens to not break
             * on linux.
             */
            pbdrv_usb_nxt_send_null();
            break;

        case SET_ADDRESS:
            /* The host has given the NXT a new USB address. This address
             * must be set AFTER sending the ack packet. Therefore, we just
             * remember the new address, and the interrupt handler will set
             * it when the transmission completes.
             */
            pbdrv_usb_nxt_new_device_address = packet->wValue;
            pbdrv_usb_nxt_send_null();

            /* If the address change is to 0, do it immediately.
             *
             * TODO: Why? And when does this happen?
             */
            if (pbdrv_usb_nxt_new_device_address == 0) {
                *AT91C_UDP_FADDR = AT91C_UDP_FEN;
                *AT91C_UDP_GLBSTATE = 0;
            }
            break;

        case GET_DESCRIPTOR:
            /* The host requested a descriptor. */

            index = (packet->wValue & 0xFF);
            switch (packet->wValue >> 8) {
                case DESC_TYPE_DEVICE: /* Device descriptor */
                    size = sizeof(pbdrv_usb_nxt_device_descriptor);
                    pbdrv_usb_nxt_write_data(EP_CONTROL, &pbdrv_usb_nxt_device_descriptor,
                        MIN(size, packet->wLength));
                    break;

                case DESC_TYPE_CONFIGURATION: /* Configuration descriptor */
                    size = sizeof(pbdrv_usb_nxt_full_config);
                    pbdrv_usb_nxt_write_data(EP_CONTROL, &pbdrv_usb_nxt_full_config,
                        MIN(size, packet->wLength));
                    break;

                case DESC_TYPE_STRING: /* String or language info. */
                {
                    const void *desc = 0;
                    switch (index) {
                        case STRING_DESC_LANGID:
                            desc = &pbdrv_usb_str_desc_langid;
                            size = sizeof(pbdrv_usb_str_desc_langid.s);
                            break;
                        case STRING_DESC_MFG:
                            desc = &pbdrv_usb_str_desc_mfg;
                            size = sizeof(pbdrv_usb_str_desc_mfg.s);
                            break;
                        case STRING_DESC_PRODUCT:
                            desc = &pbdrv_usb_str_desc_prod;
                            size = sizeof(pbdrv_usb_str_desc_prod.s);
                            break;
                        case STRING_DESC_SERIAL:
                            desc = &pbdrv_usb_str_desc_serial;
                            size = sizeof(pbdrv_usb_str_desc_serial);
                            break;
                    }

                    if (desc) {
                        pbdrv_usb_nxt_write_data(EP_CONTROL, desc, MIN(size, packet->wLength));
                    } else {
                        pbdrv_usb_nxt_send_stall(EP_CONTROL);
                    }
                }
                break;

                default: /* Unknown descriptor, tell the host by stalling. */
                    pbdrv_usb_nxt_send_stall(EP_CONTROL);
            }
            break;

        case GET_CONFIGURATION:
            /* The host wants to know the ID of the current configuration. */
            pbdrv_usb_nxt_write_data(EP_CONTROL, &pbdrv_usb_nxt_current_config, 1);
            break;

        case SET_CONFIGURATION:
            /* The host selected a new configuration. */
            pbdrv_usb_nxt_current_config = packet->wValue;

            /* we ack */
            pbdrv_usb_nxt_send_null();

            /* we set the register in configured mode */
            *AT91C_UDP_GLBSTATE = packet->wValue > 0 ?
                (AT91C_UDP_CONFG | AT91C_UDP_FADDEN) :AT91C_UDP_FADDEN;

            /* Enable the CDC data and notification endpoints. */
            AT91C_UDP_CSR[EP_BULK_OUT] = AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT;
            while (AT91C_UDP_CSR[EP_BULK_OUT] != (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT)) {
                ;
            }

            AT91C_UDP_CSR[EP_BULK_IN] = AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN;
            while (AT91C_UDP_CSR[EP_BULK_IN] != (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN)) {
                ;
            }

            AT91C_UDP_CSR[EP_NOTIF] = AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_INT_IN;
            while (AT91C_UDP_CSR[EP_NOTIF] != (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_INT_IN)) {
                ;
            }

            pbdrv_usb_nxt_configured = packet->wValue > 0;
            break;

        case GET_INTERFACE: /* TODO: This should respond, not stall. */
        case SET_DESCRIPTOR:
        default:
            pbdrv_usb_nxt_send_stall(EP_CONTROL);
            break;
    }
}

static void pbdrv_usb_nxt_handle_class_request(pbdrv_usb_setup_packet_t *packet) {
    // CDC class requests are directed at the comm interface.
    if ((packet->bmRequestType & BM_REQ_RECIP_MASK) != BM_REQ_RECIP_IF) {
        pbdrv_usb_nxt_send_stall(EP_CONTROL);
        return;
    }

    switch (packet->bRequest) {
        case USB_CDC_REQ_SET_LINE_CODING:
            // The 7-byte line coding follows in an OUT data stage, which we
            // read on the next EP0 RX_DATA interrupt. Do not ack yet.
            pbdrv_usb_nxt_expect_line_coding = true;
            break;

        case USB_CDC_REQ_GET_LINE_CODING:
            pbdrv_usb_nxt_write_data(EP_CONTROL, pbdrv_usb_nxt_line_coding,
                MIN(sizeof(pbdrv_usb_nxt_line_coding), packet->wLength));
            break;

        case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
            // DTR asserted means a host app opened the serial port. This is
            // the USB analog of a BLE host subscribing to notifications, and
            // is how we detect connect/disconnect.
            pbdrv_usb_on_dtr_changed(
                (packet->wValue & USB_CDC_CONTROL_LINE_STATE_DTR) != 0);
            pbdrv_usb_nxt_send_null();
            break;

        default:
            pbdrv_usb_nxt_send_stall(EP_CONTROL);
            break;
    }
}

/* Handle receiving and responding to setup packets on EP0. */
static void pbdrv_usb_nxt_manage_setup_packet(void) {
    /* The structure of a USB setup packet. */
    pbdrv_usb_setup_packet_t packet;

    /* Read the packet from the FIFO into the above packet struct. */
    packet.bmRequestType = AT91C_UDP_FDR[EP_CONTROL];
    packet.bRequest = AT91C_UDP_FDR[EP_CONTROL];
    packet.wValue = (AT91C_UDP_FDR[EP_CONTROL] & 0xFF) | (AT91C_UDP_FDR[EP_CONTROL] << 8);
    packet.wIndex = (AT91C_UDP_FDR[EP_CONTROL] & 0xFF) | (AT91C_UDP_FDR[EP_CONTROL] << 8);
    packet.wLength = (AT91C_UDP_FDR[EP_CONTROL] & 0xFF) | (AT91C_UDP_FDR[EP_CONTROL] << 8);

    if ((packet.bmRequestType & BM_REQ_DIR_MASK) == BM_REQ_DIR_D2H) {
        pbdrv_usb_nxt_csr_set_flag(EP_CONTROL, AT91C_UDP_DIR); /* TODO: contradicts atmel doc p475 */
    }

    pbdrv_usb_nxt_csr_clear_flag(EP_CONTROL, AT91C_UDP_RXSETUP);

    switch (packet.bmRequestType & BM_REQ_TYPE_MASK) {
        case BM_REQ_TYPE_STANDARD:
            pbdrv_usb_handle_std_request(&packet);
            break;
        case BM_REQ_TYPE_CLASS:
            pbdrv_usb_nxt_handle_class_request(&packet);
            break;
        default:
            pbdrv_usb_nxt_send_stall(EP_CONTROL);
            break;
    }
}

/* The main USB interrupt handler. */
static void pbdrv_usb_nxt_isr(void) {
    uint8_t endpoint = 127;
    uint32_t csr, isr;

    isr = *AT91C_UDP_ISR;

    /* We sent a stall, the host has acknowledged the stall. */
    if (AT91C_UDP_CSR[0] & AT91C_UDP_ISOERROR) {
        pbdrv_usb_nxt_csr_clear_flag(0, AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR);
    }

    /* End of bus reset. Starting the device setup procedure. */
    if (isr & AT91C_UDP_ENDBUSRES) {
        pbdrv_usb_nxt_configured = false;

        /* Disable and clear all interruptions, reverting to the base
         * state.
         */
        *AT91C_UDP_IDR = ~0;
        *AT91C_UDP_ICR = ~0;

        /* Reset all endpoint FIFOs. */
        *AT91C_UDP_RSTEP = ~0;
        *AT91C_UDP_RSTEP = 0;

        /* Reset internal state. */
        pbdrv_usb_nxt_current_config = 0;

        /* Reset EP0 to a basic control endpoint. */
        /* TODO: The while is ugly. Fix it. */
        AT91C_UDP_CSR[0] = AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_CTRL;
        while (AT91C_UDP_CSR[0] != (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_CTRL)) {
            ;
        }

        /* Enable interrupt handling for all three endpoints, as well as
         * suspend/resume.
         */
        *AT91C_UDP_IER = (AT91C_UDP_EPINT0 | AT91C_UDP_EPINT1 |
            AT91C_UDP_EPINT2 | AT91C_UDP_EPINT3 |
            AT91C_UDP_RXSUSP | AT91C_UDP_RXRSM);

        /* Enable the function endpoints, setting address 0, and return
         * immediately. Given that we've just reset everything, there's no
         * point in continuing.
         */
        *AT91C_UDP_FADDR = AT91C_UDP_FEN;

        return;
    }

    if (isr & AT91C_UDP_WAKEUP) {
        *AT91C_UDP_ICR = AT91C_UDP_WAKEUP;
        isr &= ~AT91C_UDP_WAKEUP;
    }

    if (isr & AT91C_UDP_SOFINT) {
        *AT91C_UDP_ICR = AT91C_UDP_SOFINT;
        isr &= ~AT91C_UDP_SOFINT;
    }

    if (isr & AT91C_UDP_RXSUSP) {
        *AT91C_UDP_ICR = AT91C_UDP_RXSUSP;
        isr &= ~AT91C_UDP_RXSUSP;
    }

    if (isr & AT91C_UDP_RXRSM) {
        *AT91C_UDP_ICR = AT91C_UDP_RXRSM;
        isr &= ~AT91C_UDP_RXRSM;
    }

    for (endpoint = 0; endpoint < PBDRV_USB_NXT_N_ENDPOINTS; endpoint++) {
        if (isr & (1 << endpoint)) {
            break;
        }
    }

    if (endpoint == 0) {

        if (AT91C_UDP_CSR[0] & AT91C_UDP_RXSETUP) {
            pbdrv_usb_nxt_manage_setup_packet();
            return;
        }

        if (AT91C_UDP_CSR[0] & (AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1)) {
            /* OUT data on the control endpoint. This is either the data stage
             * of a host-to-device control write (only SET_LINE_CODING for us)
             * or the zero-length OUT status stage that terminates a control
             * read (e.g. GET_DESCRIPTOR).
             */
            if (pbdrv_usb_nxt_expect_line_coding) {
                uint32_t count = (AT91C_UDP_CSR[0] & AT91C_UDP_RXBYTECNT) >> 16;
                for (uint32_t i = 0; i < count && i < sizeof(pbdrv_usb_nxt_line_coding); i++) {
                    pbdrv_usb_nxt_line_coding[i] = AT91C_UDP_FDR[0];
                }
                pbdrv_usb_nxt_expect_line_coding = false;
                pbdrv_usb_nxt_csr_clear_flag(0, AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1);
                /* Acknowledge the SET_LINE_CODING data stage with a ZLP. */
                pbdrv_usb_nxt_send_null();
            } else {
                /* OUT status stage of a control read. The hardware ACKs it when
                 * we clear the RX flag; we must NOT send anything back here, or
                 * we would leave a stray IN packet armed on EP0 and corrupt the
                 * next control transfer.
                 */
                pbdrv_usb_nxt_csr_clear_flag(0, AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1);
            }
            return;
        }
    }

    if (endpoint < PBDRV_USB_NXT_N_ENDPOINTS) { /* if an endpoint was specified */
        csr = AT91C_UDP_CSR[endpoint];

        if (csr & AT91C_UDP_RX_DATA_BK0
            || csr & AT91C_UDP_RX_DATA_BK1) {

            if (endpoint == EP_BULK_OUT) {
                AT91C_UDP_CSR[EP_BULK_OUT] &= ~AT91C_UDP_EPEDS;
                while (AT91C_UDP_CSR[EP_BULK_OUT] & AT91C_UDP_EPEDS) {
                    ;
                }
            }

            pbdrv_usb_rx_update(endpoint);
            pbio_os_request_poll();

            return;
        }

        if (csr & AT91C_UDP_TXCOMP) {

            /* so first we will reset this flag */
            pbdrv_usb_nxt_csr_clear_flag(endpoint, AT91C_UDP_TXCOMP);

            if (pbdrv_usb_nxt_new_device_address > 0) {
                /* the previous message received was SET_ADDR */
                /* now that the computer ACK our send_null(), we can
                 * set this address for real */

                /* we set the specified usb address in the controller */
                *AT91C_UDP_FADDR = AT91C_UDP_FEN | pbdrv_usb_nxt_new_device_address;
                /* and we tell the controller that we are in addressed mode now */
                *AT91C_UDP_GLBSTATE = AT91C_UDP_FADDEN;
                pbdrv_usb_nxt_new_device_address = 0;
            }

            /* and we will send the following data */
            if (pbdrv_usb_nxt_tx_data[endpoint] != NULL) {
                pbdrv_usb_nxt_write_data(endpoint, pbdrv_usb_nxt_tx_data[endpoint],
                    pbdrv_usb_nxt_tx_len[endpoint]);
            } else {
                /* then it means that we sent all the data and the host has acknowledged it */
                if (endpoint == EP_BULK_IN) {
                    pbdrv_usb_nxt_transmitting = false;
                }
                pbio_os_request_poll();
            }
            return;
        }

    }

    /* We clear also the unused bits,
     * just "to be sure" */
    if (isr) {
        *AT91C_UDP_ICR = 0xFFFFC4F0;
    }
}

void pbdrv_usb_nxt_deinit(void) {
    nx_aic_disable(AT91C_ID_UDP);

    *AT91C_PIOA_PER = (1 << 16);
    *AT91C_PIOA_OER = (1 << 16);
    *AT91C_PIOA_SODR = (1 << 16);
    nx_systick_wait_ms(200);
}

void pbdrv_usb_init_device(void) {

    extern char bluetooth_address_string[PBIO_ARRAY_SIZE(pbdrv_usb_str_desc_serial.wString)];
    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(pbdrv_usb_str_desc_serial.wString); i++) {
        pbdrv_usb_str_desc_serial.wString[i] = bluetooth_address_string[i];
    }
    pbdrv_usb_str_desc_serial.bLength = sizeof(pbdrv_usb_str_desc_serial);
    pbdrv_usb_str_desc_serial.bDescriptorType = DESC_TYPE_STRING;

    pbdrv_usb_nxt_deinit();

    pbdrv_usb_nxt_configured = false;
    pbdrv_usb_nxt_transmitting = false;
    pbdrv_usb_nxt_expect_line_coding = false;
    pbdrv_usb_nxt_new_device_address = 0;
    pbdrv_usb_nxt_current_config = 0;
    for (int i = 0; i < PBDRV_USB_NXT_N_ENDPOINTS; i++) {
        pbdrv_usb_nxt_tx_data[i] = NULL;
        pbdrv_usb_nxt_tx_len[i] = 0;
    }
    pbdrv_usb_rx_len = 0;

    uint32_t state = nx_interrupts_disable();

    /* usb pll was already set in init.S */

    /* enable peripheral clock */
    *AT91C_PMC_PCER = (1 << AT91C_ID_UDP);

    /* enable system clock */
    *AT91C_PMC_SCER = AT91C_PMC_UDP;

    /* disable all the interruptions */
    *AT91C_UDP_IDR = ~0;

    /* reset all the endpoints */
    *AT91C_UDP_RSTEP = 0xF;
    *AT91C_UDP_RSTEP = 0;

    *AT91C_UDP_ICR = 0xFFFFFFFF;

    /* Install the interruption routine */

    /* the first interruption we will get is an ENDBUSRES
     * this interruption is always emit (can't be disable with UDP_IER)
     */
    /* other interruptions will be enabled when needed */
    nx_aic_install_isr(AT91C_ID_UDP, AIC_PRIO_DRIVER, AIC_TRIG_LEVEL, pbdrv_usb_nxt_isr);

    nx_interrupts_enable(state);

    /* Enable the UDP pull up by outputting a zero on PA.16 */
    /* Enabling the pull up will tell to the host (the computer) that
     * we are ready for a communication
     */
    *AT91C_PIOA_PER = (1 << 16);
    *AT91C_PIOA_OER = (1 << 16);
    *AT91C_PIOA_CODR = (1 << 16);
}

void pbdrv_usb_deinit_device(void) {
    pbdrv_usb_nxt_deinit();
}

pbio_error_t pbdrv_usb_wait_until_configured(pbio_os_state_t *state) {
    return pbdrv_usb_nxt_configured ? PBIO_SUCCESS : PBIO_ERROR_AGAIN;
}

bool pbdrv_usb_is_ready(void) {
    return pbdrv_usb_nxt_configured;
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

pbio_error_t pbdrv_usb_tx_chunk(pbio_os_state_t *state, const uint8_t *data, uint32_t size) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    if (pbdrv_usb_nxt_transmitting) {
        return PBIO_ERROR_BUSY;
    }

    pbdrv_usb_nxt_transmitting = true;
    pbio_os_timer_set(&timer, PBDRV_USB_TRANSMIT_TIMEOUT);

    // Transmit the raw bytes. Framing is handled by the common driver.
    pbdrv_usb_nxt_write_data(EP_BULK_IN, data, size);

    PBIO_OS_AWAIT_UNTIL(state, !pbdrv_usb_nxt_transmitting || pbio_os_timer_is_expired(&timer));

    if (pbio_os_timer_is_expired(&timer)) {
        return PBIO_ERROR_TIMEDOUT;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state) {
    pbdrv_usb_nxt_tx_data[EP_BULK_IN] = NULL;
    pbdrv_usb_nxt_tx_len[EP_BULK_IN] = 0;
    pbdrv_usb_nxt_transmitting = false;
    return PBIO_SUCCESS;
}

uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data) {

    if (!pbdrv_usb_rx_len) {
        return 0;
    }

    // Copy data saved during interrupt to usb process.
    memcpy(data, pbdrv_usb_rx_buf, pbdrv_usb_rx_len);
    uint32_t result = pbdrv_usb_rx_len;
    pbdrv_usb_rx_len = 0;

    // Get ready to receive the next message.
    if (pbdrv_usb_nxt_configured) {
        AT91C_UDP_CSR[EP_BULK_OUT] |= AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT;
    }

    return result;
}

#endif // PBDRV_CONFIG_USB_NXT
