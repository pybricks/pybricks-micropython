// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// EV3 / TI AM1808 / Mentor Graphics MUSBMHDRC driver
// implementing a bespoke USB stack for Pybricks USB protocol

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_EV3

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/cache.h>
#include <pbdrv/compiler.h>
#include <pbdrv/usb.h>
#include <pbio/os.h>
#include <pbio/protocol.h>
#include <pbio/util.h>
#include <pbio/version.h>
#include <pbsys/command.h>
#include <pbsys/config.h>
#include <pbsys/status.h>
#include <pbsys/storage.h>

#include <lego/usb.h>

#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/cppi41dma.h>
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
#include "usb_common_desc.h"

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
            .bmAttributes = PBDRV_USB_EP_TYPE_BULK,
            .wMaxPacketSize = PYBRICKS_EP_PKT_SZ_HS,
            .bInterval = 0,
        },
        .ep_1_in = {
            .bLength = sizeof(pbdrv_usb_ep_desc_t),
            .bDescriptorType = DESC_TYPE_ENDPOINT,
            .bEndpointAddress = 0x81,
            .bmAttributes = PBDRV_USB_EP_TYPE_BULK,
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
            .bmAttributes = PBDRV_USB_EP_TYPE_BULK,
            .wMaxPacketSize = PYBRICKS_EP_PKT_SZ_FS,
            .bInterval = 0,
        },
        .ep_1_in = {
            .bLength = sizeof(pbdrv_usb_ep_desc_t),
            .bDescriptorType = DESC_TYPE_ENDPOINT,
            .bEndpointAddress = 0x81,
            .bmAttributes = PBDRV_USB_EP_TYPE_BULK,
            .wMaxPacketSize = PYBRICKS_EP_PKT_SZ_FS,
            .bInterval = 0,
        },
    }
};

// This dynamic buffer is needed in order to have an aligned,
// global-lifetime buffer for sending dynamic data in response
// to control transfers. This is used for the serial number string
// and for Pybricks protocol requests.
static union {
    uint8_t b[EP0_BUF_SZ];
    uint32_t u[EP0_BUF_SZ / sizeof(uint32_t)];
} pbdrv_usb_ev3_ep0_buffer;
_Static_assert(PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE <= EP0_BUF_SZ);

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

// Whether there is a host app listening to events
static bool pbdrv_usb_is_events_subscribed;

// Buffers, used for different logical flows on the data endpoint
static uint8_t ep1_rx_buf[PYBRICKS_EP_PKT_SZ_HS] PBDRV_DMA_BUF;
static uint8_t ep1_tx_response_buf[PYBRICKS_EP_PKT_SZ_HS];
static uint8_t ep1_tx_status_buf[PYBRICKS_EP_PKT_SZ_HS];
static uint8_t ep1_tx_stdout_buf[PYBRICKS_EP_PKT_SZ_HS];
static uint32_t ep1_tx_stdout_sz;

// Buffer status flags
static volatile bool usb_rx_is_ready;
static volatile bool transmitting;

// CPPI DMA support code

// Descriptors must be aligned to a power of 2 greater than or equal to their size.
// We are using a descriptor of 32 bytes which is also the required alignment.
#define CPPI_DESCRIPTOR_ALIGN   32

// Host Packet Descriptor
// The TI support library has hardcoded assumptions about the layout of these structures,
// so we declare it ourselves here in order to control it as we wish.
typedef struct {
    hPDWord0 word0;
    hPDWord1 word1;
    hPDWord2 word2;
    uint32_t buf_len;
    void *buf_ptr;
    void *next_desc_ptr;
    uint32_t orig_buf_len;
    void *orig_buf_ptr;
} __attribute__((aligned(CPPI_DESCRIPTOR_ALIGN))) usb_cppi_hpd_t;
_Static_assert(sizeof(usb_cppi_hpd_t) <= CPPI_DESCRIPTOR_ALIGN);

// This goes into the lower bits of the queue CTRLD register
#define CPPI_DESCRIPTOR_SIZE_BITS   CPDMA_QUEUE_REGISTER_DESC_SIZE(usb_cppi_hpd_t)

// We only use a hardcoded descriptor for each logical flow,
// rather than dynamically allocating them as needed
enum {
    CPPI_DESC_RX,
    CPPI_DESC_TX_RESPONSE,
    CPPI_DESC_TX_PYBRICKS_EVENT,
    // the minimum number of descriptors we can allocate is 32,
    // even though we do not use nearly all of them
    CPPI_DESC_COUNT = 32,
};

enum {
    // Documenting explicitly that we only use RX queue 0
    // (out of 16 total which are supported by the hardware)
    CPPI_RX_SUBMIT_QUEUE = 0,
};

