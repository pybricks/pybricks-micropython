/**
  ******************************************************************************
  * @file    usbd_pybricks.c
  * @author  MCD Application Team
  * @brief   This file provides the HID core functions.
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  * @verbatim
  *
  *          ===================================================================
  *                                Pybricks Class  Description
  *          ===================================================================
  *
  *
  *
  *
  *
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <pbio/protocol.h>

#include "usbd_ctlreq.h"
#include "usbd_pybricks.h"


/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_Pybricks
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_Pybricks_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_Pybricks_Private_Defines
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_Pybricks_Private_Macros
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_Pybricks_Private_FunctionPrototypes
  * @{
  */

static USBD_StatusTypeDef USBD_Pybricks_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static USBD_StatusTypeDef USBD_Pybricks_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static USBD_StatusTypeDef USBD_Pybricks_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static USBD_StatusTypeDef USBD_Pybricks_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static USBD_StatusTypeDef USBD_Pybricks_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static USBD_StatusTypeDef USBD_Pybricks_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t *USBD_Pybricks_GetCfgDesc(uint16_t *length);

/**
  * @}
  */

/** @defgroup USBD_Pybricks_Private_Variables
  * @{
  */

USBD_ClassTypeDef USBD_Pybricks_ClassDriver =
{
    .Init = USBD_Pybricks_Init,
    .DeInit = USBD_Pybricks_DeInit,
    .Setup = USBD_Pybricks_Setup,
    .EP0_RxReady = USBD_Pybricks_EP0_RxReady,
    .DataIn = USBD_Pybricks_DataIn,
    .DataOut = USBD_Pybricks_DataOut,
    .GetFSConfigDescriptor = USBD_Pybricks_GetCfgDesc,
};

/* USB Pybricks device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_Pybricks_CfgDesc[USBD_PYBRICKS_CONFIG_DESC_SIZ] __ALIGN_END =
{
    /* Configuration Descriptor */
    0x09,                                     /* bLength: Configuration Descriptor size */
    USB_DESC_TYPE_CONFIGURATION,              /* bDescriptorType: Configuration */
    LOBYTE(USBD_PYBRICKS_CONFIG_DESC_SIZ),    /* wTotalLength:no of returned bytes */
    HIBYTE(USBD_PYBRICKS_CONFIG_DESC_SIZ),
    0x01,                                     /* bNumInterfaces: 1 interface */
    0x01,                                     /* bConfigurationValue: Configuration value */
    0x00,                                     /* iConfiguration: Index of string descriptor describing the configuration */
    0x80,                                     /* bmAttributes */
    250,                                      /* MaxPower 500mA (number of 2mA units) */

    /*---------------------------------------------------------------------------*/

    /* Data class interface descriptor */
    0x09,                                     /* bLength: Endpoint Descriptor size */
    USB_DESC_TYPE_INTERFACE,                  /* bDescriptorType: */
    0x00,                                     /* bInterfaceNumber: Number of Interface */
    0x00,                                     /* bAlternateSetting: Alternate setting */
    0x02,                                     /* bNumEndpoints: Two endpoints used */
    PBIO_PYBRICKS_USB_DEVICE_CLASS,           /* bInterfaceClass */
    PBIO_PYBRICKS_USB_DEVICE_SUBCLASS,        /* bInterfaceSubClass */
    PBIO_PYBRICKS_USB_DEVICE_PROTOCOL,        /* bInterfaceProtocol */
    0x00,                                     /* iInterface: */

    /* Endpoint OUT Descriptor */
    0x07,                                     /* bLength: Endpoint Descriptor size */
    USB_DESC_TYPE_ENDPOINT,                   /* bDescriptorType: Endpoint */
    USBD_PYBRICKS_OUT_EP,                     /* bEndpointAddress */
    USBD_EP_TYPE_BULK,                        /* bmAttributes */
    LOBYTE(USBD_PYBRICKS_MAX_PACKET_SIZE),    /* wMaxPacketSize: */
    HIBYTE(USBD_PYBRICKS_MAX_PACKET_SIZE),
    0x00,                                     /* bInterval: ignore for Bulk transfer */

    /* Endpoint IN Descriptor */
    0x07,                                     /* bLength: Endpoint Descriptor size */
    USB_DESC_TYPE_ENDPOINT,                   /* bDescriptorType: Endpoint */
    USBD_PYBRICKS_IN_EP,                      /* bEndpointAddress */
    USBD_EP_TYPE_BULK,                        /* bmAttributes */
    LOBYTE(USBD_PYBRICKS_MAX_PACKET_SIZE),    /* wMaxPacketSize: */
    HIBYTE(USBD_PYBRICKS_MAX_PACKET_SIZE),
    0x00                                      /* bInterval */
};

