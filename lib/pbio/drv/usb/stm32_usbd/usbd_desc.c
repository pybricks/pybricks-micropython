/**
  ******************************************************************************
  * @file    USB_Device/CDC_Standalone/Src/usbd_desc.c
  * @author  MCD Application Team
  * @brief   This file provides the USBD descriptors and string formating method.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include <string.h>

#include <lego_usb.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/config.h>
#include <pbio/protocol.h>
#include <pbio/version.h>
#include <pbsys/app.h>
#include <pbsys/program_load.h>

#include "usbd_core.h"
#include "usbd_conf.h"
#include "usbd_pybricks.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define USBD_LANGID_STRING            0x409
#define USBD_CONFIGURATION_FS_STRING  "Pybricks Config"
#define USBD_INTERFACE_FS_STRING      "Pybricks Interface"

static const char firmware_version[] = PBIO_VERSION_STR;
static const char software_version[] = PBIO_PROTOCOL_VERSION_STR;

#define         DEVICE_ID1          (0x1FFF7A10)
#define         DEVICE_ID2          (0x1FFF7A14)
#define         DEVICE_ID3          (0x1FFF7A18)

#define  USB_DEV_CAP_TYPE_PLATFORM   (5)

#define  USB_SIZ_STRING_SERIAL       0x1A
#define  USB_SIZ_BOS_DESC_CONST      (5 + 28 + 24)
#define  USB_SIZ_UUID                (128 / 8)
#define  USB_SIZ_PLATFORM_HDR        (4 + USB_SIZ_UUID)
#define  USB_SIZ_HUB_NAME_MAX        (16)
#define  USB_SIZ_BOS_DESC            (USB_SIZ_BOS_DESC_CONST + \
    USB_SIZ_PLATFORM_HDR + USB_SIZ_HUB_NAME_MAX + \
    USB_SIZ_PLATFORM_HDR + sizeof(firmware_version) - 1 + \
    USB_SIZ_PLATFORM_HDR + sizeof(software_version) - 1 + \
    USB_SIZ_PLATFORM_HDR + PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE)

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static
#if !defined(PBDRV_CONFIG_USB_STM32F4_HUB_VARIANT_ADDR)
const
#endif
uint8_t USBD_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12,                     /* bLength */
    USB_DESC_TYPE_DEVICE,     /* bDescriptorType */
    0x01,                     /* bcdUSB */  /* changed to USB version 2.01
                                               in order to support BOS Desc */
    0x02,
    PBIO_PYBRICKS_USB_DEVICE_CLASS,     /* bDeviceClass */
    PBIO_PYBRICKS_USB_DEVICE_SUBCLASS,  /* bDeviceSubClass */
    PBIO_PYBRICKS_USB_DEVICE_PROTOCOL,  /* bDeviceProtocol */
    USB_MAX_EP0_SIZE,         /* bMaxPacketSize */
    LOBYTE(PBDRV_CONFIG_USB_VID), /* idVendor */
    HIBYTE(PBDRV_CONFIG_USB_VID), /* idVendor */
    LOBYTE(PBDRV_CONFIG_USB_PID), /* idProduct */
    HIBYTE(PBDRV_CONFIG_USB_PID), /* idProduct */
    0x00,                     /* bcdDevice rel. 2.00 */
    0x02,
    USBD_IDX_MFC_STR,         /* Index of manufacturer string */
    USBD_IDX_PRODUCT_STR,     /* Index of product string */
    USBD_IDX_SERIAL_STR,      /* Index of serial number string */
    USBD_MAX_NUM_CONFIGURATION /* bNumConfigurations */
}; /* USB_DeviceDescriptor */