// CPPI memory
static usb_cppi_hpd_t cppi_descriptors[CPPI_DESC_COUNT];
static uint32_t cppi_linking_ram[CPPI_DESC_COUNT];
// Tags a Host Packet Descriptor (i.e. the first descriptor
// which contains full information about a packet, rather than
// a Host Buffer Descriptor containing only an additional buffer).
#define CPPI_HOST_PACKET_DESCRIPTOR_TYPE    0x10

// Fill in the CPPI DMA descriptor to receive a packet
static void usb_setup_rx_dma_desc(void) {
    PBDRV_UNCACHED(cppi_descriptors[CPPI_DESC_RX]) = (usb_cppi_hpd_t) {
        .word0 = {
            .hostPktType = CPPI_HOST_PACKET_DESCRIPTOR_TYPE,
        },
        .word1 = {},
        .word2 = {
            .pktRetQueue = RX_COMPQ1,
        },
        .buf_len = PYBRICKS_EP_PKT_SZ_HS,
        .buf_ptr = ep1_rx_buf,
        .next_desc_ptr = 0,
        .orig_buf_len = PYBRICKS_EP_PKT_SZ_HS,
        .orig_buf_ptr = ep1_rx_buf,
    };

    pbdrv_compiler_memory_barrier();

    HWREG(USB_0_OTGBASE + CPDMA_QUEUE_REGISTER_D + CPPI_RX_SUBMIT_QUEUE * 16) =
        (uint32_t)(&cppi_descriptors[CPPI_DESC_RX]) | CPPI_DESCRIPTOR_SIZE_BITS;
}


// Fill in the CPPI DMA descriptor to send a packet
static void usb_setup_tx_dma_desc(int tx_type, void *buf, uint32_t buf_len) {
    PBDRV_UNCACHED(cppi_descriptors[tx_type]) = (usb_cppi_hpd_t) {
        .word0 = {
            .hostPktType = CPPI_HOST_PACKET_DESCRIPTOR_TYPE,
            .pktLength = buf_len,
        },
        .word1 = {
            .srcPrtNum = 1,             // port is EP1
        },
        .word2 = {
            .pktType = 5,               // USB packet type
            .pktRetQueue = TX_COMPQ1,
        },
        .buf_len = buf_len,
        .buf_ptr = buf,
        .next_desc_ptr = 0,
        .orig_buf_len = buf_len,
        .orig_buf_ptr = buf,
    };

    pbdrv_compiler_memory_barrier();

    HWREG(USB_0_OTGBASE + CPDMA_QUEUE_REGISTER_D + TX_SUBMITQ1 * 16) =
        (uint32_t)(&cppi_descriptors[tx_type]) | CPPI_DESCRIPTOR_SIZE_BITS;
}

