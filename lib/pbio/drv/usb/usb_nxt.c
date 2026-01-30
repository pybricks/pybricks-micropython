// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2007 the NxOS developers
 * Copyright (C) 2025 the Pybricks Authors
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 */

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_NXT

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/usb.h>

#include <pbio/protocol.h>
#include <pbio/version.h>
#include <pbsys/config.h>
#include <pbsys/storage.h>

#include <at91sam7s256.h>

#include "nxos/interrupts.h"
#include "nxos/assert.h"
#include "nxos/drivers/systick.h"
#include "nxos/drivers/aic.h"
#include "nxos/util.h"

#include <lego/usb.h>

#include "usb.h"

#include "usb_ch9.h"
#include "usb_common_desc.h"

/* The USB controller supports up to 4 endpoints. */
#define PBDRV_USB_NXT_N_ENDPOINTS 4

/* Maximum data packet sizes. Endpoint 0 is a special case (control
 * endpoint).
 *
 * TODO: Discuss the need/use for separating recv/send.
 */
#define MAX_EP0_SIZE 8
#define MAX_RCV_SIZE 64
#define MAX_SND_SIZE 64

/* Various constants for the setup packets.
 *
 * TODO: clean up these. Most are unused.
 */
#define USB_BMREQUEST_DIR             0x80
#define   USB_BMREQUEST_H_TO_D          0x00
#define   USB_BMREQUEST_D_TO_H          0x80
#define USB_BMREQUEST_TYPE            0x60
#define   USB_BMREQUEST_TYPE_STD        0x00
#define   USB_BMREQUEST_TYPE_CLASS      0x20
#define   USB_BMREQUEST_TYPE_VENDOR     0x40
#define USB_BMREQUEST_RCPT            0x1F
#define   USB_BMREQUEST_RCPT_DEV        0x00 /* device */
#define   USB_BMREQUEST_RCPT_INT        0x01 /* interface */
#define   USB_BMREQUEST_RCPT_EPT        0x02 /* endpoint */
#define   USB_BMREQUEST_RCPT_OTH        0x03 /* other */

// Standard requests
#define USB_BREQUEST_GET_STATUS      0x0
#define USB_BREQUEST_CLEAR_FEATURE   0x1
#define USB_BREQUEST_SET_FEATURE     0x3
#define USB_BREQUEST_SET_ADDRESS     0x5
#define USB_BREQUEST_GET_DESCRIPTOR  0x6
#define USB_BREQUEST_SET_DESCRIPTOR  0x7
#define USB_BREQUEST_GET_CONFIG      0x8
#define USB_BREQUEST_SET_CONFIG      0x9
#define USB_BREQUEST_GET_INTERFACE   0xA
#define USB_BREQUEST_SET_INTERFACE   0xB

#define USB_WVALUE_TYPE        (0xFF << 8)
#define USB_DESC_TYPE_DEVICE           1
#define USB_DESC_TYPE_CONFIG           2
#define USB_DESC_TYPE_STR              3
#define USB_DESC_TYPE_INT              4
#define USB_DESC_TYPE_ENDPT            5
#define USB_DESC_TYPE_DEVICE_QUALIFIER 6
#define USB_DESC_TYPE_BOS              15

// BOS descriptor related defines
#define USB_DEVICE_CAPABILITY_TYPE    0x10
#define USB_DEV_CAP_TYPE_PLATFORM     5

#define USB_WVALUE_INDEX       0xFF

/**
 * Indices for string descriptors
 */
enum {
    STRING_DESC_LANGID,
    STRING_DESC_MFG,
    STRING_DESC_PRODUCT,
    STRING_DESC_SERIAL,
};

/* The following definitions are 'raw' USB setup packets. They are all
 * standard responses to various setup requests by the USB host. These
 * packets are all constant, and mostly boilerplate. Don't be too
 * bothered if you skip over these to real code.
 *
 * If you want to understand the full meaning of every bit of these
 * packets, you should refer to the USB 2.0 specifications.
 *
 * One point of interest: the USB device space is partitionned by
 * vendor and product ID. As we are lacking money and real need, we
 * don't have a vendor ID to use. Therefore, we are currently
 * piggybacking on Lego's device space, using an unused product ID.
 */
