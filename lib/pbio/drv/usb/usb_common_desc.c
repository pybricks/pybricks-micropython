// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Descriptors which are shared across devices

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB

#include "usb_common_desc.h"

const pbdrv_usb_langid_union_t pbdrv_usb_str_desc_langid = {
    .s = {
        .bLength = 4,
        .bDescriptorType = DESC_TYPE_STRING,
        .langID = {PBDRV_USB_STRING_LANGID_EN_US},
    }
};

const pbdrv_usb_str_mfg_union_t pbdrv_usb_str_desc_mfg = {
    .s = {
        .bLength = sizeof(pbdrv_usb_str_mfg_t),
        .bDescriptorType = DESC_TYPE_STRING,
        .str = PBDRV_CONFIG_USB_MFG_STR,
    }
};

const pbdrv_usb_str_prod_union_t pbdrv_usb_str_desc_prod = {
    .s = {
        .bLength = sizeof(pbdrv_usb_str_prod_t),
        .bDescriptorType = DESC_TYPE_STRING,
        .str = PBDRV_CONFIG_USB_PROD_STR,
    }
};

#endif // PBDRV_CONFIG_USB
