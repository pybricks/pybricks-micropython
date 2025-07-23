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

#include <lego/usb.h>

#include <pbdrv/config.h>
#include <pbio/protocol.h>

#include "usbd_core.h"
#include "usbd_conf.h"
#include "usbd_pybricks.h"

#include "../usb_ch9.h"
#include "../usb_common_desc.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define USBD_LANGID_STRING            0x409

// STM32 MCU Device ID register addresses
// REVISIT: make pbdrv_xxx_get_serial_number() and use that instead
#define         DEVICE_ID1          (0x1FFF7A10)
#define         DEVICE_ID2          (0x1FFF7A14)
#define         DEVICE_ID3          (0x1FFF7A18)

// bDevCapabilityType for USB_DEVICE_CAPABITY_TYPE
#define USB_DEV_CAP_TYPE_PLATFORM   (5)

// descriptor sizes
#define USB_SIZ_STRING_SERIAL       26

/* USB Standard Device Descriptor */
static
#if !defined(PBDRV_CONFIG_USB_STM32F4_HUB_VARIANT_ADDR)
const
#endif
pbdrv_usb_dev_desc_union_t USBD_DeviceDesc = {
    .s = {
        .bLength = sizeof(pbdrv_usb_dev_desc_t),
        .bDescriptorType = DESC_TYPE_DEVICE,
        .bcdUSB = 0x0210,       /* 2.1.0 (for BOS support) */
        .bDeviceClass = PBIO_PYBRICKS_USB_DEVICE_CLASS,
        .bDeviceSubClass = PBIO_PYBRICKS_USB_DEVICE_SUBCLASS,
        .bDeviceProtocol = PBIO_PYBRICKS_USB_DEVICE_PROTOCOL,
        .bMaxPacketSize0 = USB_MAX_EP0_SIZE,
        .idVendor = PBDRV_CONFIG_USB_VID,
        .idProduct = PBDRV_CONFIG_USB_PID,
        .bcdDevice = 0x0200,    /* rel. 2.0.0 */
        .iManufacturer = USBD_IDX_MFC_STR,
        .iProduct = USBD_IDX_PRODUCT_STR,
        .iSerialNumber = USBD_IDX_SERIAL_STR,
        .bNumConfigurations = USBD_MAX_NUM_CONFIGURATION,
    }
}; /* USB_DeviceDescriptor */

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] __ALIGN_END = {
    USB_SIZ_STRING_SERIAL,
    USB_DESC_TYPE_STRING,
};


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

    *length = sizeof(USBD_DeviceDesc.s);
    return (uint8_t *)&USBD_DeviceDesc;
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

    *length = sizeof(pbdrv_usb_str_desc_langid.s);
    return (uint8_t *)&pbdrv_usb_str_desc_langid;
}

/**
  * @brief  Returns the product string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_Pybricks_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    /* Prevent unused argument(s) compilation warning */
    UNUSED(speed);

    *length = sizeof(pbdrv_usb_str_desc_prod.s);
    return (uint8_t *)&pbdrv_usb_str_desc_prod;
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

    *length = sizeof(pbdrv_usb_str_desc_mfg.s);
    return (uint8_t *)&pbdrv_usb_str_desc_mfg;
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

static uint8_t *USBD_Pybricks_BOSDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    /* Prevent unused argument(s) compilation warning */
    UNUSED(speed);

    *length = sizeof(pbdrv_usb_bos_desc_set.s);
    return (uint8_t *)&pbdrv_usb_bos_desc_set;
}

USBD_DescriptorsTypeDef USBD_Pybricks_Desc = {
    .GetDeviceDescriptor = USBD_Pybricks_DeviceDescriptor,
    .GetLangIDStrDescriptor = USBD_Pybricks_LangIDStrDescriptor,
    .GetManufacturerStrDescriptor = USBD_Pybricks_ManufacturerStrDescriptor,
    .GetProductStrDescriptor = USBD_Pybricks_ProductStrDescriptor,
    .GetSerialStrDescriptor = USBD_Pybricks_SerialStrDescriptor,
    .GetBOSDescriptor = USBD_Pybricks_BOSDescriptor,
};

void USBD_Pybricks_Desc_Init(void) {
    // the same firmware runs on both SPIKE Prime and Robot Inventor so we need
    // to dynamically set the PID based on the variant
    #ifdef PBDRV_CONFIG_USB_STM32F4_HUB_VARIANT_ADDR
    #define VARIANT (*(uint32_t *)PBDRV_CONFIG_USB_STM32F4_HUB_VARIANT_ADDR)
    if (VARIANT == 0) {
        USBD_DeviceDesc.s.idProduct = PBDRV_CONFIG_USB_PID_0;
    } else if (VARIANT == 1) {
        USBD_DeviceDesc.s.idProduct = PBDRV_CONFIG_USB_PID_1;
    }
    #endif
}
