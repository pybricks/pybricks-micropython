/**
  ******************************************************************************
  * @file    usbd_pybricks.h
  * @author  MCD Application Team
  * @brief   Header file for the usbd_pybricks.c file.
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
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_PYBRICKS_H
#define __USB_PYBRICKS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_Pybricks
  * @brief This file is the header file for usbd_pybricks.c
  * @{
  */


/** @defgroup USBD_Pybricks_Exported_Defines
  * @{
  */
#define USBD_MS_VENDOR_CODE                0xCC
#define USBD_SIZ_MS_OS_DSCRPTR_SET         (10 + 20 + 130)

#define USBD_PYBRICKS_CONFIG_DESC_SIZ      (9 + 9 + 7 + 7)

#define USBD_PYBRICKS_IN_EP                0x81U  /* EP1 for data IN */
#define USBD_PYBRICKS_OUT_EP               0x01U  /* EP1 for data OUT */

#define USBD_PYBRICKS_MAX_PACKET_SIZE      64U

/**
  * @}
  */


/** @defgroup USBD_Pybricks_Exported_TypesDefinitions
  * @{
  */

/**
  * @}
  */
typedef struct
{
    int8_t (*Init)(void);
    int8_t (*DeInit)(void);
    int8_t (*Receive)(uint8_t *Buf, uint32_t *Len);
    int8_t (*TransmitCplt)(uint8_t *Buf, uint32_t *Len, uint8_t epnum);
} USBD_Pybricks_ItfTypeDef;


typedef struct
{
    uint32_t data[USBD_PYBRICKS_MAX_PACKET_SIZE / 4U]; /* Force 32bits alignment */
    uint8_t CmdOpCode;
    uint8_t CmdLength;
    uint8_t *RxBuffer;
    uint8_t *TxBuffer;
    uint32_t RxLength;
    uint32_t TxLength;

    __IO uint32_t TxState;
    __IO uint32_t RxState;
} USBD_Pybricks_HandleTypeDef;



/** @defgroup USBD_Pybricks_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_Pybricks_Exported_Variables
  * @{
  */

extern USBD_ClassTypeDef USBD_Pybricks_ClassDriver;

extern USBD_DescriptorsTypeDef Pybricks_Desc;
extern uint8_t USBD_OSDescSet[USBD_SIZ_MS_OS_DSCRPTR_SET];
/**
  * @}
  */

/** @defgroup USB_Pybricks_Exported_Functions
  * @{
  */
uint8_t USBD_Pybricks_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_Pybricks_ItfTypeDef *fops);
uint8_t USBD_Pybricks_SetTxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t length);
uint8_t USBD_Pybricks_SetRxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuf);
uint8_t USBD_Pybricks_ReceivePacket(USBD_HandleTypeDef *pdev);
uint8_t USBD_Pybricks_TransmitPacket(USBD_HandleTypeDef *pdev);
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif  /* __USB_PYBRICKS_H */
/**
  * @}
  */

/**
  * @}
  */
