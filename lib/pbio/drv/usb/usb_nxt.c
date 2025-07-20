// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2007 the NxOS developers
 * Copyright (C) 2025 the Pybricks Authors
 *
 * See AUTHORS for a full list of the developers.
 */

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_NXT

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>
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
static const uint8_t pbdrv_usb_nxt_device_descriptor[] = {
    18, USB_DESC_TYPE_DEVICE, /* Packet size and type. */
    0x10, 0x02, /* This packet is USB 2.1 (needed for BOS descriptors). */
    PBIO_PYBRICKS_USB_DEVICE_CLASS, /* Class code. */
    PBIO_PYBRICKS_USB_DEVICE_SUBCLASS, /* Sub class code. */
    PBIO_PYBRICKS_USB_DEVICE_PROTOCOL, /* Device protocol. */
    MAX_EP0_SIZE, /* Maximum packet size for EP0 (control endpoint). */
    0x94, 0x06, /* Vendor ID : LEGO */
    0x02, 0x00, /* Product ID : NXT */
    0x00, 0x02, /* Product revision: 2.0.0. */
    1, /* Index of the vendor string. */
    2, /* Index of the product string. */
    0, /* Index of the serial number (none for us). */
    1, /* The number of possible configurations. */
};

static const uint8_t pbdrv_usb_nxt_dev_qualifier_desc[] = {
    10, USB_DESC_TYPE_DEVICE_QUALIFIER, /* Packet size and type. */
    0x10, 0x02, /* This packet is USB 2.1. */
    PBIO_PYBRICKS_USB_DEVICE_CLASS, /* Class code */
    PBIO_PYBRICKS_USB_DEVICE_SUBCLASS, /* Sub class code */
    PBIO_PYBRICKS_USB_DEVICE_PROTOCOL, /* Device protocol */
    MAX_EP0_SIZE, /* Maximum packet size for EP0. */
    1, /* The number of possible configurations. */
    0, /* Reserved for future use, must be zero. */
};

// These enumerations are specific to the configuration of this device.

enum {
    PBDRV_USB_NXT_VENDOR_CODE_WEBUSB,
    PBDRV_USB_NXT_VENDOR_CODE_MS,
};

// NB: Chromium seems quite particular about the order of these descriptors.
// The WebUSB descriptor must come first and the MS OS 2.0 descriptor be last.
static const uint8_t pbdrv_usb_nxt_bos_desc[] = {
    5, USB_DESC_TYPE_BOS, /* Descriptor length and type. */
    0x39, 0x00, /* Total length of the descriptor = 57. */
    2, /* Number of device capabilities. */

    24,                               /* bLength */
    USB_DEVICE_CAPABILITY_TYPE,       /* bDescriptorType = Device Capability */
    USB_DEV_CAP_TYPE_PLATFORM,        /* bDevCapabilityType */
    0x00,                             /* bReserved */

    /*
      * PlatformCapabilityUUID
      * WebUSB Platform Capability descriptor
      * 3408B638-09A9-47A0-8BFD-A0768815B665
      * RFC 4122 explains the correct byte ordering
      */
    0x38, 0xB6, 0x08, 0x34,           /* 32-bit value */
    0xA9, 0x09,                       /* 16-bit value */
    0xA0, 0x47,                       /* 16-bit value */
    0x8B, 0xFD,
    0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,

    0x00, 0x01,                       /* bcdVersion = 1.00 */
    PBDRV_USB_NXT_VENDOR_CODE_WEBUSB,          /* bVendorCode */
    1,                                /* iLandingPage */

    28,                               /* bLength */
    USB_DEVICE_CAPABILITY_TYPE,       /* bDescriptorType = Device Capability */
    USB_DEV_CAP_TYPE_PLATFORM,        /* bDevCapabilityType */
    0x00,                             /* bReserved */

    /*
     * PlatformCapabilityUUID
     * Microsoft OS 2.0 descriptor platform capability ID
     * D8DD60DF-4589-4CC7-9CD2-659D9E648A9F
     * RFC 4122 explains the correct byte ordering
     */
    0xDF, 0x60, 0xDD, 0xD8,         /* 32-bit value */
    0x89, 0x45,                     /* 16-bit value */
    0xC7, 0x4C,                     /* 16-bit value */
    0x9C, 0xD2,
    0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,

    0x00, 0x00, 0x03, 0x06,         /* dwWindowsVersion = 0x06030000 for Windows 8.1 Build */
    0xA2, 0x00,                     /* wMSOSDescriptorSetTotalLength = 162 */
    PBDRV_USB_NXT_VENDOR_CODE_MS,            /* bMS_VendorCode */
    0x00,                           /* bAltEnumCode = Does not support alternate enumeration */
};