/** BOS descriptor. */
__ALIGN_BEGIN static uint8_t USBD_BOSDesc[USB_SIZ_BOS_DESC] __ALIGN_END =
{
    5,                          /* bLength */
    USB_DESC_TYPE_BOS,          /* bDescriptorType = BOS */
    LOBYTE(USB_SIZ_BOS_DESC),   /* wTotalLength */
    HIBYTE(USB_SIZ_BOS_DESC),   /* wTotalLength */
    2,                          /* bNumDeviceCaps */

    28,                         /* bLength */
    USB_DEVICE_CAPABITY_TYPE,   /* bDescriptorType = Device Capability */
    USB_DEV_CAP_TYPE_PLATFORM,  /* bDevCapabilityType */
    0x00,                       /* bReserved */

    /*
     * PlatformCapabilityUUID
     * Microsoft OS 2.0 descriptor platform capability ID
     * D8DD60DF-4589-4CC7-9CD2-659D9E648A9F
     * RFC 4122 explains the correct byte ordering
     */
    0xDF, 0x60, 0xDD, 0xD8,           /* 32-bit value */
    0x89, 0x45,                       /* 16-bit value */
    0xC7, 0x4C,                       /* 16-bit value */
    0x9C, 0xD2,
    0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,

    0x00, 0x00, 0x03, 0x06,           /* dwWindowsVersion = 0x06030000 for Windows 8.1 Build */
    LOBYTE(USBD_SIZ_MS_OS_DSCRPTR_SET), /* wMSOSDescriptorSetTotalLength */
    HIBYTE(USBD_SIZ_MS_OS_DSCRPTR_SET), /* wMSOSDescriptorSetTotalLength */
    USBD_MS_VENDOR_CODE,              /* bMS_VendorCode */
    0x00,                             /* bAltEnumCode = Does not support alternate enumeration */

    24,                         /* bLength */
    USB_DEVICE_CAPABITY_TYPE,   /* bDescriptorType = Device Capability */
    USB_DEV_CAP_TYPE_PLATFORM,  /* bDevCapabilityType */
    0x00,                       /* bReserved */

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

    LOBYTE(0x0100),                   /* bcdVersion */
    HIBYTE(0x0100),                   /* bcdVersion */
    USBD_WEBUSB_VENDOR_CODE,          /* bVendorCode */
    USBD_WEBUSB_LANDING_PAGE_IDX      /* iLandingPage */
};

static uint16_t USBD_BOSDesc_Len;

__ALIGN_BEGIN const uint8_t USBD_OSDescSet[USBD_SIZ_MS_OS_DSCRPTR_SET] __ALIGN_END =
{
    0x0A, 0x00,                       /* wLength = 10 */
    0x00, 0x00,                       /* wDescriptorType = MS_OS_20_SET_HEADER_DESCRIPTOR */
    0x00, 0x00, 0x03, 0x06,           /* dwWindowsVersion = 0x06030000 for Windows 8.1 Build */
    LOBYTE(USBD_SIZ_MS_OS_DSCRPTR_SET), /* wTotalLength */
    HIBYTE(USBD_SIZ_MS_OS_DSCRPTR_SET), /* wTotalLength (cont.) */

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
    '\0', '\0'
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static const uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC,
    USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID_STRING),
    HIBYTE(USBD_LANGID_STRING),
};

__ALIGN_BEGIN static uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] __ALIGN_END = {
    USB_SIZ_STRING_SERIAL,
    USB_DESC_TYPE_STRING,
};

__ALIGN_BEGIN static uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;


/**
  * @brief  Convert Hex 32Bits value into char
  * @param  value: value to convert
  * @param  pbuf: pointer to the buffer
  * @param  len: buffer length
  * @retval None
  */
static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len) {
    uint8_t idx = 0;

    for (idx = 0; idx < len; idx++) {
        if (((value >> 28)) < 0xA) {
            pbuf[ 2 * idx] = (value >> 28) + '0';
        } else {
            pbuf[2 * idx] = (value >> 28) + 'A' - 10;
        }

        value = value << 4;

        pbuf[ 2 * idx + 1] = 0;
    }
}

/**
  * @brief  Create the serial number string descriptor
  * @param  None
  * @retval None
  */
static void Get_SerialNum(void) {
    uint32_t deviceserial0, deviceserial1, deviceserial2;

    deviceserial0 = *(uint32_t *)DEVICE_ID1;
    deviceserial1 = *(uint32_t *)DEVICE_ID2;
    deviceserial2 = *(uint32_t *)DEVICE_ID3;

    deviceserial0 += deviceserial2;

    if (deviceserial0 != 0) {
        IntToUnicode(deviceserial0, &USBD_StringSerial[2], 8);
        IntToUnicode(deviceserial1, &USBD_StringSerial[18], 4);
    }
}

