/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
* File Name          : bluenrg_types.h
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
#ifndef __BLUENRG_TYPES_H__
#define __BLUENRG_TYPES_H__

#include <stdint.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 0
#define __BIG_ENDIAN    1
#endif

/* Change this define to 1 if zero-length arrays are not supported by your compiler. */
#define VARIABLE_SIZE   

/* Byte order conversions */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htobs(d)  (d)
#define htobl(d)  (d)
#define btohs(d)  (d)
#define btohl(d)  (d)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define htobs(d)  (d<<8|d>>8)
#define htobl(d)  (d<<24|((d<<8)&0x00ff0000)|((d>>8)&0x0000ff00)|((d>>24)&0x000000ff))
#define btohs(d)  (d<<8|d>>8)
#define btohl(d)  (d<<24|((d<<8)&0x00ff0000)|((d>>8)&0x0000ff00)|((d>>24)&0x000000ff))
#else
#error "Unknown byte order"
#endif

typedef uint8_t BOOL;

#ifndef TRUE 
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifdef __GNUC__
#undef __packed
#define __packed
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

#endif /* __BLUENRG_TYPES_H__ */