static const uint8_t pbdrv_usb_nxt_full_config[] = {
    0x09, USB_DESC_TYPE_CONFIG, /* Descriptor size and type. */
    0x20, 0x00, /* Total length of the configuration, interface
               * description included.
               */
    1, /* The number of interfaces declared by this configuration. */
    1, /* The ID for this configuration. */
    0, /* Index of the configuration description string (none). */

    /* Configuration attributes bitmap. Bit 7 (MSB) must be 1, bit 6 is
     * 1 because the NXT is self-powered, bit 5 is 0 because the NXT
     * doesn't support remote wakeup, and bits 0-4 are 0 (reserved).
     */
    0xC0,
    0, /* Device power consumption, for non self-powered devices. */

    /*
     * This is the descriptor for the interface associated with the
     * configuration.
     */
    0x09, USB_DESC_TYPE_INT, /* Descriptor size and type. */
    0x00, /* Interface index. */
    0x00, /* ID for this interface configuration. */
    0x02, /* The number of endpoints defined by this interface
         * (excluding EP0).
         */
    PBIO_PYBRICKS_USB_DEVICE_CLASS, /* Interface class ("Vendor specific"). */
    PBIO_PYBRICKS_USB_DEVICE_SUBCLASS, /* Interface subclass (see above). */
    PBIO_PYBRICKS_USB_DEVICE_PROTOCOL, /* Interface protocol (see above). */
    0x00, /* Index of the string descriptor for this interface (none). */

    /*
     * Descriptor for EP1.
     */
    7, USB_DESC_TYPE_ENDPT, /* Descriptor length and type. */
    0x1, /* Endpoint number. MSB is zero, meaning this is an OUT EP. */
    0x2, /* Endpoint type (bulk). */
    MAX_RCV_SIZE, 0x00, /* Maximum packet size (64). */
    0, /* EP maximum NAK rate (device never NAKs). */

    /*
     * Descriptor for EP2.
     */
    7, USB_DESC_TYPE_ENDPT, /* Descriptor length and type. */
    0x82, /* Endpoint number. MSB is one, meaning this is an IN EP. */
    0x2, /* Endpoint type (bulk). */
    MAX_RCV_SIZE, 0x00, /* Maximum packet size (64). */
    0, /* EP maximum NAK rate (device never NAKs). */
};

static const uint8_t pbdrv_usb_nxt_string_desc[] = {
    4, USB_DESC_TYPE_STR, /* Descriptor length and type. */
    0x09, 0x04, /* Supported language ID (US English). */
};

static const uint8_t pbdrv_usb_lego_str[] = {
    10, USB_DESC_TYPE_STR,
    'L', 0,
    'E', 0,
    'G', 0,
    'O', 0,
};

static const uint8_t pbdrv_usb_nxt_str[] = {
    30, USB_DESC_TYPE_STR,
    'N', 0,
    'X', 0,
    'T', 0,
    ' ', 0,
    '+', 0,
    ' ', 0,
    'P', 0,
    'y', 0,
    'b', 0,
    'r', 0,
    'i', 0,
    'c', 0,
    'k', 0,
    's', 0,
};

/* Internal lookup table mapping string descriptors to their indices
 * in the USB string descriptor table.
 */