/**
  * @brief  Add the short BLE UUID to the buffer in little-endian format.
  * @param  dst     The destination buffer
  * @param  dst     The short BLE UUID to add
  * @retval None
  */
static void add_ble_short_uuid_le(uint8_t *dst, uint16_t short_uuid) {
    /* 32-bit */
    dst[0] = LOBYTE(short_uuid);
    dst[1] = HIBYTE(short_uuid);
    dst[2] = 0x00;
    dst[3] = 0x00;

    /* 16-bit */
    dst[4] = 0x00;
    dst[5] = 0x00;

    /* 16-bit */
    dst[6] = 0x00;
    dst[7] = 0x10;

    /* 8-byte buffer */
    dst[8] = 0x80;
    dst[9] = 0x00;
    dst[10] = 0x00;
    dst[11] = 0x80;
    dst[12] = 0x5F;
    dst[13] = 0x9B;
    dst[14] = 0x34;
    dst[15] = 0xFB;
}

/**
  * @brief  Returns the device descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_Pybricks_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    /* Prevent unused argument(s) compilation warning */
    UNUSED(speed);

    *length = sizeof(USBD_DeviceDesc);
    return (uint8_t *)USBD_DeviceDesc;
}

/**
  * @brief  Returns the LangID string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_Pybricks_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    /* Prevent unused argument(s) compilation warning */
    UNUSED(speed);

    *length = sizeof(USBD_LangIDDesc);
    return (uint8_t *)USBD_LangIDDesc;
}

/**
  * @brief  Returns the product string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_Pybricks_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    USBD_GetString((uint8_t *)PBDRV_CONFIG_USB_PROD_STR, USBD_StrDesc, length);
    return USBD_StrDesc;
}

/**
  * @brief  Returns the manufacturer string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_Pybricks_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    /* Prevent unused argument(s) compilation warning */
    UNUSED(speed);

    USBD_GetString((uint8_t *)PBDRV_CONFIG_USB_MFG_STR, USBD_StrDesc, length);
    return USBD_StrDesc;
}

/**
  * @brief  Returns the serial number string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_Pybricks_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    /* Prevent unused argument(s) compilation warning */
    UNUSED(speed);

    *length = USB_SIZ_STRING_SERIAL;

    /* Update the serial number string descriptor with the data from the unique ID*/
    Get_SerialNum();

    return (uint8_t *)USBD_StringSerial;
}

/**
  * @brief  Returns the configuration string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_Pybricks_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

/**
  * @brief  Returns the interface string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_Pybricks_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    USBD_GetString((uint8_t *)USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

static uint8_t *USBD_Pybricks_BOSDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    /* Prevent unused argument(s) compilation warning */
    UNUSED(speed);

    *length = USBD_BOSDesc_Len;
    return (uint8_t *)USBD_BOSDesc;
}

USBD_DescriptorsTypeDef USBD_Pybricks_Desc = {
    .GetDeviceDescriptor = USBD_Pybricks_DeviceDescriptor,
    .GetLangIDStrDescriptor = USBD_Pybricks_LangIDStrDescriptor,
    .GetManufacturerStrDescriptor = USBD_Pybricks_ManufacturerStrDescriptor,
    .GetProductStrDescriptor = USBD_Pybricks_ProductStrDescriptor,
    .GetSerialStrDescriptor = USBD_Pybricks_SerialStrDescriptor,
    .GetConfigurationStrDescriptor = USBD_Pybricks_ConfigStrDescriptor,
    .GetInterfaceStrDescriptor = USBD_Pybricks_InterfaceStrDescriptor,
    .GetBOSDescriptor = USBD_Pybricks_BOSDescriptor,
};

