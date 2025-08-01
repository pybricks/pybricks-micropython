// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Descriptors which are shared across devices
// (i.e. WebUSB, Microsoft OS descriptors)

#ifndef _INTERNAL_PBDRV_USB_COMMON_DESC_H_
#define _INTERNAL_PBDRV_USB_COMMON_DESC_H_

#include <pbio/util.h>

#include <lego/usb.h>
#include "pbdrvconfig.h"

#include "usb_ch9.h"

/**
 * USB vendor requests which will be used to retrieve WebUSB and Microsoft descriptors
 */
enum {
    PBDRV_USB_VENDOR_REQ_WEBUSB,
    PBDRV_USB_VENDOR_REQ_MS_20,
};

// The descriptor index for the WebUSB landing page
#define PBDRV_USB_WEBUSB_LANDING_PAGE_URL_IDX       1

#define PBDRV_USB_MS_20_REG_PROPERTY_NAME   u"DeviceInterfaceGUIDs"
// This UUID has been generated by Pybricks and identifies specifically
// our devices which use the Pybricks protocol. Windows APIs can match on this.
// An extra null terminator is required to terminate a REG_MULTI_SZ
#define PBDRV_USB_MS_20_REG_PROPERTY_VAL    u"{A5C44A4C-53D4-4389-9821-AE95051908A1}\x00"

// Complete set of Microsoft USB descriptors
typedef struct PBDRV_PACKED {
    pbdrv_usb_ms_20_desc_set_header_t desc_set_hdr;
    pbdrv_usb_ms_20_compatible_t compatible;
    pbdrv_usb_ms_20_reg_prop_hdr_t reg_prop_hdr;
    uint16_t property_name_len;
    uint16_t device_interface_guids[PBIO_ARRAY_SIZE(PBDRV_USB_MS_20_REG_PROPERTY_NAME)];
    uint16_t property_val_len;
    uint16_t device_interface_guid_val[PBIO_ARRAY_SIZE(PBDRV_USB_MS_20_REG_PROPERTY_VAL)];
} pbdrv_usb_ms_20_desc_set_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_ms_20_desc_set);

extern const pbdrv_usb_ms_20_desc_set_union_t pbdrv_usb_ms_20_desc_set;

// WebUSB descriptor
extern const pbdrv_usb_webusb_url_desc_union_t pbdrv_usb_webusb_landing_page;

// Complete set of USB BOS descriptors
typedef struct PBDRV_PACKED {
    pbdrv_usb_bos_desc_t bos;
    // WebUSB must occur before Microsoft OS descriptors
    pbdrv_usb_webusb_capability_t webusb;
    pbdrv_usb_microsoft_20_capability_t ms_20;
} pbdrv_usb_bos_desc_set_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_bos_desc_set);

extern const pbdrv_usb_bos_desc_set_union_t pbdrv_usb_bos_desc_set;

// (Human-readable) string descriptors
typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t langID[1];
} pbdrv_usb_langid_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_langid);

extern const pbdrv_usb_langid_union_t pbdrv_usb_str_desc_langid;

typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t str[PBIO_ARRAY_SIZE(PBDRV_CONFIG_USB_MFG_STR) - 1];
} pbdrv_usb_str_mfg_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_str_mfg);

extern const pbdrv_usb_str_mfg_union_t pbdrv_usb_str_desc_mfg;

typedef struct PBDRV_PACKED {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t str[PBIO_ARRAY_SIZE(PBDRV_CONFIG_USB_PROD_STR) - 1];
} pbdrv_usb_str_prod_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_str_prod);

extern const pbdrv_usb_str_prod_union_t pbdrv_usb_str_desc_prod;

#endif // _INTERNAL_PBDRV_USB_COMMON_DESC_H_