// Helper function to set up CPPI DMA upon USB reset
static void usb_reset_cppi_dma(void) {
    // Set up the FIFOs
    // We use a hardcoded address allocation as follows
    // @ 0      ==> EP0
    // @ 64     ==> EP1 IN (device to host, tx)
    // @ 64+512 ==> EP1 OUT (host to device, rx)
    HWREGB(USB0_BASE + USB_O_EPIDX) = 1;
    if (pbdrv_usb_is_usb_hs) {
        HWREGB(USB0_BASE + USB_O_TXFIFOSZ) = USB_TXFIFOSZ_SIZE_512;
        HWREGB(USB0_BASE + USB_O_RXFIFOSZ) = USB_RXFIFOSZ_SIZE_512;
        HWREGH(USB0_BASE + USB_O_TXMAXP1) = PYBRICKS_EP_PKT_SZ_HS;
        HWREGH(USB0_BASE + USB_O_RXMAXP1) = PYBRICKS_EP_PKT_SZ_HS;
    } else {
        HWREGB(USB0_BASE + USB_O_TXFIFOSZ) = USB_TXFIFOSZ_SIZE_64;
        HWREGB(USB0_BASE + USB_O_RXFIFOSZ) = USB_RXFIFOSZ_SIZE_64;
        HWREGH(USB0_BASE + USB_O_TXMAXP1) = PYBRICKS_EP_PKT_SZ_FS;
        HWREGH(USB0_BASE + USB_O_RXMAXP1) = PYBRICKS_EP_PKT_SZ_FS;
    }
    HWREGH(USB0_BASE + USB_O_TXFIFOADD) = EP0_BUF_SZ / 8;
    HWREGH(USB0_BASE + USB_O_RXFIFOADD) = (EP0_BUF_SZ + PYBRICKS_EP_PKT_SZ_HS) / 8;

    // Set up the TX fifo for DMA and a stall condition
    HWREGH(USB0_BASE + USB_O_TXCSRL1) = ((USB_TXCSRH1_AUTOSET | USB_TXCSRH1_MODE | USB_TXCSRH1_DMAEN | USB_TXCSRH1_DMAMOD) << 8) | USB_TXCSRL1_STALL;
    // Set up the RX fifo for DMA and a stall condition
    HWREGH(USB0_BASE + USB_O_RXCSRL1) = ((USB_RXCSRH1_AUTOCL | USB_RXCSRH1_DMAEN) << 8) | USB_RXCSRL1_STALL;

    // Set up CPPI DMA
    HWREG(USB_0_OTGBASE + CPDMA_LRAM_0_BASE) = (uint32_t)cppi_linking_ram;
    HWREG(USB_0_OTGBASE + CPDMA_LRAM_0_SIZE) = CPPI_DESC_COUNT;
    HWREG(USB_0_OTGBASE + CPDMA_LRAM_1_BASE) = 0;

    HWREG(USB_0_OTGBASE + CPDMA_QUEUEMGR_REGION_0) = (uint32_t)cppi_descriptors;
    // 32 descriptors of 32 bytes each
    HWREG(USB_0_OTGBASE + CPDMA_QUEUEMGR_REGION_0_CONTROL) = 0;

    // scheduler table: RX on 0, TX on 0
    HWREG(USB_0_OTGBASE + CPDMA_SCHED_TABLE_0) =
        ((CPDMA_SCHED_TX | 0) << CPDMA_SCHED_ENTRY_SHIFT) |
        ((CPDMA_SCHED_RX | 0) << 0);
    HWREG(USB_0_OTGBASE + CPDMA_SCHED_CONTROL_REG) = (1 << SCHEDULER_ENABLE_SHFT) | (2 - 1);

    // CPPI RX
    HWREG(USB_0_OTGBASE + CPDMA_RX_CHANNEL_REG_A) =
        (CPPI_RX_SUBMIT_QUEUE << 0) |
        (CPPI_RX_SUBMIT_QUEUE << 16);
    HWREG(USB_0_OTGBASE + CPDMA_RX_CHANNEL_REG_B) =
        (CPPI_RX_SUBMIT_QUEUE << 0) |
        (CPPI_RX_SUBMIT_QUEUE << 16);
    HWREG(USB_0_OTGBASE + CPDMA_RX_CHANNEL_CONFIG_REG) =
        CPDMA_RX_GLOBAL_CHAN_CFG_ENABLE |
        CPDMA_RX_GLOBAL_CHAN_CFG_ERR_RETRY |    // starvation = retry
        CPDMA_RX_GLOBAL_CHAN_CFG_DESC_TY |      // "host" descriptors (the only valid type)
        RX_COMPQ1;

    // CPPI TX
    HWREG(USB_0_OTGBASE + CPDMA_TX_CHANNEL_CONFIG_REG) =
        CPDMA_TX_GLOBAL_CHAN_CFG_ENABLE |
        TX_COMPQ1;
}

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
                    pbdrv_usb_setup_data_to_send = pbdrv_usb_str_desc_langid.u;
                    pbdrv_usb_setup_data_to_send_sz = sizeof(pbdrv_usb_str_desc_langid.s);
                    return true;

                case STRING_DESC_MFG:
                    pbdrv_usb_setup_data_to_send = pbdrv_usb_str_desc_mfg.u;
                    pbdrv_usb_setup_data_to_send_sz = sizeof(pbdrv_usb_str_desc_mfg.s);
                    return true;

                case STRING_DESC_PRODUCT:
                    pbdrv_usb_setup_data_to_send = pbdrv_usb_str_desc_prod.u;
                    pbdrv_usb_setup_data_to_send_sz = sizeof(pbdrv_usb_str_desc_prod.s);
                    return true;

                case STRING_DESC_SERIAL:
                    pbdrv_usb_ev3_ep0_buffer.b[0] = 2 * 2 * 6 + 2;
                    pbdrv_usb_ev3_ep0_buffer.b[1] = DESC_TYPE_STRING;
                    for (i = 0; i < 6; i++) {
                        pbdrv_usb_ev3_ep0_buffer.b[2 + 4 * i + 0] = "0123456789ABCDEF"[pbdrv_ev3_bluetooth_mac_address[i] >> 4];
                        pbdrv_usb_ev3_ep0_buffer.b[2 + 4 * i + 1] = 0;
                        pbdrv_usb_ev3_ep0_buffer.b[2 + 4 * i + 2] = "0123456789ABCDEF"[pbdrv_ev3_bluetooth_mac_address[i] & 0xf];
                        pbdrv_usb_ev3_ep0_buffer.b[2 + 4 * i + 3] = 0;
                    }

                    pbdrv_usb_setup_data_to_send = pbdrv_usb_ev3_ep0_buffer.u;
                    pbdrv_usb_setup_data_to_send_sz = pbdrv_usb_ev3_ep0_buffer.b[0];
                    return true;
            }
            break;

        case DESC_TYPE_BOS:
            pbdrv_usb_setup_data_to_send = pbdrv_usb_bos_desc_set.u;
            pbdrv_usb_setup_data_to_send_sz = sizeof(pbdrv_usb_bos_desc_set.s);
            return true;
    }

    return false;
}

