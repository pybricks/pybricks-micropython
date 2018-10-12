/**
******************************************************************************
* @file    hci.h
* @author  AMG RF FW team
* @version V1.1.0
* @date    18-July-2016
* @brief   Header file for framework required for handling HCI interface
******************************************************************************
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics</center></h2>
  * All rights reserved.
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
#ifndef __HCI_H_
#define __HCI_H_

#include "bluenrg_types.h"

/** 
 * @addtogroup HIGH_LEVEL_INTERFACE HIGH_LEVEL_INTERFACE
 * @{
 */

/**
 * @defgroup HCI_TL HCI_TL
 * @{
 */

/** 
 * @defgroup HCI_TL_functions HCI_TL functions
 * @{
 */

/**
 * @brief Initialize the Host Controller Interface. 
 *        This function must be called before any data can be received 
 *        from BLE controller.
 *
 * @param  pData: ACI events callback function pointer
 *         This callback is triggered when an user event is received from 
 *         the BLE core device. 
 * @param  pConf: Configuration structure pointer
 * @retval None
 */
void hci_init(void(* UserEvtRx)(void* pData), void* pConf);

/**
 * @brief  Processing function that must be called after an event is received from
 *         HCI interface. 
 *         It must be called outside ISR. It will call user_notify() if necessary.
 *
 * @param  None
 * @retval None
 */ 
void hci_user_evt_proc(void);

/**
 * @}
 */

/**
 * @}
 */ 

/**
 * @}
 */  
#endif /* __HCI_H_ */
