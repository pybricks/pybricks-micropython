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

#include <lego_usb.h>

#include <pbdrv/config.h>
#include <pbio/protocol.h>

#include "usbd_core.h"
#include "usbd_conf.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define USBD_LANGID_STRING            0x409
#define USBD_CONFIGURATION_FS_STRING  "Pybricks Config"
#define USBD_INTERFACE_FS_STRING      "Pybricks Interface"

#define         DEVICE_ID1          (0x1FFF7A10)
#define         DEVICE_ID2          (0x1FFF7A14)
#define         DEVICE_ID3          (0x1FFF7A18)

#define  USB_SIZ_STRING_SERIAL       0x1A

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static
#if !defined(PBDRV_CONFIG_USB_STM32F4_HUB_VARIANT_ADDR)
const
#endif
uint8_t USBD_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12,                     /* bLength */
    USB_DESC_TYPE_DEVICE,     /* bDescriptorType */
    0x00,                     /* bcdUSB */
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

USBD_DescriptorsTypeDef USBD_Pybricks_Desc = {
    .GetDeviceDescriptor = USBD_Pybricks_DeviceDescriptor,
    .GetLangIDStrDescriptor = USBD_Pybricks_LangIDStrDescriptor,
    .GetManufacturerStrDescriptor = USBD_Pybricks_ManufacturerStrDescriptor,
    .GetProductStrDescriptor = USBD_Pybricks_ProductStrDescriptor,
    .GetSerialStrDescriptor = USBD_Pybricks_SerialStrDescriptor,
    .GetConfigurationStrDescriptor = USBD_Pybricks_ConfigStrDescriptor,
    .GetInterfaceStrDescriptor = USBD_Pybricks_InterfaceStrDescriptor,
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
}