static const pbdrv_usb_dev_desc_t pbdrv_usb_nxt_device_descriptor = {
    .bLength = sizeof(pbdrv_usb_dev_desc_t),
    .bDescriptorType = DESC_TYPE_DEVICE,
    .bcdUSB = 0x0210,       /* This packet is USB 2.1 (needed for BOS descriptors). */
    .bDeviceClass = PBIO_PYBRICKS_USB_DEVICE_CLASS,
    .bDeviceSubClass = PBIO_PYBRICKS_USB_DEVICE_SUBCLASS,
    .bDeviceProtocol = PBIO_PYBRICKS_USB_DEVICE_PROTOCOL,
    .bMaxPacketSize0 = MAX_EP0_SIZE,
    .idVendor = 0x0694,     /* Vendor ID : LEGO */
    .idProduct = 0x0002,    /* Product ID : NXT */
    .bcdDevice = 0x0200,    /* Product revision: 2.0.0. */
    .iManufacturer = STRING_DESC_MFG,
    .iProduct = STRING_DESC_PRODUCT,
    .iSerialNumber = STRING_DESC_SERIAL,
    .bNumConfigurations = 1,
};

typedef struct PBDRV_PACKED {
    pbdrv_usb_conf_desc_t conf_desc;
    pbdrv_usb_iface_desc_t iface_desc;
    pbdrv_usb_ep_desc_t ep_out;
    pbdrv_usb_ep_desc_t ep_in;
} pbdrv_usb_nxt_conf_t;