static const uint8_t *pbdrv_usb_nxt_strings[] = {
    pbdrv_usb_lego_str,
    pbdrv_usb_nxt_str,
};

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

    /* Used to write the data from the EP1
     */
    uint8_t *rx_data;

    /* size of the rx data buffer */
    uint32_t rx_size;

    /* length of the read packet (0 if none) */
    uint32_t rx_len;

    /* The USB controller has two hardware input buffers. This remembers
     * the one currently in use.
     */
    uint8_t current_rx_bank;
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
static void pbdrv_usb_nxt_write_data(int endpoint, const uint8_t *ptr, uint32_t length) {
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
        pbdrv_usb_nxt_state.tx_data[tx] = NULL;
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

/* Read one data packet from the USB controller.
 * Assume that pbdrv_usb_nxt_state.rx_data and pbdrv_usb_nxt_state.rx_len are set.
 */
static void pbdrv_usb_nxt_read_data(int endpoint) {
    uint16_t i;
    uint16_t total;

    /* Given our configuration, we should only be getting packets on
     * endpoint 1. Ignore data on any other endpoint.
     * (note: data from EP0 are managed by usb_manage_setup())
     */
    if (endpoint != 1) {
        pbdrv_usb_nxt_csr_clear_flag(endpoint, AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1);
        return;
    }

    /* must not happen ! */
    if (pbdrv_usb_nxt_state.rx_len > 0) {
        return;
    }

    total = (AT91C_UDP_CSR[endpoint] & AT91C_UDP_RXBYTECNT) >> 16;

    /* we start reading */
    /* all the bytes will be put in rx_data */
    for (i = 0;
         i < total && i < pbdrv_usb_nxt_state.rx_size;
         i++) {
        pbdrv_usb_nxt_state.rx_data[i] = AT91C_UDP_FDR[1];
    }

    pbdrv_usb_nxt_state.rx_len = i;

    /* if we have read all the byte ... */
    if (i == total) {
        /* Acknowledge reading the current RX bank, and switch to the other. */
        pbdrv_usb_nxt_csr_clear_flag(1, pbdrv_usb_nxt_state.current_rx_bank);
        if (pbdrv_usb_nxt_state.current_rx_bank == AT91C_UDP_RX_DATA_BK0) {
            pbdrv_usb_nxt_state.current_rx_bank = AT91C_UDP_RX_DATA_BK1;
        } else {
            pbdrv_usb_nxt_state.current_rx_bank = AT91C_UDP_RX_DATA_BK0;
        }
    }
    /* else we let the interruption running :
     * after this function, the interruption should be disabled until
     * a new buffer to read is provided */
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

static const uint8_t pbdrv_usb_desc_set_ms_os[] = {
    0x0A, 0x00,                       /* wLength = 10 */
    0x00, 0x00,                       /* wDescriptorType = MS_OS_20_SET_HEADER_DESCRIPTOR */
    0x00, 0x00, 0x03, 0x06,           /* dwWindowsVersion = 0x06030000 for Windows 8.1 Build */
    0xA2, 0x00,                       /* wTotalLength = 162 */

    0x14, 0x00,                       /* wLength = 20 */
    0x03, 0x00,                       /* wDescriptorType = MS_OS_20_FEATURE_COMPATBLE_ID */
    'W', 'I', 'N', 'U', 'S', 'B',     /* CompatibleID */
    0x00, 0x00,                       /* CompatibleID (cont.) */
    0x00, 0x00, 0x00, 0x00,           /* SubCompatibleID */
    0x00, 0x00, 0x00, 0x00,           /* SubCompatibleID (cont.) */

    0x84, 0x00,                       /* wLength = 132 */
    0x04, 0x00,                       /* wDescriptorType = MS_OS_20_FEATURE_REG_PROPERTY */
    0x07, 0x00,                       /* wStringType = REG_MULTI_SZ */
    /* wPropertyNameLength = 42 */
    0x2A, 0x00,
    /* PropertyName = DeviceInterfaceGUIDs */
    'D', '\0',
    'e', '\0',
    'v', '\0',
    'i', '\0',
    'c', '\0',
    'e', '\0',
    'I', '\0',
    'n', '\0',
    't', '\0',
    'e', '\0',
    'r', '\0',
    'f', '\0',
    'a', '\0',
    'c', '\0',
    'e', '\0',
    'G', '\0',
    'U', '\0',
    'I', '\0',
    'D', '\0',
    's', '\0',
    '\0', '\0',

    /* wPropertyDataLength = 80 */
    0x50, 0x00,
    /* PropertyData = {A5C44A4C-53D4-4389-9821-AE95051908A1} */
    '{', '\0',
    'A', '\0',
    '5', '\0',
    'C', '\0',
    '4', '\0',
    '4', '\0',
    'A', '\0',
    '4', '\0',
    'C', '\0',
    '-', '\0',
    '5', '\0',
    '3', '\0',
    'D', '\0',
    '4', '\0',
    '-', '\0',
    '4', '\0',
    '3', '\0',
    '8', '\0',
    '9', '\0',
    '-', '\0',
    '9', '\0',
    '8', '\0',
    '2', '\0',
    '1', '\0',
    '-', '\0',
    'A', '\0',
    'E', '\0',
    '9', '\0',
    '5', '\0',
    '0', '\0',
    '5', '\0',
    '1', '\0',
    '9', '\0',
    '0', '\0',
    '8', '\0',
    'A', '\0',
    '1', '\0',
    '}', '\0',
    '\0', '\0',
    '\0', '\0',
};

static const uint8_t pbdrv_usb_desc_set_webusb[] = {
    20,    /* bLength */
    0x03,  /* bDescriptorType = URL */
    0x01,  /* bScheme = https:// */

    /* URL */
    #if PBIO_VERSION_LEVEL_HEX == 0xA
    'a', 'l', 'p', 'h', 'a',
    #elif PBIO_VERSION_LEVEL_HEX == 0xB
    'b', 'e', 't', 'a',
    #else
    'c', 'o', 'd', 'e',
    #endif
    '.', 'p', 'y', 'b', 'r', 'i', 'c', 'k', 's', '.', 'c', 'o', 'm',
};

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

            pbdrv_usb_nxt_write_data(0, (uint8_t *)&response, 2);
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
                    size = pbdrv_usb_nxt_device_descriptor[0];
                    pbdrv_usb_nxt_write_data(0, pbdrv_usb_nxt_device_descriptor,
                        MIN(size, packet->length));
                    break;

                case USB_DESC_TYPE_CONFIG: /* Configuration descriptor */
                    pbdrv_usb_nxt_write_data(0, pbdrv_usb_nxt_full_config,
                        MIN(pbdrv_usb_nxt_full_config[2], packet->length));

                    /* TODO: Why? This is not specified in the USB specs. */
                    if (pbdrv_usb_nxt_full_config[2] < packet->length) {
                        pbdrv_usb_nxt_send_null();
                    }
                    break;

                case USB_DESC_TYPE_STR: /* String or language info. */
                    if ((packet->value & USB_WVALUE_INDEX) == 0) {
                        pbdrv_usb_nxt_write_data(0, pbdrv_usb_nxt_string_desc,
                            MIN(pbdrv_usb_nxt_string_desc[0], packet->length));
                    } else {
                        /* The host wants a specific string. */
                        /* TODO: This should check if the requested string exists. */
                        pbdrv_usb_nxt_write_data(0, pbdrv_usb_nxt_strings[index - 1],
                            MIN(pbdrv_usb_nxt_strings[index - 1][0],
                                packet->length));
                    }
                    break;

                case USB_DESC_TYPE_DEVICE_QUALIFIER: /* Device qualifier descriptor. */
                    size = pbdrv_usb_nxt_dev_qualifier_desc[0];
                    pbdrv_usb_nxt_write_data(0, pbdrv_usb_nxt_dev_qualifier_desc,
                        MIN(size, packet->length));
                    break;

                case USB_DESC_TYPE_BOS: /* BOS descriptor */
                    size = pbdrv_usb_nxt_bos_desc[2];
                    pbdrv_usb_nxt_write_data(0, pbdrv_usb_nxt_bos_desc, MIN(size, packet->length));
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

            /* we can only active the EP1 if we have a buffer to get the data */
            /* TODO: This was:
             *
             * if (pbdrv_usb_nxt_state.rx_len == 0 && pbdrv_usb_nxt_state.rx_size >= 0) {
             *
             * The second part always evaluates to true.
             */
            if (pbdrv_usb_nxt_state.rx_len == 0) {
                AT91C_UDP_CSR[1] = AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT;
                while (AT91C_UDP_CSR[1] != (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT)) {
                    ;
                }
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
                case 0x01: // Standard GATT characteristic
                    switch (packet->value) {
                        case 0x2A00: { // device name
                            const char *name = pbdrv_bluetooth_get_hub_name();
                            pbdrv_usb_nxt_write_data(0, (const uint8_t *)name,
                                MIN(strlen(name), packet->length));
                            break;
                        }
                        case 0x2A26: { // firmware revision
                            const char *fw = PBIO_VERSION_STR;
                            pbdrv_usb_nxt_write_data(0, (const uint8_t *)fw,
                                MIN(strlen(fw), packet->length));
                            break;
                        }
                        case 0x2A28: { // software revision
                            const char *sw = PBIO_PROTOCOL_VERSION_STR;
                            pbdrv_usb_nxt_write_data(0, (const uint8_t *)sw,
                                MIN(strlen(sw), packet->length));
                            break;
                        }
                        default:
                            pbdrv_usb_nxt_send_stall(0);
                            break;
                    }
                    break;
                case 0x02: // Pybricks characteristic
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
                case PBDRV_USB_NXT_VENDOR_CODE_WEBUSB:
                    // Since there is only one WebUSB descriptor, we ignore the index.
                    pbdrv_usb_nxt_write_data(0, pbdrv_usb_desc_set_webusb,
                        MIN(sizeof(pbdrv_usb_desc_set_webusb), packet.length));
                    break;
                case PBDRV_USB_NXT_VENDOR_CODE_MS:
                    // Since there is only one MS descriptor, we ignore the index.
                    pbdrv_usb_nxt_write_data(0, pbdrv_usb_desc_set_ms_os,
                        MIN(sizeof(pbdrv_usb_desc_set_ms_os), packet.length));
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
        pbdrv_usb_nxt_state.current_rx_bank = AT91C_UDP_RX_DATA_BK0;
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

            pbdrv_usb_nxt_read_data(endpoint);

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
            if (pbdrv_usb_nxt_state.tx_len[endpoint] > 0
                && pbdrv_usb_nxt_state.tx_data[endpoint] != NULL) {
                pbdrv_usb_nxt_write_data(endpoint, pbdrv_usb_nxt_state.tx_data[endpoint],
                    pbdrv_usb_nxt_state.tx_len[endpoint]);
            } else {
                /* then it means that we sent all the data and the host has acknowledged it */
                pbdrv_usb_nxt_state.status = USB_READY;
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

void pbdrv_usb_init(void) {
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

bool nx_usb_can_write(void) {
    return pbdrv_usb_nxt_state.status == USB_READY;
}

void nx_usb_write(uint8_t *data, uint32_t length) {
    NX_ASSERT_MSG(pbdrv_usb_nxt_state.status != USB_UNINITIALIZED,
        "USB not init");
    NX_ASSERT_MSG(pbdrv_usb_nxt_state.status != USB_SUSPENDED,
        "USB asleep");
    NX_ASSERT(data != NULL);
    NX_ASSERT(length > 0);

    /* TODO: Make call asynchronous */
    while (pbdrv_usb_nxt_state.status != USB_READY) {
        ;
    }

    /* start sending the data */
    pbdrv_usb_nxt_write_data(2, data, length);
}

bool nx_usb_data_written(void) {
    return pbdrv_usb_nxt_state.tx_len[1] == 0;
}

bool nx_usb_is_connected(void) {
    return pbdrv_usb_nxt_state.status != USB_UNINITIALIZED;
}

void nx_usb_read(uint8_t *data, uint32_t length) {
    pbdrv_usb_nxt_state.rx_data = data;
    pbdrv_usb_nxt_state.rx_size = length;
    pbdrv_usb_nxt_state.rx_len = 0;

    if (pbdrv_usb_nxt_state.status > USB_UNINITIALIZED
        && pbdrv_usb_nxt_state.status != USB_SUSPENDED) {
        AT91C_UDP_CSR[1] |= AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT;
    }
}

uint32_t nx_usb_data_read(void) {
    return pbdrv_usb_nxt_state.rx_len;
}

#endif // PBDRV_CONFIG_USB_NXT
