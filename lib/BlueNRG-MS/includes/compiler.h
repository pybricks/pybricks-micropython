/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
* File Name          : compiler.h
* Author             : AMS - HEA&RF BU
* Version            : V1.0.0
* Date               : 19-July-2012
* Description        : This header file defines the basic data types used by the
*                       BLE stack.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
#ifndef __COMPILER_H__
#define __COMPILER_H__

#ifdef __GNUC__
#undef __packed
#define __packed
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

#endif /* __COMPILER_H__ */