static const pbdrv_usb_nxt_conf_t pbdrv_usb_nxt_full_config = {
    .conf_desc = {
        .bLength = sizeof(pbdrv_usb_conf_desc_t),
        .bDescriptorType = DESC_TYPE_CONFIGURATION,
        .wTotalLength = sizeof(pbdrv_usb_nxt_conf_t),
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        /* Configuration attributes bitmap. Bit 7 (MSB) must be 1, bit 6 is
        * 1 because the NXT is self-powered, bit 5 is 0 because the NXT
        * doesn't support remote wakeup, and bits 0-4 are 0 (reserved).
        */
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
    /*
     * Descriptor for EP1.
     */
    .ep_out = {
        .bLength = sizeof(pbdrv_usb_ep_desc_t),
        .bDescriptorType = DESC_TYPE_ENDPOINT,
        .bEndpointAddress = 0x01,   /* Endpoint number. MSB is zero, meaning this is an OUT EP. */
        .bmAttributes = PBDRV_USB_EP_TYPE_BULK,
        .wMaxPacketSize = MAX_RCV_SIZE,
        .bInterval = 0,
    },
    /*
     * Descriptor for EP2.
     */
    .ep_in = {
        .bLength = sizeof(pbdrv_usb_ep_desc_t),
        .bDescriptorType = DESC_TYPE_ENDPOINT,
        .bEndpointAddress = 0x82,   /* Endpoint number. MSB is one, meaning this is an IN EP. */
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

typedef enum {
    USB_UNINITIALIZED,
    USB_READY,
    USB_BUSY,
    USB_SUSPENDED,
} pbdrv_usb_nxt_status_t;

/*
 * The USB device state. Contains the current USB state (selected
 * configuration, etc.) and transitory state for data transfers.
 */
static volatile struct {
    /* The current state of the device. */
    pbdrv_usb_nxt_status_t status;

    /* Holds the status the bus was in before entering suspend. */
    pbdrv_usb_nxt_status_t pre_suspend_status;

    /* When the host gives us an address, we must send a null ACK packet
     * back before actually changing addresses. This field stores the
     * address that should be set once the ACK is sent.
     */
    uint32_t new_device_address;

    /* The currently selected USB configuration. */
    uint8_t current_config;

    /* Holds the state of the data transmissions on both EP0 and
     * EP2. This only gets used if the transmission needed to be split
     * into several USB packets.
     *  0 = EP0
     *  1 = EP2
     */
    uint8_t *tx_data[2];
    uint32_t tx_len[2];
} pbdrv_usb_nxt_state;

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
    int tx;

    if (endpoint != 0 && endpoint != 2) {
        return;
    }

    tx = endpoint / 2;

    /* The bus is now busy. */
    pbdrv_usb_nxt_state.status = USB_BUSY;

    if (endpoint == 0) {
        packet_size = MIN(MAX_EP0_SIZE, length);
    } else {
        packet_size = MIN(MAX_SND_SIZE, length);
    }

    /* If there is more data than can fit in a single packet, queue the
     * rest up.
     */
    if (length > packet_size) {
        length -= packet_size;
        pbdrv_usb_nxt_state.tx_data[tx] = (uint8_t *)(ptr + packet_size);
        pbdrv_usb_nxt_state.tx_len[tx] = length;
    } else {
        if (length == packet_size && endpoint == 0) {
            // If we are sending data to the control pipe, we must terminate the data
            // with a ZLP. In order to do so, we set the data pointer to non-NULL
            // but the length to 0. We do not want to send ZLPs on the Pybricks bulk pipe.
            pbdrv_usb_nxt_state.tx_data[tx] = (uint8_t *)(ptr);
        } else {
            pbdrv_usb_nxt_state.tx_data[tx] = NULL;
        }
        pbdrv_usb_nxt_state.tx_len[tx] = 0;
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

    // Given our configuration, we should only get packets on endpoint 1.
    // Ignore data on any other endpoint. Data from EP0 is handled separately.
    if (endpoint != 1) {
        pbdrv_usb_nxt_csr_clear_flag(endpoint, AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1);
        return;
    }

    pbdrv_usb_rx_len = (AT91C_UDP_CSR[endpoint] & AT91C_UDP_RXBYTECNT) >> 16;

    // Read all available bytes.
    for (uint16_t i = 0; i < pbdrv_usb_rx_len; i++) {
        pbdrv_usb_rx_buf[i] = AT91C_UDP_FDR[1];
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
    pbdrv_usb_nxt_state.status = USB_UNINITIALIZED;
    pbdrv_usb_nxt_csr_set_flag(endpoint, AT91C_UDP_FORCESTALL);
}

/* During setup, we need to send packets with null data. */
static void pbdrv_usb_nxt_send_null(void) {
    pbdrv_usb_nxt_write_data(0, NULL, 0);
}

typedef struct {
    uint8_t request_attrs;  /* Request characteristics. */
    uint8_t request;        /* Request type. */
    uint16_t value;         /* Request-specific value. */
    uint16_t index;         /* Request-specific index. */
    uint16_t length;        /* The number of bytes transferred in the (optional)
                             * second phase of the control transfer. */
} pbdrv_usb_nxt_setup_packet_t;

static void pbdrv_usb_handle_std_request(pbdrv_usb_nxt_setup_packet_t *packet) {
    uint32_t size;
    uint8_t index;

    switch (packet->request) {
        case USB_BREQUEST_GET_STATUS: {
            /* The host wants to know our status.
            *
            * If it wants the device status, just reply that the NXT is still
            * self-powered (as first declared by the setup packets). If it
            * wants endpoint status, reply that the endpoint has not
            * halted. Any other status request types are reserved, which
            * translates to replying zero.
            */
            uint16_t response;

            if ((packet->request_attrs & USB_BMREQUEST_RCPT) == USB_BMREQUEST_RCPT_DEV) {
                response = 1;
            } else {
                response = 0;
            }

            pbdrv_usb_nxt_write_data(0, &response, 2);
        }
        break;

        case USB_BREQUEST_CLEAR_FEATURE:
        case USB_BREQUEST_SET_INTERFACE:
        case USB_BREQUEST_SET_FEATURE:
            /* TODO: Refer back to the specs and send the right
             * replies. This is wrong, even though it happens to not break
             * on linux.
             */
            pbdrv_usb_nxt_send_null();
            break;

        case USB_BREQUEST_SET_ADDRESS:
            /* The host has given the NXT a new USB address. This address
             * must be set AFTER sending the ack packet. Therefore, we just
             * remember the new address, and the interrupt handler will set
             * it when the transmission completes.
             */
            pbdrv_usb_nxt_state.new_device_address = packet->value;
            pbdrv_usb_nxt_send_null();

            /* If the address change is to 0, do it immediately.
             *
             * TODO: Why? And when does this happen?
             */
            if (pbdrv_usb_nxt_state.new_device_address == 0) {
                *AT91C_UDP_FADDR = AT91C_UDP_FEN;
                *AT91C_UDP_GLBSTATE = 0;
            }
            break;

        case USB_BREQUEST_GET_DESCRIPTOR:
            /* The host requested a descriptor. */

            index = (packet->value & USB_WVALUE_INDEX);
            switch ((packet->value & USB_WVALUE_TYPE) >> 8) {
                case USB_DESC_TYPE_DEVICE: /* Device descriptor */
                    size = sizeof(pbdrv_usb_nxt_device_descriptor);
                    pbdrv_usb_nxt_write_data(0, &pbdrv_usb_nxt_device_descriptor,
                        MIN(size, packet->length));
                    break;

                case USB_DESC_TYPE_CONFIG: /* Configuration descriptor */
                    size = sizeof(pbdrv_usb_nxt_full_config);
                    pbdrv_usb_nxt_write_data(0, &pbdrv_usb_nxt_full_config,
                        MIN(size, packet->length));
                    break;

                case USB_DESC_TYPE_STR: /* String or language info. */
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
                        pbdrv_usb_nxt_write_data(0, desc, MIN(size, packet->length));
                    } else {
                        pbdrv_usb_nxt_send_stall(0);
                    }
                }
                break;

                case USB_DESC_TYPE_BOS: /* BOS descriptor */
                    size = sizeof(pbdrv_usb_bos_desc_set.s);
                    pbdrv_usb_nxt_write_data(0, &pbdrv_usb_bos_desc_set, MIN(size, packet->length));
                    break;

                default: /* Unknown descriptor, tell the host by stalling. */
                    pbdrv_usb_nxt_send_stall(0);
            }
            break;

        case USB_BREQUEST_GET_CONFIG:
            /* The host wants to know the ID of the current configuration. */
            pbdrv_usb_nxt_write_data(0, (uint8_t *)&(pbdrv_usb_nxt_state.current_config), 1);
            break;

        case USB_BREQUEST_SET_CONFIG:
            /* The host selected a new configuration. */
            pbdrv_usb_nxt_state.current_config = packet->value;

            /* we ack */
            pbdrv_usb_nxt_send_null();

            /* we set the register in configured mode */
            *AT91C_UDP_GLBSTATE = packet->value > 0 ?
                (AT91C_UDP_CONFG | AT91C_UDP_FADDEN) :AT91C_UDP_FADDEN;

            /* TODO: Make this a little nicer. Not quite sure how. */

            AT91C_UDP_CSR[1] = AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT;
            while (AT91C_UDP_CSR[1] != (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT)) {
                ;
            }

            AT91C_UDP_CSR[2] = AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN;
            while (AT91C_UDP_CSR[2] != (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN)) {
                ;
            }
            AT91C_UDP_CSR[3] = 0;
            while (AT91C_UDP_CSR[3] != 0) {
                ;
            }

            pbdrv_usb_nxt_state.status = USB_READY;
            break;

        case USB_BREQUEST_GET_INTERFACE: /* TODO: This should respond, not stall. */
        case USB_BREQUEST_SET_DESCRIPTOR:
        default:
            pbdrv_usb_nxt_send_stall(0);
            break;
    }
}

static void pbdrv_usb_nxt_handle_class_request(pbdrv_usb_nxt_setup_packet_t *packet) {
    switch (packet->request_attrs & USB_BMREQUEST_RCPT) {
        case USB_BMREQUEST_RCPT_INT:
            // Ignoring wIndex for now as we only have one interface.
            switch (packet->request) {
                case PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_GATT:
                    // Standard GATT characteristic
                    switch (packet->value) {
                        case 0x2A00: { // device name
                            const char *name = pbdrv_bluetooth_get_hub_name();
                            pbdrv_usb_nxt_write_data(0, name,
                                MIN(strlen(name), packet->length));
                            break;
                        }
                        case 0x2A26: { // firmware revision
                            const char *fw = PBIO_VERSION_STR;
                            pbdrv_usb_nxt_write_data(0, fw,
                                MIN(strlen(fw), packet->length));
                            break;
                        }
                        case 0x2A28: { // software revision
                            const char *sw = PBIO_PROTOCOL_VERSION_STR;
                            pbdrv_usb_nxt_write_data(0, sw,
                                MIN(strlen(sw), packet->length));
                            break;
                        }
                        default:
                            pbdrv_usb_nxt_send_stall(0);
                            break;
                    }
                    break;
                case PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_PYBRICKS:
                    // Pybricks characteristic
                    switch (packet->value) {
                        case 0x0003: { // hub capabilities
                            uint8_t caps[PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE];
                            pbio_pybricks_hub_capabilities(caps,
                                MAX_RCV_SIZE - 1,
                                PBSYS_CONFIG_APP_FEATURE_FLAGS,
                                pbsys_storage_get_maximum_program_size(),
                                PBSYS_CONFIG_HMI_NUM_SLOTS);
                            pbdrv_usb_nxt_write_data(0, caps, MIN(sizeof(caps), packet->length));
                            break;
                        }
                        default:
                            pbdrv_usb_nxt_send_stall(0);
                            break;
                    }
                    break;
                default:
                    pbdrv_usb_nxt_send_stall(0);
                    break;
            }
            break;
        default:
            pbdrv_usb_nxt_send_stall(0);
            break;
    }
}

/* Handle receiving and responding to setup packets on EP0. */
static uint32_t pbdrv_usb_nxt_manage_setup_packet(void) {
    /* The structure of a USB setup packet. */
    pbdrv_usb_nxt_setup_packet_t packet;

    /* Read the packet from the FIFO into the above packet struct. */
    packet.request_attrs = AT91C_UDP_FDR[0];
    packet.request = AT91C_UDP_FDR[0];
    packet.value = (AT91C_UDP_FDR[0] & 0xFF) | (AT91C_UDP_FDR[0] << 8);
    packet.index = (AT91C_UDP_FDR[0] & 0xFF) | (AT91C_UDP_FDR[0] << 8);
    packet.length = (AT91C_UDP_FDR[0] & 0xFF) | (AT91C_UDP_FDR[0] << 8);

    if ((packet.request_attrs & USB_BMREQUEST_DIR) == USB_BMREQUEST_D_TO_H) {
        pbdrv_usb_nxt_csr_set_flag(0, AT91C_UDP_DIR); /* TODO: contradicts atmel doc p475 */
    }

    pbdrv_usb_nxt_csr_clear_flag(0, AT91C_UDP_RXSETUP);

    switch (packet.request_attrs & USB_BMREQUEST_TYPE) {
        case USB_BMREQUEST_TYPE_STD:
            pbdrv_usb_handle_std_request(&packet);
            break;
        case USB_BMREQUEST_TYPE_CLASS:
            pbdrv_usb_nxt_handle_class_request(&packet);
            break;
        case USB_BMREQUEST_TYPE_VENDOR:
            switch (packet.request) {
                case PBDRV_USB_VENDOR_REQ_WEBUSB:
                    // Since there is only one WebUSB descriptor, we ignore the index.
                    pbdrv_usb_nxt_write_data(0, &pbdrv_usb_webusb_landing_page,
                        MIN(pbdrv_usb_webusb_landing_page.s.bLength, packet.length));
                    break;
                case PBDRV_USB_VENDOR_REQ_MS_20:
                    // Since there is only one MS descriptor, we ignore the index.
                    pbdrv_usb_nxt_write_data(0, &pbdrv_usb_ms_20_desc_set,
                        MIN(sizeof(pbdrv_usb_ms_20_desc_set.s), packet.length));
                    break;
                default:
                    pbdrv_usb_nxt_send_stall(0);
                    break;
            }
            break;
        default:
            pbdrv_usb_nxt_send_stall(0);
            break;
    }

    return packet.request;
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
        pbdrv_usb_nxt_state.status = USB_UNINITIALIZED;

        /* Disable and clear all interruptions, reverting to the base
         * state.
         */
        *AT91C_UDP_IDR = ~0;
        *AT91C_UDP_ICR = ~0;

        /* Reset all endpoint FIFOs. */
        *AT91C_UDP_RSTEP = ~0;
        *AT91C_UDP_RSTEP = 0;

        /* Reset internal state. */
        pbdrv_usb_nxt_state.current_config = 0;

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
        pbdrv_usb_nxt_state.pre_suspend_status = pbdrv_usb_nxt_state.status;
        pbdrv_usb_nxt_state.status = USB_SUSPENDED;
    }

    if (isr & AT91C_UDP_RXRSM) {
        *AT91C_UDP_ICR = AT91C_UDP_RXRSM;
        isr &= ~AT91C_UDP_RXRSM;
        pbdrv_usb_nxt_state.status = pbdrv_usb_nxt_state.pre_suspend_status;
    }

    for (endpoint = 0; endpoint < PBDRV_USB_NXT_N_ENDPOINTS; endpoint++) {
        if (isr & (1 << endpoint)) {
            break;
        }
    }

    if (endpoint == 0) {

        if (AT91C_UDP_CSR[0] & AT91C_UDP_RXSETUP) {
            csr = pbdrv_usb_nxt_manage_setup_packet();
            return;
        }
    }

    if (endpoint < PBDRV_USB_NXT_N_ENDPOINTS) { /* if an endpoint was specified */
        csr = AT91C_UDP_CSR[endpoint];

        if (csr & AT91C_UDP_RX_DATA_BK0
            || csr & AT91C_UDP_RX_DATA_BK1) {

            if (endpoint == 1) {
                AT91C_UDP_CSR[1] &= ~AT91C_UDP_EPEDS;
                while (AT91C_UDP_CSR[1] & AT91C_UDP_EPEDS) {
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

            if (pbdrv_usb_nxt_state.new_device_address > 0) {
                /* the previous message received was SET_ADDR */
                /* now that the computer ACK our send_null(), we can
                 * set this address for real */

                /* we set the specified usb address in the controller */
                *AT91C_UDP_FADDR = AT91C_UDP_FEN | pbdrv_usb_nxt_state.new_device_address;
                /* and we tell the controller that we are in addressed mode now */
                *AT91C_UDP_GLBSTATE = AT91C_UDP_FADDEN;
                pbdrv_usb_nxt_state.new_device_address = 0;
            }

            /* and we will send the following data */
            if (pbdrv_usb_nxt_state.tx_data[endpoint] != NULL) {
                pbdrv_usb_nxt_write_data(endpoint, pbdrv_usb_nxt_state.tx_data[endpoint],
                    pbdrv_usb_nxt_state.tx_len[endpoint]);
            } else {
                /* then it means that we sent all the data and the host has acknowledged it */
                pbdrv_usb_nxt_state.status = USB_READY;
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
    pbdrv_usb_str_desc_serial.bLength = PBIO_ARRAY_SIZE(pbdrv_usb_str_desc_serial.wString) * 2;
    pbdrv_usb_str_desc_serial.bDescriptorType = USB_DESC_TYPE_STR,

    pbdrv_usb_nxt_deinit();
    memset((void *)&pbdrv_usb_nxt_state, 0, sizeof(pbdrv_usb_nxt_state));

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
    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_UNTIL(state, pbdrv_usb_nxt_state.status == USB_READY);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

bool pbdrv_usb_is_ready(void) {
    return pbdrv_usb_nxt_state.status != USB_UNINITIALIZED;
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

pbio_error_t pbdrv_usb_tx_event(pbio_os_state_t *state, const uint8_t *data, uint32_t size, bool cancel) {

    PBIO_OS_ASYNC_BEGIN(state);

    // REVISIT: Won't work if we include this check.
    // if (pbdrv_usb_nxt_state.status != USB_READY) {
    //     return PBIO_ERROR_BUSY;
    // }

    pbdrv_usb_nxt_write_data(2, data, size);

    PBIO_OS_AWAIT_UNTIL(state, pbdrv_usb_nxt_state.status == USB_READY || cancel);
    if (pbdrv_usb_nxt_state.status != USB_READY && cancel) {
        return PBIO_ERROR_CANCELED;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_usb_tx_response(pbio_os_state_t *state, pbio_pybricks_error_t code, bool cancel) {

    static uint8_t usb_response_buf[PBIO_PYBRICKS_USB_MESSAGE_SIZE(sizeof(uint32_t))] __aligned(4) = { PBIO_PYBRICKS_IN_EP_MSG_RESPONSE };

    PBIO_OS_ASYNC_BEGIN(state);

    // REVISIT: Won't work if we include this check.
    // if (pbdrv_usb_nxt_state.status != USB_READY) {
    //     return PBIO_ERROR_BUSY;
    // }

    pbio_set_uint32_le(&usb_response_buf[1], code);
    pbdrv_usb_nxt_write_data(2, usb_response_buf, sizeof(usb_response_buf));

    PBIO_OS_AWAIT_UNTIL(state, pbdrv_usb_nxt_state.status == USB_READY || cancel);
    if (pbdrv_usb_nxt_state.status != USB_READY && cancel) {
        return PBIO_ERROR_CANCELED;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state) {
    // REVISIT: Make async.
    pbdrv_usb_init_device();
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
    if (pbdrv_usb_nxt_state.status > USB_UNINITIALIZED
        && pbdrv_usb_nxt_state.status != USB_SUSPENDED) {
        AT91C_UDP_CSR[1] |= AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT;
    }

    return result;
}

#endif // PBDRV_CONFIG_USB_NXT