__ALIGN_BEGIN static const uint8_t WebUSB_DescSet[20] __ALIGN_END =
{
    20,    /* bLength */
    0x03,  /* bDescriptorType = URL */
    0x01,  /* bScheme = https:// */

    /* URL */
    'c', 'o', 'd', 'e', '.',
    'p', 'y', 'b', 'r', 'i', 'c', 'k', 's', '.',
    'c', 'o', 'm'
};

/**
  * @}
  */

/** @defgroup USBD_Pybricks_Private_Functions
  * @{
  */

/**
  * @brief  USBD_Pybricks_Init
  *         Initialize the Pybricks interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static USBD_StatusTypeDef USBD_Pybricks_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    UNUSED(cfgidx);
    static USBD_Pybricks_HandleTypeDef hPybricks;

    pdev->pClassData = &hPybricks;

    (void)USBD_LL_OpenEP(pdev, USBD_PYBRICKS_IN_EP, USBD_EP_TYPE_BULK, USBD_PYBRICKS_MAX_PACKET_SIZE);
    pdev->ep_in[USBD_PYBRICKS_IN_EP & 0xFU].is_used = 1U;

    (void)USBD_LL_OpenEP(pdev, USBD_PYBRICKS_OUT_EP, USBD_EP_TYPE_BULK, USBD_PYBRICKS_MAX_PACKET_SIZE);
    pdev->ep_out[USBD_PYBRICKS_OUT_EP & 0xFU].is_used = 1U;

    /* Init  physical Interface components */
    ((USBD_Pybricks_ItfTypeDef *)pdev->pUserData[pdev->classId])->Init();

    /* Init Xfer states */
    hPybricks.TxState = 0U;
    hPybricks.RxState = 0U;

    (void)USBD_LL_PrepareReceive(pdev, USBD_PYBRICKS_OUT_EP, hPybricks.RxBuffer, USBD_PYBRICKS_MAX_PACKET_SIZE);

    return USBD_OK;
}

/**
  * @brief  USBD_Pybricks_DeInit
  *         DeInitialize the Pybricks layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static USBD_StatusTypeDef USBD_Pybricks_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    UNUSED(cfgidx);

    /* Close EP IN */
    (void)USBD_LL_CloseEP(pdev, USBD_PYBRICKS_IN_EP);
    pdev->ep_in[USBD_PYBRICKS_IN_EP & 0xFU].is_used = 0U;

    /* Close EP OUT */
    (void)USBD_LL_CloseEP(pdev, USBD_PYBRICKS_OUT_EP);
    pdev->ep_out[USBD_PYBRICKS_OUT_EP & 0xFU].is_used = 0U;

    /* DeInit  physical Interface components */
    if (pdev->pClassData != NULL) {
        ((USBD_Pybricks_ItfTypeDef *)pdev->pUserData[pdev->classId])->DeInit();
        pdev->pClassData = NULL;
    }

    return USBD_OK;
}

/**
  * @brief  USBD_Pybricks_Setup
  *         Handle the Pybricks specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static USBD_StatusTypeDef USBD_Pybricks_Setup(USBD_HandleTypeDef *pdev,
    USBD_SetupReqTypedef *req) {
    uint8_t ifalt = 0U;
    uint16_t status_info = 0U;
    USBD_StatusTypeDef ret = USBD_OK;

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
        case USB_REQ_TYPE_CLASS:
            break;

        case USB_REQ_TYPE_VENDOR:
            switch (req->bRequest)
            {
                case USBD_MS_VENDOR_CODE:
                    (void)USBD_CtlSendData(pdev,
                        (uint8_t *)USBD_OSDescSet,
                        MIN(sizeof(USBD_OSDescSet), req->wLength));
                    break;

                case USBD_WEBUSB_VENDOR_CODE:
                    if ((req->wValue == USBD_WEBUSB_LANDING_PAGE_IDX) && (req->wIndex == 0x02)) {
                        (void)USBD_CtlSendData(pdev,
                            (uint8_t *)WebUSB_DescSet,
                            MIN(sizeof(WebUSB_DescSet), req->wLength));
                    }
                    break;

                default:
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                    break;
            }
            break;

        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest)
            {
                case USB_REQ_GET_STATUS:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                        (void)USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
                    } else {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                case USB_REQ_GET_INTERFACE:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                        (void)USBD_CtlSendData(pdev, &ifalt, 1U);
                    } else {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                case USB_REQ_SET_INTERFACE:
                    if (pdev->dev_state != USBD_STATE_CONFIGURED) {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                case USB_REQ_CLEAR_FEATURE:
                    break;

                default:
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                    break;
            }
            break;

        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
    }

    return ret;
}

/**
  * @brief  USBD_Pybricks_GetCfgDesc
  *         return configuration descriptor
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t *USBD_Pybricks_GetCfgDesc(uint16_t *length) {
    *length = (uint16_t)sizeof(USBD_Pybricks_CfgDesc);
    return USBD_Pybricks_CfgDesc;
}

/**
  * @brief  USBD_Pybricks_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static USBD_StatusTypeDef USBD_Pybricks_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    USBD_Pybricks_HandleTypeDef *hPybricks = pdev->pClassData;
    PCD_HandleTypeDef *hpcd = pdev->pData;

    if (hPybricks == NULL) {
        return USBD_FAIL;
    }

    if ((pdev->ep_in[epnum].total_length > 0U) &&
        ((pdev->ep_in[epnum].total_length % hpcd->IN_ep[epnum].maxpacket) == 0U)) {
        /* Update the packet total length */
        pdev->ep_in[epnum].total_length = 0U;

        /* Send ZLP */
        (void)USBD_LL_Transmit(pdev, epnum, NULL, 0U);
    } else {
        hPybricks->TxState = 0U;
        ((USBD_Pybricks_ItfTypeDef *)pdev->pUserData[pdev->classId])->TransmitCplt(hPybricks->TxBuffer, &hPybricks->TxLength, epnum);
    }

    return USBD_OK;
}

