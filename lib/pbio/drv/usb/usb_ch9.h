// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Various bits of USB "Chapter 9" types

#ifndef _INTERNAL_PBDRV_USB_CH9_H_
#define _INTERNAL_PBDRV_USB_CH9_H_

#include <stdint.h>
#include <pbdrv/compiler.h>

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error This file needs to be revisited for big-endian systems
#endif

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
//
// The following macro helps construct these structs
#define PBDRV_USB_TYPE_PUNNING_HELPER(type)                                     \
    typedef union {                                                                 \
        type##_t s;                                                                 \
        uint32_t u[(sizeof(type##_t) + sizeof(uint32_t) - 1) / sizeof(uint32_t)];   \
    } type##_union_t;

// SETUP transaction packet structure
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} pbdrv_usb_setup_packet_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_setup_packet)

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
// Binary Object Store, originally from the Wireless USB / USB 3.0 spec
// These descriptors have been backported to USB 2 and are available to
// devices which indicate a bcdDevice of at least 2.1.
// They are used for WebUSB and Microsoft WinUSB driver installation.
#define DESC_TYPE_BOS                           15
#define DESC_TYPE_DEVICE_CAPABILITY             16

// Device descriptor
typedef struct PBDRV_PACKED {
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
} pbdrv_usb_dev_desc_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_dev_desc);

// Configuration descriptor
typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} pbdrv_usb_conf_desc_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_conf_desc);

#define USB_CONF_DESC_BM_ATTR_MUST_BE_SET       0x80
#define USB_CONF_DESC_BM_ATTR_SELF_POWERED      0x40
#define USB_CONF_DESC_BM_ATTR_REMOTE_WAKEUP     0x20

// Interface descriptor
typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} pbdrv_usb_iface_desc_t;

// This LangID is used for string descriptors
// English (United States)
#define PBDRV_USB_STRING_LANGID_EN_US   0x0409

// Endpoint descriptor
typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} pbdrv_usb_ep_desc_t;

// Endpoint types for bmAttributes
#define PBDRV_USB_EP_TYPE_CTRL    0
#define PBDRV_USB_EP_TYPE_ISOC    1
#define PBDRV_USB_EP_TYPE_BULK    2
#define PBDRV_USB_EP_TYPE_INTR    3

// Device qualifier descriptor
typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint8_t bNumConfigurations;
    uint8_t bReserved;
} pbdrv_usb_dev_qualifier_desc_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_dev_qualifier_desc);

// Binary Object Store
typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumDeviceCaps;
} pbdrv_usb_bos_desc_t;

typedef struct PBDRV_PACKED {
    uint32_t u0;
    uint16_t u1;
    uint16_t u2;
    uint8_t u3[8];
} pbdrv_usb_uuid_t;

// Platform-specific capabilities
#define USB_DEVICE_CAPABILITY_TYPE_PLATFORM     5

typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDevCapabilityType;
    uint8_t bReserved;
    pbdrv_usb_uuid_t uuid;
} pbdrv_usb_platform_caps_common_t;

typedef struct PBDRV_PACKED {
    pbdrv_usb_platform_caps_common_t hdr;
    uint16_t bcdVersion;
    uint8_t bVendorCode;
    uint8_t iLandingPage;
} pbdrv_usb_webusb_capability_t;

// {3408b638-09a9-47a0-8bfd-a0768815b665}
#define USB_PLATFORM_CAP_GUID_WEBUSB    {                           \
        .u0 = 0x3408b638,                                           \
        .u1 = 0x09a9,                                               \
        .u2 = 0x47a0,                                               \
        .u3 = {0x8b, 0xfd, 0xa0, 0x76, 0x88, 0x15, 0xb6, 0x65},     \
}

typedef struct PBDRV_PACKED {
    pbdrv_usb_platform_caps_common_t hdr;
    uint32_t dwWindowsVersion;
    uint16_t wMSOSDescriptorSetTotalLength;
    uint8_t bMS_VendorCode;
    uint8_t bAltEnumCode;
} pbdrv_usb_microsoft_20_capability_t;

// {D8DD60DF-4589-4CC7-9CD2-659D9E648A9F}
#define USB_PLATFORM_CAP_GUID_MS_20     {                           \
        .u0 = 0xD8DD60DF,                                           \
        .u1 = 0x4589,                                               \
        .u2 = 0x4CC7,                                               \
        .u3 = {0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F},     \
}

// These are not technically part of USB Chapter 9, but
// we need them and they logically fit with the rest of the structs.

#define WEBUSB_REQ_GET_URL          2
#define WEBUSB_DESC_TYPE_URL        3
#define WEBUSB_URL_SCHEME_HTTP      0
#define WEBUSB_URL_SCHEME_HTTPS     1

typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bScheme;
    uint8_t url[];
} pbdrv_usb_webusb_url_desc_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_webusb_url_desc);

// Microsoft wIndex values
#define MS_OS_20_DESCRIPTOR_INDEX       7
#define MS_OS_20_SET_ALT_ENUMERATION    8

// Microsoft descriptor types
#define MS_OS_20_SET_HEADER_DESCRIPTOR          0
#define MS_OS_20_SUBSET_HEADER_CONFIGURATION    1
#define MS_OS_20_SUBSET_HEADER_FUNCTION         2
#define MS_OS_20_FEATURE_COMPATBLE_ID           3
#define MS_OS_20_FEATURE_REG_PROPERTY           4
#define MS_OS_20_FEATURE_MIN_RESUME_TIME        5
#define MS_OS_20_FEATURE_MODEL_ID               6
#define MS_OS_20_FEATURE_CCGP_DEVICE            7
#define MS_OS_20_FEATURE_VENDOR_REVISION        8

// We only need (and thus define) the following descriptor types

typedef struct PBDRV_PACKED {
    uint16_t wLength;
    uint16_t wDescriptorType;
    uint32_t dwWindowsVersion;
    uint16_t wTotalLength;
} pbdrv_usb_ms_20_desc_set_header_t;

typedef struct PBDRV_PACKED {
    uint16_t wLength;
    uint16_t wDescriptorType;
    uint8_t CompatibleID[8];
    uint8_t SubCompatibleID[8];
} pbdrv_usb_ms_20_compatible_t;

typedef struct PBDRV_PACKED {
    uint16_t wLength;
    uint16_t wDescriptorType;
    uint16_t wPropertyDataType;
} pbdrv_usb_ms_20_reg_prop_hdr_t;

#define MS_OS_20_REG_PROP_TYPE_REG_SZ           1
#define MS_OS_20_REG_PROP_TYPE_REG_EXPAND_SZ    2
#define MS_OS_20_REG_PROP_TYPE_REG_BINARY       3
#define MS_OS_20_REG_PROP_TYPE_REG_DWORD_LE     4
#define MS_OS_20_REG_PROP_TYPE_REG_DWORD_BE     5
#define MS_OS_20_REG_PROP_TYPE_REG_LINK         6
#define MS_OS_20_REG_PROP_TYPE_REG_MULTI_SZ     7

// This is the minimum Windows version needed to support these descriptors
#define MS_WINDOWS_VERSION_81       0x06030000

#endif // _INTERNAL_PBDRV_USB_CH9_H_
