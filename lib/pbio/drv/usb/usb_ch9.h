// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Various bits of USB "Chapter 9" types

// A number of these types additionally define a union with uint32_t.
// This serves two functions:
// 1. Aligns the data to a 4-byte boundary
// 2. Explicitly enables type punning without violating strict aliasing
//
// The latter is important for drivers which want to efficiently copy data
// multiple bytes at a time. The C standard considers it undefined to access
// data behind a struct pointer by casting it to a uint32_t pointer,
// and doing so can cause miscompiles. In order to get what we actually want,
// GCC explicitly allows type punning via unions. This is documented in the manual:
// https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#Type-punning

#ifndef _INTERNAL_PBDRV_USB_CH9_H_
#define _INTERNAL_PBDRV_USB_CH9_H_

#include <stdint.h>

// SETUP transaction packet structure
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet;

typedef union {
    usb_setup_packet s;
    uint32_t u[2];
} usb_setup_packet_u;

// Bitfields in bmRequestType
#define BM_REQ_DIR_MASK         0x80
#define BM_REQ_DIR_H2D          0x00
#define BM_REQ_DIR_D2H          0x80

#define BM_REQ_TYPE_MASK        0x60
#define BM_REQ_TYPE_STANDARD    (0 << 5)
#define BM_REQ_TYPE_CLASS       (1 << 5)
#define BM_REQ_TYPE_VENDOR      (2 << 5)

#define BM_REQ_RECIP_MASK       0x1f
#define BM_REQ_RECIP_DEV        0
#define BM_REQ_RECIP_IF         1
#define BM_REQ_RECIP_EP         2
#define BM_REQ_RECIP_OTHER      3

// Standard USB requests
#define GET_STATUS              0
#define CLEAR_FEATURE           1
#define SET_FEATURE             3
#define SET_ADDRESS             5
#define GET_DESCRIPTOR          6
#define SET_DESCRIPTOR          7
#define GET_CONFIGURATION       8
#define SET_CONFIGURATION       9
#define GET_INTERFACE           10
#define SET_INTERFACE           11
#define SYNCH_FRAME             12

// Standard descriptor types
#define DESC_TYPE_DEVICE                        1
#define DESC_TYPE_CONFIGURATION                 2
#define DESC_TYPE_STRING                        3
#define DESC_TYPE_INTERFACE                     4
#define DESC_TYPE_ENDPOINT                      5
#define DESC_TYPE_DEVICE_QUALIFIER              6
#define DESC_TYPE_OTHER_SPEED_CONFIGURATION     7
#define DESC_TYPE_INTERFACE_POWER               8

// Device descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} usb_dev_desc;

typedef union {
    usb_dev_desc s;
    uint32_t u[(sizeof(usb_dev_desc) + 3) / 4];
} usb_dev_desc_u;

// Configuration descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} usb_conf_desc;

#define USB_CONF_DESC_BM_ATTR_MUST_BE_SET       0x80
#define USB_CONF_DESC_BM_ATTR_SELF_POWERED      0x40
#define USB_CONF_DESC_BM_ATTR_REMOTE_WAKEUP     0x20

// Interface descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} usb_iface_desc;

// Endpoint descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} usb_ep_desc;

// Endpoint types for bmAttributes
#define EP_TYPE_CTRL    0
#define EP_TYPE_ISOC    1
#define EP_TYPE_BULK    2
#define EP_TYPE_INTR    3

// Device qualifier descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint8_t bNumConfigurations;
    uint8_t bReserved;
} usb_dev_qualifier_desc;

typedef union {
    usb_dev_qualifier_desc s;
    uint32_t u[(sizeof(usb_dev_qualifier_desc) + 3) / 4];
} usb_dev_qualifier_desc_u;

#endif // _INTERNAL_PBDRV_USB_CH9_H_