/**
  * @brief  USBD_Pybricks_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static USBD_StatusTypeDef USBD_Pybricks_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    USBD_Pybricks_HandleTypeDef *hPybricks = pdev->pClassData;

    if (hPybricks == NULL) {
        return USBD_FAIL;
    }

    /* Get the received data length */
    hPybricks->RxLength = USBD_LL_GetRxDataSize(pdev, epnum);

    /* USB data will be immediately processed, this allow next USB traffic being
    NAKed till the end of the application Xfer */

    ((USBD_Pybricks_ItfTypeDef *)pdev->pUserData[pdev->classId])->Receive(hPybricks->RxBuffer, &hPybricks->RxLength);

    return USBD_OK;
}

/**
  * @brief  USBD_Pybricks_EP0_RxReady
  *         Handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static USBD_StatusTypeDef USBD_Pybricks_EP0_RxReady(USBD_HandleTypeDef *pdev) {
    return USBD_OK;
}

/**
* @brief  USBD_Pybricks_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
USBD_StatusTypeDef USBD_Pybricks_RegisterInterface(USBD_HandleTypeDef *pdev,
    USBD_Pybricks_ItfTypeDef *fops) {
    if (fops == NULL) {
        return USBD_FAIL;
    }

    pdev->pUserData[pdev->classId] = fops;

    return USBD_OK;
}

/**
  * @brief  USBD_Pybricks_SetRxBuffer
  * @param  pdev: device instance
  * @param  pbuff: Rx Buffer
  * @retval status
  */
USBD_StatusTypeDef USBD_Pybricks_SetRxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff) {
    USBD_Pybricks_HandleTypeDef *hPybricks = pdev->pClassData;

    if (hPybricks == NULL) {
        return USBD_FAIL;
    }

    hPybricks->RxBuffer = pbuff;

    return USBD_OK;
}

/**
  * @brief  USBD_Pybricks_TransmitPacket
  *         Transmit packet on IN endpoint
  * @param  pdev: device instance
  * @retval status
  */
USBD_StatusTypeDef USBD_Pybricks_TransmitPacket(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t length) {
    USBD_Pybricks_HandleTypeDef *hPybricks = pdev->pClassData;
    USBD_StatusTypeDef ret = USBD_BUSY;

    if (hPybricks == NULL) {
        return USBD_FAIL;
    }

    if (hPybricks->TxState == 0U) {
        /* Tx Transfer in progress */
        hPybricks->TxState = 1U;

        hPybricks->TxBuffer = pbuf;
        hPybricks->TxLength = length;

        /* Update the packet total length */
        pdev->ep_in[USBD_PYBRICKS_IN_EP & 0xFU].total_length = hPybricks->TxLength;

        /* Transmit next packet */
        (void)USBD_LL_Transmit(pdev, USBD_PYBRICKS_IN_EP, hPybricks->TxBuffer, hPybricks->TxLength);

        ret = USBD_OK;
    }

    return ret;
}


/**
  * @brief  USBD_Pybricks_ReceivePacket
  *         prepare OUT Endpoint for reception
  * @param  pdev: device instance
  * @retval status
  */
USBD_StatusTypeDef USBD_Pybricks_ReceivePacket(USBD_HandleTypeDef *pdev) {
    USBD_Pybricks_HandleTypeDef *hPybricks = pdev->pClassData;

    if (hPybricks == NULL) {
        return USBD_FAIL;
    }

    /* Prepare Out endpoint to receive next packet */
    (void)USBD_LL_PrepareReceive(pdev, USBD_PYBRICKS_OUT_EP, hPybricks->RxBuffer, USBD_PYBRICKS_MAX_PACKET_SIZE);

    return USBD_OK;
}

/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */
