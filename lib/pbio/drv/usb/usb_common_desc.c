// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Descriptors which are shared across devices

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB

#include <string.h>

#include <pbsys/host.h>

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

/**
 * Gets the product string descriptor, built at runtime from the hub display
 * name so that the hub is recognizable in host OS device pickers.
 */
const pbdrv_usb_str_prod_union_t *pbdrv_usb_get_str_desc_prod(void) {
    static pbdrv_usb_str_prod_union_t desc;

    const char *hub_name = pbsys_host_get_hub_display_name(PBSYS_HOST_TRANSPORT_TYPE_USB);
    size_t len = strlen(hub_name);

    desc.s.bLength = 2 + 2 * len;
    desc.s.bDescriptorType = DESC_TYPE_STRING;
    for (size_t i = 0; i < len; i++) {
        desc.s.str[i] = hub_name[i];
    }

    return &desc;
}

#endif // PBDRV_CONFIG_USB
