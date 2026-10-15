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
#include "usbd_ctlreq.h"
#include "usbd_pybricks.h"

#include "../usb_ch9.h"


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
typedef struct PBDRV_PACKED {
    pbdrv_usb_conf_desc_t conf_desc;
    pbdrv_usb_iad_desc_t iad;
    pbdrv_usb_iface_desc_t comm_iface;
    pbdrv_usb_cdc_header_desc_t cdc_header;
    pbdrv_usb_cdc_call_mgmt_desc_t cdc_call_mgmt;
    pbdrv_usb_cdc_acm_desc_t cdc_acm;
    pbdrv_usb_cdc_union_desc_t cdc_union;
    pbdrv_usb_ep_desc_t cmd_ep;
    pbdrv_usb_iface_desc_t data_iface;
    pbdrv_usb_ep_desc_t ep_out;
    pbdrv_usb_ep_desc_t ep_in;
} pbdrv_usb_stm32_conf_t;
PBDRV_USB_TYPE_PUNNING_HELPER(pbdrv_usb_stm32_conf);

static pbdrv_usb_stm32_conf_union_t USBD_Pybricks_CfgDesc = {
    .s = {
        .conf_desc = {
            .bLength = sizeof(pbdrv_usb_conf_desc_t),
            .bDescriptorType = DESC_TYPE_CONFIGURATION,
            .wTotalLength = sizeof(pbdrv_usb_stm32_conf_t),
            .bNumInterfaces = 2,
            .bConfigurationValue = 1,
            .iConfiguration = 0,
            .bmAttributes = USB_CONF_DESC_BM_ATTR_MUST_BE_SET,
            .bMaxPower = 250,   /* 500mA (number of 2mA units) */
        },
        /* Interface Association: groups the comm and data interfaces into one
         * CDC ACM function. */
        .iad = {
            .bLength = sizeof(pbdrv_usb_iad_desc_t),
            .bDescriptorType = DESC_TYPE_INTERFACE_ASSOCIATION,
            .bFirstInterface = 0,
            .bInterfaceCount = 2,
            .bFunctionClass = USB_CLASS_CDC,
            .bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
            .bFunctionProtocol = USB_CDC_PROTOCOL_AT,
            .iFunction = 0,
        },
        /* Communication interface */
        .comm_iface = {
            .bLength = sizeof(pbdrv_usb_iface_desc_t),
            .bDescriptorType = DESC_TYPE_INTERFACE,
            .bInterfaceNumber = 0,
            .bAlternateSetting = 0,
            .bNumEndpoints = 1,
            .bInterfaceClass = USB_CLASS_CDC,
            .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
            .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
            .iInterface = 0,
        },
        .cdc_header = {
            .bFunctionLength = sizeof(pbdrv_usb_cdc_header_desc_t),
            .bDescriptorType = USB_CDC_CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_FUNC_SUBTYPE_HEADER,
            .bcdCDC = 0x0110,
        },
        .cdc_call_mgmt = {
            .bFunctionLength = sizeof(pbdrv_usb_cdc_call_mgmt_desc_t),
            .bDescriptorType = USB_CDC_CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_FUNC_SUBTYPE_CALL_MGMT,
            .bmCapabilities = 0x00,
            .bDataInterface = 1,
        },
        .cdc_acm = {
            .bFunctionLength = sizeof(pbdrv_usb_cdc_acm_desc_t),
            .bDescriptorType = USB_CDC_CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_FUNC_SUBTYPE_ACM,
            .bmCapabilities = 0x02,
        },
        .cdc_union = {
            .bFunctionLength = sizeof(pbdrv_usb_cdc_union_desc_t),
            .bDescriptorType = USB_CDC_CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_FUNC_SUBTYPE_UNION,
            .bControlInterface = 0,
            .bSubordinateInterface0 = 1,
        },
        .cmd_ep = {
            .bLength = sizeof(pbdrv_usb_ep_desc_t),
            .bDescriptorType = DESC_TYPE_ENDPOINT,
            .bEndpointAddress = USBD_PYBRICKS_CMD_EP,
            .bmAttributes = PBDRV_USB_EP_TYPE_INTR,
            .wMaxPacketSize = USBD_PYBRICKS_CMD_PACKET_SIZE,
            .bInterval = 16,
        },
        /* Data interface */
        .data_iface = {
            .bLength = sizeof(pbdrv_usb_iface_desc_t),
            .bDescriptorType = DESC_TYPE_INTERFACE,
            .bInterfaceNumber = 1,
            .bAlternateSetting = 0,
            .bNumEndpoints = 2,
            .bInterfaceClass = USB_CLASS_CDC_DATA,
            .bInterfaceSubClass = 0,
            .bInterfaceProtocol = 0,
            .iInterface = 0,
        },
        .ep_out = {
            .bLength = sizeof(pbdrv_usb_ep_desc_t),
            .bDescriptorType = DESC_TYPE_ENDPOINT,
            .bEndpointAddress = USBD_PYBRICKS_OUT_EP,
            .bmAttributes = PBDRV_USB_EP_TYPE_BULK,
            .wMaxPacketSize = USBD_PYBRICKS_MAX_PACKET_SIZE,
            .bInterval = 0,     /* ignore for Bulk transfer */
        },
        .ep_in = {
            .bLength = sizeof(pbdrv_usb_ep_desc_t),
            .bDescriptorType = DESC_TYPE_ENDPOINT,
            .bEndpointAddress = USBD_PYBRICKS_IN_EP,
            .bmAttributes = PBDRV_USB_EP_TYPE_BULK,
            .wMaxPacketSize = USBD_PYBRICKS_MAX_PACKET_SIZE,
            .bInterval = 0,     /* ignore for Bulk transfer */
        },
    }
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

    /* Default line coding reported to the host: 115200 baud, 1 stop bit, no
     * parity, 8 data bits. This is inert: USB does not transfer data at this
     * rate (it is not a real UART). CDC ACM just requires valid line coding to
     * be stored and echoed back on GET_LINE_CODING. */
    hPybricks.LineCoding[0] = 0x00;
    hPybricks.LineCoding[1] = 0xC2;
    hPybricks.LineCoding[2] = 0x01;
    hPybricks.LineCoding[3] = 0x00;
    hPybricks.LineCoding[4] = 0x00;
    hPybricks.LineCoding[5] = 0x00;
    hPybricks.LineCoding[6] = 0x08;
    hPybricks.CmdOpCode = 0xFFU;

    (void)USBD_LL_OpenEP(pdev, USBD_PYBRICKS_IN_EP, USBD_EP_TYPE_BULK, USBD_PYBRICKS_MAX_PACKET_SIZE);
    pdev->ep_in[USBD_PYBRICKS_IN_EP & 0xFU].is_used = 1U;

    (void)USBD_LL_OpenEP(pdev, USBD_PYBRICKS_OUT_EP, USBD_EP_TYPE_BULK, USBD_PYBRICKS_MAX_PACKET_SIZE);
    pdev->ep_out[USBD_PYBRICKS_OUT_EP & 0xFU].is_used = 1U;

    (void)USBD_LL_OpenEP(pdev, USBD_PYBRICKS_CMD_EP, USBD_EP_TYPE_INTR, USBD_PYBRICKS_CMD_PACKET_SIZE);
    pdev->ep_in[USBD_PYBRICKS_CMD_EP & 0xFU].is_used = 1U;

    /* Init  physical Interface components */
    ((USBD_Pybricks_ItfTypeDef *)pdev->pUserData[pdev->classId])->Init();

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

    /* Close command EP */
    (void)USBD_LL_CloseEP(pdev, USBD_PYBRICKS_CMD_EP);
    pdev->ep_in[USBD_PYBRICKS_CMD_EP & 0xFU].is_used = 0U;

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
    USBD_Pybricks_HandleTypeDef *hPybricks = pdev->pClassData;
    uint8_t ifalt = 0U;
    uint16_t status_info = 0U;
    USBD_StatusTypeDef ret = USBD_OK;

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
        case USB_REQ_TYPE_CLASS:
            if (hPybricks == NULL) {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
                break;
            }
            switch (req->bRequest)
            {
                case USB_CDC_REQ_SET_LINE_CODING:
                    /* Receive the line coding into the handle. The value is
                     * accepted but not acted on (the link is not a real UART). */
                    if (req->wLength == USB_CDC_LINE_CODING_SIZE) {
                        hPybricks->CmdOpCode = (uint8_t)req->bRequest;
                        hPybricks->CmdLength = USB_CDC_LINE_CODING_SIZE;
                        (void)USBD_CtlPrepareRx(pdev, hPybricks->LineCoding, USB_CDC_LINE_CODING_SIZE);
                    } else {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                case USB_CDC_REQ_GET_LINE_CODING:
                    (void)USBD_CtlSendData(pdev, hPybricks->LineCoding,
                        MIN(USB_CDC_LINE_CODING_SIZE, req->wLength));
                    break;

                case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
                    /* The DTR bit indicates whether a host application has
                     * opened the port. This is the USB analog of a BLE host
                     * subscribing to notifications. */
                    ((USBD_Pybricks_ItfTypeDef *)pdev->pUserData[pdev->classId])->SetControlLineState(
                        (req->wValue & USB_CDC_CONTROL_LINE_STATE_DTR) != 0U);
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
    *length = (uint16_t)sizeof(USBD_Pybricks_CfgDesc.s);
    return (uint8_t *)&USBD_Pybricks_CfgDesc;
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
        ((USBD_Pybricks_ItfTypeDef *)pdev->pUserData[pdev->classId])->TransmitCplt(hPybricks->TxBuffer, hPybricks->TxLength, epnum);
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

    ((USBD_Pybricks_ItfTypeDef *)pdev->pUserData[pdev->classId])->Receive(hPybricks->RxBuffer, hPybricks->RxLength);

    return USBD_OK;
}

/**
  * @brief  USBD_Pybricks_EP0_RxReady
  *         Handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static USBD_StatusTypeDef USBD_Pybricks_EP0_RxReady(USBD_HandleTypeDef *pdev) {
    USBD_Pybricks_HandleTypeDef *hPybricks = pdev->pClassData;

    if (hPybricks != NULL && hPybricks->CmdOpCode != 0xFFU) {
        /* SET_LINE_CODING data has been received into hPybricks->LineCoding.
         * Nothing to do; the value is stored for GET_LINE_CODING. */
        hPybricks->CmdOpCode = 0xFFU;
    }

    return USBD_OK;
}

/**
* @brief  USBD_Pybricks_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
USBD_StatusTypeDef USBD_Pybricks_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_Pybricks_ItfTypeDef *fops) {
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

    if (hPybricks == NULL) {
        return USBD_FAIL;
    }

    hPybricks->TxBuffer = pbuf;
    hPybricks->TxLength = length;

    /* Update the packet total length */
    pdev->ep_in[USBD_PYBRICKS_IN_EP & 0xFU].total_length = hPybricks->TxLength;

    /* Transmit next packet */
    (void)USBD_LL_Transmit(pdev, USBD_PYBRICKS_IN_EP, hPybricks->TxBuffer, hPybricks->TxLength);

    return USBD_OK;
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