static void usb_device_intr(void) {
    IntSystemStatusClear(SYS_INT_USB0);
    uint32_t intr_src = HWREG(USB_0_OTGBASE + USB_0_INTR_SRC);

    if (intr_src & USBOTG_INTR_DISCON) {
        // USB cable disconnected

        // Mark config and address as 0 for main loop to detect
        pbdrv_usb_addr = 0;
        pbdrv_usb_config = 0;
        pbio_os_request_poll();
    }

    if (intr_src & USBOTG_INTR_RESET) {
        // USB reset

        // Reset state variables
        pbdrv_usb_addr = 0;
        pbdrv_usb_config = 0;
        pbdrv_usb_setup_data_to_send = 0;
        pbdrv_usb_addr_needs_setting = false;

        if (HWREGB(USB0_BASE + USB_O_POWER) & USB_POWER_HSMODE) {
            pbdrv_usb_is_usb_hs = true;
        } else {
            pbdrv_usb_is_usb_hs = false;
        }

        // Set up all the CPPI DMA registers
        usb_reset_cppi_dma();

        // queue RX descriptor
        usb_setup_rx_dma_desc();
    }

    if (intr_src & USBOTG_INTR_EP0) {
        // USB EP0
        uint16_t peri_csr = HWREGH(USB0_BASE + USB_O_CSRL0);

        if (peri_csr & USB_CSRL0_STALLED) {
            // If this is a sent-stall confirmation, clear the bit
            HWREGH(USB0_BASE + USB_O_CSRL0) = 0;
            pbdrv_usb_setup_data_to_send = 0;
            pbdrv_usb_addr_needs_setting = false;
        }

        if (peri_csr & USB_CSRL0_SETEND) {
            // Error in SETUP transaction
            HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_SETENDC;
            pbdrv_usb_setup_data_to_send = 0;
            pbdrv_usb_addr_needs_setting = false;
        }

        // If we got here (and didn't wipe out this flag),
        // then this indicates completion of the SET_ADDRESS command.
        // We thus have to make it take effect at this point.
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

            switch (setup_pkt.s.bmRequestType & BM_REQ_TYPE_MASK) {
                case BM_REQ_TYPE_STANDARD:
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

                                            // Reset data toggle, clear stall, flush fifo
                                            HWREGB(USB0_BASE + USB_O_TXCSRL1) = USB_TXCSRL1_CLRDT | USB_TXCSRL1_FLUSH;
                                            HWREGB(USB0_BASE + USB_O_RXCSRL1) = USB_RXCSRL1_CLRDT | USB_RXCSRL1_FLUSH;
                                        } else {
                                            // deconfiguring

                                            // Set stall condition
                                            HWREGB(USB0_BASE + USB_O_TXCSRL1) = USB_TXCSRL1_STALL;
                                            HWREGB(USB0_BASE + USB_O_RXCSRL1) = USB_RXCSRL1_STALL;
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
                                    pbdrv_usb_setup_misc_tx_byte = 1; // self-powered
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

                        case BM_REQ_RECIP_EP:
                            switch (setup_pkt.s.bRequest) {
                                case GET_STATUS:
                                    if (setup_pkt.s.wIndex == 1) {
                                        pbdrv_usb_setup_misc_tx_byte = !!(HWREGB(USB0_BASE + USB_O_RXCSRL1) & USB_RXCSRL1_STALL);
                                        pbdrv_usb_setup_data_to_send = &pbdrv_usb_setup_misc_tx_byte;
                                        pbdrv_usb_setup_data_to_send_sz = 2;
                                        handled = true;
                                    } else if (setup_pkt.s.wIndex == 0x81) {
                                        pbdrv_usb_setup_misc_tx_byte = !!(HWREGB(USB0_BASE + USB_O_TXCSRL1) & USB_TXCSRL1_STALL);
                                        pbdrv_usb_setup_data_to_send = &pbdrv_usb_setup_misc_tx_byte;
                                        pbdrv_usb_setup_data_to_send_sz = 2;
                                        handled = true;
                                    }
                                    break;

                                case CLEAR_FEATURE:
                                    if (setup_pkt.s.wValue == 0) {
                                        // Clear the endpoint halt, which also resets the data toggle value
                                        if (setup_pkt.s.wIndex == 1) {
                                            HWREGB(USB0_BASE + USB_O_RXCSRL1) = (HWREGB(USB0_BASE + USB_O_RXCSRL1) & ~USB_RXCSRL1_STALL) | USB_RXCSRL1_CLRDT;
                                            handled = true;
                                        } else if (setup_pkt.s.wIndex == 0x81) {
                                            HWREGB(USB0_BASE + USB_O_TXCSRL1) = (HWREGB(USB0_BASE + USB_O_TXCSRL1) & ~USB_TXCSRL1_STALL) | USB_TXCSRL1_CLRDT;
                                            handled = true;
                                        }
                                    }
                                    break;

                                case SET_FEATURE:
                                    if (setup_pkt.s.wValue == 0) {
                                        if (setup_pkt.s.wIndex == 1) {
                                            HWREGB(USB0_BASE + USB_O_RXCSRL1) |= USB_RXCSRL1_STALL;
                                            handled = true;
                                        } else if (setup_pkt.s.wIndex == 0x81) {
                                            HWREGB(USB0_BASE + USB_O_TXCSRL1) |= USB_TXCSRL1_STALL;
                                            handled = true;
                                        }
                                    }
                                    break;
                            }
                            break;
                    }
                    break;

                case BM_REQ_TYPE_VENDOR:
                    switch (setup_pkt.s.bRequest) {
                        case PBDRV_USB_VENDOR_REQ_WEBUSB:
                            if (setup_pkt.s.wIndex == WEBUSB_REQ_GET_URL && setup_pkt.s.wValue == PBDRV_USB_WEBUSB_LANDING_PAGE_URL_IDX) {
                                pbdrv_usb_setup_data_to_send = pbdrv_usb_webusb_landing_page.u;
                                pbdrv_usb_setup_data_to_send_sz = pbdrv_usb_webusb_landing_page.s.bLength;
                                handled = true;
                            }
                            break;

                        case PBDRV_USB_VENDOR_REQ_MS_20:
                            if (setup_pkt.s.wIndex == MS_OS_20_DESCRIPTOR_INDEX) {
                                pbdrv_usb_setup_data_to_send = pbdrv_usb_ms_20_desc_set.u;
                                pbdrv_usb_setup_data_to_send_sz = sizeof(pbdrv_usb_ms_20_desc_set.s);
                                handled = true;
                            }
                            break;
                    }
                    break;

                case BM_REQ_TYPE_CLASS:
                    if ((setup_pkt.s.bmRequestType & BM_REQ_RECIP_MASK) != BM_REQ_RECIP_IF) {
                        // Pybricks class requests must be directed at the interface
                        break;
                    }

                    switch (setup_pkt.s.bRequest) {
                        const char *s;

                        case PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_GATT:
                            // Standard GATT characteristic
                            switch (setup_pkt.s.wValue) {
                                case 0x2A00:
                                    // GATT Device Name characteristic
                                    s = pbdrv_bluetooth_get_hub_name();
                                    pbdrv_usb_setup_data_to_send_sz = strlen(s);
                                    memcpy(pbdrv_usb_ev3_ep0_buffer.b, s, pbdrv_usb_setup_data_to_send_sz);
                                    pbdrv_usb_setup_data_to_send = pbdrv_usb_ev3_ep0_buffer.u;
                                    handled = true;
                                    break;

                                case 0x2A26:
                                    // GATT Firmware Revision characteristic
                                    s = PBIO_VERSION_STR;
                                    pbdrv_usb_setup_data_to_send_sz = strlen(s);
                                    memcpy(pbdrv_usb_ev3_ep0_buffer.b, s, pbdrv_usb_setup_data_to_send_sz);
                                    pbdrv_usb_setup_data_to_send = pbdrv_usb_ev3_ep0_buffer.u;
                                    handled = true;
                                    break;

                                case 0x2A28:
                                    // GATT Software Revision characteristic
                                    s = PBIO_PROTOCOL_VERSION_STR;
                                    pbdrv_usb_setup_data_to_send_sz = strlen(s);
                                    memcpy(pbdrv_usb_ev3_ep0_buffer.b, s, pbdrv_usb_setup_data_to_send_sz);
                                    pbdrv_usb_setup_data_to_send = pbdrv_usb_ev3_ep0_buffer.u;
                                    handled = true;
                                    break;
                            }
                            break;

                        case PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_PYBRICKS:
                            // Pybricks characteristic
                            switch (setup_pkt.s.wValue) {
                                case 0x0003:
                                    pbio_pybricks_hub_capabilities(
                                        pbdrv_usb_ev3_ep0_buffer.b,
                                        (pbdrv_usb_is_usb_hs ? PYBRICKS_EP_PKT_SZ_HS : PYBRICKS_EP_PKT_SZ_FS) - 1,
                                        PBSYS_CONFIG_APP_FEATURE_FLAGS,
                                        pbsys_storage_get_maximum_program_size(),
                                        PBSYS_CONFIG_HMI_NUM_SLOTS);
                                    pbdrv_usb_setup_data_to_send = pbdrv_usb_ev3_ep0_buffer.u;
                                    pbdrv_usb_setup_data_to_send_sz = PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE;
                                    handled = true;
                                    break;
                            }
                            break;
                    }
                    break;
            }

            // Note regarding the setting of the USB_CSRL0_RXRDYC bit:
            // The Linux kernel has a comment saying
            // > For zero-data requests we want to delay the STATUS stage to avoid SETUPEND errors.
            // but also that
            // > If we write data, the controller acts happier if we enable the TX FIFO right away
            // We implement something similar but not identical. In general, we wait until
            // we have completely processed the request and decided what we're going to do
            // before we indicate that we are ready to progress to the next phase.
            // We do not support any requests that require receiving data from the host,
            // only zero-data requests or those that require sending data to the host.
            // For errors or zero-data requests, we set USB_CSRL0_RXRDYC and USB_CSRL0_DATAEND
            // at the same time so that we don't get spurious SETUPEND errors
            // (which we treat as a command failure). For requests that require sending data,
            // we set USB_CSRL0_RXRDYC while we get ready to copy data into the FIFO.

            if (!handled) {
                // Indicate we read the packet, but also send stall
                HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_RXRDYC | USB_CSRL0_STALL;
            } else {
                if (pbdrv_usb_setup_data_to_send) {
                    // Indicate we read the packet
                    HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_RXRDYC;

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
                    HWREGH(USB0_BASE + USB_O_CSRL0) = USB_CSRL0_RXRDYC | USB_CSRL0_DATAEND;
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

    // EP1 interrupts, which only trigger on error conditions since we use DMA

    if (intr_src & USBOTG_INTR_EP1_OUT) {
        // EP 1 OUT, host to device, rx
        uint8_t rxcsr = HWREGB(USB0_BASE + USB_O_RXCSRL1);

        // Clear error bits
        rxcsr &= ~USB_RXCSRL1_STALLED;

        HWREGB(USB0_BASE + USB_O_RXCSRL1) = rxcsr;
    }

    if (intr_src & USBOTG_INTR_EP1_IN) {
        // EP 1 IN, device to host, tx
        uint8_t txcsr = HWREGB(USB0_BASE + USB_O_TXCSRL1);

        // Clear error bits
        txcsr &= ~(USB_TXCSRL1_STALLED | USB_TXCSRL1_UNDRN | USB_TXCSRL1_FIFONE);

        HWREGB(USB0_BASE + USB_O_TXCSRL1) = txcsr;
    }

    // Check for DMA completions
    uint32_t dma_q_pend_0 = HWREG(USB_0_OTGBASE + CPDMA_PEND_0_REGISTER);

    if (dma_q_pend_0 & (1 << RX_COMPQ1)) {
        // DMA for EP 1 OUT is done

        // Pop the descriptor from the queue
        uint32_t qctrld = HWREG(USB_0_OTGBASE + CPDMA_QUEUE_REGISTER_D + RX_COMPQ1 * 16);
        (void)qctrld;

        // Signal the main loop that we have something
        usb_rx_is_ready = true;
        pbio_os_request_poll();
    }

    while (dma_q_pend_0 & (1 << TX_COMPQ1)) {
        // DMA for EP 1 IN is done

        // Pop the descriptor from the queue
        uint32_t qctrld = HWREG(USB_0_OTGBASE + CPDMA_QUEUE_REGISTER_D + TX_COMPQ1 * 16) & ~CPDMA_QUEUE_REGISTER_DESC_SIZE_MASK;

        if (qctrld == (uint32_t)(&cppi_descriptors[CPPI_DESC_TX_RESPONSE])) {
            transmitting = false;
            pbio_os_request_poll();
        } else if (qctrld == (uint32_t)(&cppi_descriptors[CPPI_DESC_TX_PYBRICKS_EVENT])) {
            transmitting = false;
            pbio_os_request_poll();
        }

        // Multiple TX completions can happen at once
        // (since we have multiple descriptors in flight)
        dma_q_pend_0 = HWREG(USB_0_OTGBASE + CPDMA_PEND_0_REGISTER);
    }

    HWREG(USB_0_OTGBASE + USB_0_INTR_SRC_CLEAR) = intr_src;
    HWREG(USB_0_OTGBASE + USB_0_END_OF_INTR) = 0;
}

/**
 * Pybricks system command handler.
 */
static pbdrv_usb_receive_handler_t pbdrv_usb_receive_handler;

void pbdrv_usb_set_receive_handler(pbdrv_usb_receive_handler_t handler) {
    pbdrv_usb_receive_handler = handler;
}

/**
 * Buffer for scheduled status message.
 */
static uint8_t pbdrv_usb_status_data[PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE];
static bool pbdrv_usb_status_data_pending;

void pbdrv_usb_schedule_status_update(const uint8_t *status_msg) {
    // Ignore if message identical to last.
    if (!memcmp(pbdrv_usb_status_data, status_msg, sizeof(pbdrv_usb_status_data))) {
        return;
    }

    // Schedule to send whenever the USB process gets round to it.
    memcpy(pbdrv_usb_status_data, status_msg, sizeof(pbdrv_usb_status_data));
    pbdrv_usb_status_data_pending = true;
    pbio_os_request_poll();
}

// True if we are expected to send a response to a recent incoming message.
static bool usb_send_response;

/**
 * Non-blocking poll handler to process pending incoming messages.
 */
static void pbdrv_usb_handle_data_in(void) {
    if (!usb_rx_is_ready) {
        return;
    }

    // This barrier prevents *subsequent* memory reads from being
    // speculatively moved *earlier*, outside the if statement
    // (which is technically allowed by the as-if rule).
    pbdrv_compiler_memory_barrier();

    uint32_t usb_rx_sz = PBDRV_UNCACHED(cppi_descriptors[CPPI_DESC_RX].word0).pktLength;
    pbio_pybricks_error_t result;

    // Skip empty commands.
    if (usb_rx_sz) {
        pbdrv_cache_prepare_after_dma(ep1_rx_buf, sizeof(ep1_rx_buf));

        switch (ep1_rx_buf[0]) {
            case PBIO_PYBRICKS_OUT_EP_MSG_SUBSCRIBE:
                pbdrv_usb_is_events_subscribed = ep1_rx_buf[1];
                ep1_tx_response_buf[0] = PBIO_PYBRICKS_IN_EP_MSG_RESPONSE;
                pbio_set_uint32_le(&ep1_tx_response_buf[1], PBIO_PYBRICKS_ERROR_OK);
                usb_send_response = true;
                // Request to resend status flags now that the host has subscribed.
                // Our response will take priority, status follows right after.
                pbdrv_usb_status_data_pending = true;
                break;
            case PBIO_PYBRICKS_OUT_EP_MSG_COMMAND:
                if (!pbdrv_usb_receive_handler) {
                    break;
                }
                result = pbdrv_usb_receive_handler(ep1_rx_buf + 1, usb_rx_sz - 1);
                ep1_tx_response_buf[0] = PBIO_PYBRICKS_IN_EP_MSG_RESPONSE;
                pbio_set_uint32_le(&ep1_tx_response_buf[1], result);
                usb_send_response = true;
                break;
        }
    }

    // Re-queue RX buffer after processing is complete
    usb_rx_is_ready = false;
    usb_setup_rx_dma_desc();
}

static pbio_error_t pbdrv_usb_handle_data_out(void) {

    static pbio_os_timer_t transmit_timer;

    if (transmitting) {
        if (!pbio_os_timer_is_expired(&transmit_timer)) {
            // Still transmitting, can't do anything for now.
            return PBIO_SUCCESS;
        }

        // Transmission has taken too long, so reset the state to allow
        // new transmissions. This can happen if the host stops reading
        // data for some reason. This need some time to complete, so delegate
        // the reset back to the process.
        return PBIO_ERROR_TIMEDOUT;
    }

    // Transmit. Give priority to response, then status updates, then stdout.
    if (usb_send_response) {
        usb_send_response = false;
        transmitting = true;
        pbdrv_cache_prepare_before_dma(ep1_tx_response_buf, sizeof(ep1_tx_response_buf));
        usb_setup_tx_dma_desc(CPPI_DESC_TX_RESPONSE, ep1_tx_response_buf, PBIO_PYBRICKS_USB_MESSAGE_SIZE(sizeof(uint32_t)));
    } else if (pbdrv_usb_is_events_subscribed) {
        if (pbdrv_usb_status_data_pending) {
            pbdrv_usb_status_data_pending = false;
            ep1_tx_status_buf[0] = PBIO_PYBRICKS_IN_EP_MSG_EVENT;
            uint32_t usb_status_sz = PBIO_PYBRICKS_USB_MESSAGE_SIZE(pbsys_status_get_status_report(&ep1_tx_status_buf[1]));

            transmitting = true;
            pbdrv_cache_prepare_before_dma(ep1_tx_status_buf, sizeof(ep1_tx_status_buf));
            usb_setup_tx_dma_desc(CPPI_DESC_TX_PYBRICKS_EVENT, ep1_tx_status_buf, usb_status_sz);
        } else if (ep1_tx_stdout_sz) {
            transmitting = true;
            pbdrv_cache_prepare_before_dma(ep1_tx_stdout_buf, sizeof(ep1_tx_stdout_buf));
            usb_setup_tx_dma_desc(CPPI_DESC_TX_PYBRICKS_EVENT, ep1_tx_stdout_buf, ep1_tx_stdout_sz);
            ep1_tx_stdout_sz = 0;
        }
    }

    if (transmitting) {
        // If the FIFO isn't emptied quickly, then there probably isn't an
        // app anymore. This timer is used to detect such a condition.
        pbio_os_timer_set(&transmit_timer, 50);
    }

    return PBIO_SUCCESS;
}

static pbio_os_process_t pbdrv_usb_ev3_process;

static pbio_error_t pbdrv_usb_ev3_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    pbdrv_usb_handle_data_in();

    if (pbdrv_usb_config == 0) {
        pbdrv_usb_is_events_subscribed = false;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    // Process pending outgoing data until shutdown requested.
    while (pbdrv_usb_ev3_process.request != PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL) {

        PBIO_OS_ASYNC_SET_CHECKPOINT(state);
        pbio_error_t err = pbdrv_usb_handle_data_out();
        if (err == PBIO_SUCCESS) {
            return PBIO_ERROR_AGAIN;
        }

        transmitting = false;
        ep1_tx_stdout_sz = 0;
        pbdrv_usb_is_events_subscribed = false;

        // Flush _all_ TX packets
        while (HWREGB(USB0_BASE + USB_O_TXCSRL1) & USB_TXCSRL1_TXRDY) {
            HWREGB(USB0_BASE + USB_O_TXCSRL1) = USB_TXCSRL1_FLUSH;
            // We need to wait a bit until the DMA refills the FIFO.
            // There doesn't seem to be a good way to figure out if
            // there are packets in flight *within* the DMA engine itself
            // (i.e. no longer in the queue but in the transfer engine).
            PBIO_OS_AWAIT_MS(state, &timer, 1);
        }
    }

    PBIO_OS_ASYNC_END(PBIO_ERROR_CANCELED);
}

void pbdrv_usb_deinit(void) {
    pbio_os_process_make_request(&pbdrv_usb_ev3_process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);
}

void pbdrv_usb_init(void) {
    // If we came straight from a firmware update, we need to send a disconnect
    // to the host, then reset the USB controller.
    USBDevDisconnect(USB0_BASE);

    // Reset the module through the PSC
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_USB0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_SWRSTDISABLE);
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_USB0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    // Local soft reset, as well as following the sequence described in sprz313h.pdf
    // Advisory 2.3.3 describing a workaround for a potential reset timing issue
    USBReset(USB_0_OTGBASE);
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_USB0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_DISABLE);

    // This reset sequence is from Example 34-1 in the AM1808 TRM (spruh82c.pdf)
    // Because PHYs and clocking are... as they tend to be, use the precise sequence
    // of operations specified.

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

    // Final enable for the USB module
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_USB0, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

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
        USBOTG_INTR_DISCON |
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

bool pbdrv_usb_connection_is_active(void) {
    return pbdrv_usb_is_events_subscribed;
}

/**
 * Queues data to be transmitted via USB.
 * @param data  [in]        The data to be sent.
 * @param size  [in, out]   The size of @p data in bytes. After return, @p size
 *                          contains the number of bytes actually written.
 * @return                  ::PBIO_SUCCESS if some @p data was queued, ::PBIO_ERROR_AGAIN
 *                          if no @p data could not be queued at this time (e.g. buffer
 *                          is full), ::PBIO_ERROR_INVALID_OP if there is not an
 *                          active USB connection
 */
pbio_error_t pbdrv_usb_stdout_tx(const uint8_t *data, uint32_t *size) {
    if (!pbdrv_usb_is_events_subscribed) {
        // If the app hasn't subscribed to events, we can't send stdout.
        return PBIO_ERROR_INVALID_OP;
    }

    uint8_t *ptr = ep1_tx_stdout_buf;
    uint32_t ptr_len = pbdrv_usb_is_usb_hs ? PYBRICKS_EP_PKT_SZ_HS : PYBRICKS_EP_PKT_SZ_FS;

    if (transmitting) {
        *size = 0;
        return PBIO_ERROR_AGAIN;
    }

    *ptr++ = PBIO_PYBRICKS_IN_EP_MSG_EVENT;
    ptr_len--;

    *ptr++ = PBIO_PYBRICKS_EVENT_WRITE_STDOUT;
    ptr_len--;

    if (*size > ptr_len) {
        *size = ptr_len;
    }
    memcpy(ptr, data, *size);
    ep1_tx_stdout_sz = 2 + *size;
    pbio_os_request_poll();

    return PBIO_SUCCESS;
}

// REVISIT: These two functions do not keep track of how much data
// is held in the DMA engine or the packet FIFO (i.e. the DMA engine
// has returned the descriptor to us (and thus we can queue more),
// but the hardware is still buffering data which we cannot see without
// performing much more accurate accounting)
uint32_t pbdrv_usb_stdout_tx_available(void) {
    if (!pbdrv_usb_is_events_subscribed) {
        return UINT32_MAX;
    }

    if (transmitting) {
        return 0;
    }

    // Subtract 2 bytes for header
    if (pbdrv_usb_is_usb_hs) {
        return PYBRICKS_EP_PKT_SZ_HS - 2;
    }
    return PYBRICKS_EP_PKT_SZ_FS - 2;
}

bool pbdrv_usb_stdout_tx_is_idle(void) {
    return !transmitting;
}

#endif // PBDRV_CONFIG_USB_EV3