void USBD_Pybricks_Desc_Init(void) {
    // the same firmware runs on both SPIKE Prime and Robot Inventor so we need
    // to dynamically set the PID based on the variant
    #ifdef PBDRV_CONFIG_USB_STM32F4_HUB_VARIANT_ADDR
    #define VARIANT (*(uint32_t *)PBDRV_CONFIG_USB_STM32F4_HUB_VARIANT_ADDR)
    if (VARIANT == 0) {
        USBD_DeviceDesc[10] = LOBYTE(PBDRV_CONFIG_USB_PID_0);
        USBD_DeviceDesc[11] = HIBYTE(PBDRV_CONFIG_USB_PID_0);
    } else if (VARIANT == 1) {
        USBD_DeviceDesc[10] = LOBYTE(PBDRV_CONFIG_USB_PID_1);
        USBD_DeviceDesc[11] = HIBYTE(PBDRV_CONFIG_USB_PID_1);
    }
    #endif

    const char *str;
    size_t len;

    uint8_t *ptr = &USBD_BOSDesc[USB_SIZ_BOS_DESC_CONST];

    /* Add device name */
    str = pbdrv_bluetooth_get_hub_name();
    len = MIN(strlen(str), USB_SIZ_HUB_NAME_MAX);

    *ptr++ = USB_SIZ_PLATFORM_HDR + len; // bLength
    *ptr++ = USB_DEVICE_CAPABITY_TYPE;   // bDescriptorType
    *ptr++ = USB_DEV_CAP_TYPE_PLATFORM;  // bDevCapabilityType
    *ptr++ = 0x00;                       // bReserved

    // PlatformCapabilityUUID
    add_ble_short_uuid_le(ptr, pbio_gatt_device_name_char_uuid);
    ptr += USB_SIZ_UUID;

    // CapabilityData: Device Name
    memcpy(ptr, str, len);
    ptr += len;

    /* Add firmware version */
    *ptr++ = USB_SIZ_PLATFORM_HDR + sizeof(firmware_version) - 1; // bLength
    *ptr++ = USB_DEVICE_CAPABITY_TYPE;                            // bDescriptorType
    *ptr++ = USB_DEV_CAP_TYPE_PLATFORM;                           // bDevCapabilityType
    *ptr++ = 0x00;                                                // bReserved

    // PlatformCapabilityUUID
    add_ble_short_uuid_le(ptr, pbio_gatt_firmware_version_char_uuid);
    ptr += USB_SIZ_UUID;

    // CapabilityData: Firmware Version
    memcpy(ptr, firmware_version, sizeof(firmware_version) - 1);
    ptr += sizeof(firmware_version) - 1;

    /* Add software (protocol) version */
    *ptr++ = USB_SIZ_PLATFORM_HDR + sizeof(software_version) - 1; // bLength
    *ptr++ = USB_DEVICE_CAPABITY_TYPE;                            // bDescriptorType
    *ptr++ = USB_DEV_CAP_TYPE_PLATFORM;                           // bDevCapabilityType
    *ptr++ = 0x00;                                                // bReserved

    // PlatformCapabilityUUID
    add_ble_short_uuid_le(ptr, pbio_gatt_software_version_char_uuid);
    ptr += USB_SIZ_UUID;

    // CapabilityData: Software Version
    memcpy(ptr, software_version, sizeof(software_version) - 1);
    ptr += sizeof(software_version) - 1;

    /* Add hub capabilities */
    *ptr++ = USB_SIZ_PLATFORM_HDR + PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE; // bLength
    *ptr++ = USB_DEVICE_CAPABITY_TYPE;                                         // bDescriptorType
    *ptr++ = USB_DEV_CAP_TYPE_PLATFORM;                                        // bDevCapabilityType
    *ptr++ = 0x00;                                                             // bReserved

    // PlatformCapabilityUUID
    pbio_uuid128_le_copy(ptr, pbio_pybricks_hub_capabilities_char_uuid);
    ptr += USB_SIZ_UUID;

    // CapabilityData: Hub Capabilities
    pbio_pybricks_hub_capabilities(ptr,
        USBD_PYBRICKS_MAX_PACKET_SIZE - 1,
        PBSYS_APP_HUB_FEATURE_FLAGS,
        PBSYS_PROGRAM_LOAD_MAX_PROGRAM_SIZE);
    ptr += PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE;

    /* Update wTotalLength field in BOS Descriptor */
    USBD_BOSDesc_Len = ptr - USBD_BOSDesc;
    USBD_BOSDesc[2] = LOBYTE(USBD_BOSDesc_Len);
    USBD_BOSDesc[3] = HIBYTE(USBD_BOSDesc_Len);

    /* Update bNumDeviceCaps field in BOS Descriptor */
    USBD_BOSDesc[4] += 4;
}
