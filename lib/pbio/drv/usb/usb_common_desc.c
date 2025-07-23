// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Descriptors which are shared across devices
// (i.e. WebUSB, Microsoft OS descriptors)

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB

#include "usb_common_desc.h"

#if PBIO_VERSION_LEVEL_HEX == 0xA
#define PYBRICKS_CODE_URL   "labs.pybricks.com"
#elif PBIO_VERSION_LEVEL_HEX == 0xB
#define PYBRICKS_CODE_URL   "beta.pybricks.com"
#else
#define PYBRICKS_CODE_URL   "code.pybricks.com"
#endif

const pbdrv_usb_webusb_url_desc_union_t pbdrv_usb_webusb_landing_page = {
    .s = {
        .bLength = sizeof(pbdrv_usb_webusb_url_desc_t) + sizeof(PYBRICKS_CODE_URL) - 1,
        .bDescriptorType = WEBUSB_DESC_TYPE_URL,
        .bScheme = WEBUSB_URL_SCHEME_HTTPS,
        .url = PYBRICKS_CODE_URL,
    },
};

#define MS_20_REGISTRY_DATA_EXTRA_SZ                    \
    2 + sizeof(PBDRV_USB_MS_20_REG_PROPERTY_NAME) +     \
    2 + sizeof(PBDRV_USB_MS_20_REG_PROPERTY_VAL)

const pbdrv_usb_ms_20_desc_set_union_t pbdrv_usb_ms_20_desc_set = {
    .s = {
        .desc_set_hdr = {
            .wLength = sizeof(pbdrv_usb_ms_20_desc_set_header_t),
            .wDescriptorType = MS_OS_20_SET_HEADER_DESCRIPTOR,
            .dwWindowsVersion = MS_WINDOWS_VERSION_81,
            .wTotalLength = sizeof(pbdrv_usb_ms_20_desc_set_t),
        },
        .compatible = {
            .wLength = sizeof(pbdrv_usb_ms_20_compatible_t),
            .wDescriptorType = MS_OS_20_FEATURE_COMPATBLE_ID,
            .CompatibleID = "WINUSB",
            .SubCompatibleID = "",
        },
        .reg_prop_hdr = {
            .wLength = sizeof(pbdrv_usb_ms_20_reg_prop_hdr_t) + MS_20_REGISTRY_DATA_EXTRA_SZ,
            .wDescriptorType = MS_OS_20_FEATURE_REG_PROPERTY,
            .wPropertyDataType = MS_OS_20_REG_PROP_TYPE_REG_MULTI_SZ,
        },
        .property_name_len = sizeof(PBDRV_USB_MS_20_REG_PROPERTY_NAME),
        .device_interface_guids = PBDRV_USB_MS_20_REG_PROPERTY_NAME,
        .property_val_len = sizeof(PBDRV_USB_MS_20_REG_PROPERTY_VAL),
        .device_interface_guid_val = PBDRV_USB_MS_20_REG_PROPERTY_VAL,
    }
};

const pbdrv_usb_bos_desc_set_union_t pbdrv_usb_bos_desc_set = {
    .s = {
        .bos = {
            .bLength = sizeof(pbdrv_usb_bos_desc_t),
            .bDescriptorType = DESC_TYPE_BOS,
            .wTotalLength = sizeof(pbdrv_usb_bos_desc_set_t),
            .bNumDeviceCaps = 2,
        },
        .webusb = {
            .hdr = {
                .bLength = sizeof(pbdrv_usb_webusb_capability_t),
                .bDescriptorType = DESC_TYPE_DEVICE_CAPABILITY,
                .bDevCapabilityType = USB_DEVICE_CAPABILITY_TYPE_PLATFORM,
                .bReserved = 0,
                .uuid = USB_PLATFORM_CAP_GUID_WEBUSB,
            },
            .bcdVersion = 0x0100,
            .bVendorCode = PBDRV_USB_VENDOR_REQ_WEBUSB,
            .iLandingPage = PBDRV_USB_WEBUSB_LANDING_PAGE_URL_IDX,
        },
        .ms_20 = {
            .hdr = {
                .bLength = sizeof(pbdrv_usb_microsoft_20_capability_t),
                .bDescriptorType = DESC_TYPE_DEVICE_CAPABILITY,
                .bDevCapabilityType = USB_DEVICE_CAPABILITY_TYPE_PLATFORM,
                .bReserved = 0,
                .uuid = USB_PLATFORM_CAP_GUID_MS_20,
            },
            .dwWindowsVersion = MS_WINDOWS_VERSION_81,
            .wMSOSDescriptorSetTotalLength = sizeof(pbdrv_usb_ms_20_desc_set_t),
            .bMS_VendorCode = PBDRV_USB_VENDOR_REQ_MS_20,
            .bAltEnumCode = 0,
        }
    }
};

#endif // PBDRV_CONFIG_USB
