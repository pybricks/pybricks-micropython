// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Descriptors which are shared across devices

#ifndef _INTERNAL_PBDRV_USB_COMMON_DESC_H_
#define _INTERNAL_PBDRV_USB_COMMON_DESC_H_

#include <pbio/util.h>

#include <lego/usb.h>
#include "pbdrvconfig.h"

#include "usb_ch9.h"

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
