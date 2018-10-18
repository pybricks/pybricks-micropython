/******************** (C) COPYRIGHT 2014 STMicroelectronics ********************
* File Name          : bluenrg_updater_aci.h
* Author             : AMS - HEA&RF BU
* Version            : V1.0.0
* Date               : 26-Jun-2014
* Description        : Header file with updater commands for BlueNRG FW6.3.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#ifndef __BLUENRG_UPDATER_ACI_H__
#define __BLUENRG_UPDATER_ACI_H__

#include "bluenrg_types.h"

/** 
 * @addtogroup HIGH_LEVEL_INTERFACE HIGH_LEVEL_INTERFACE
 * @{
 */

/**
 * @addtogroup ACI_UPDATER ACI_UPDATER
 * @brief API for UPDATER layer.
 * @{
 */
 
/**
 * @defgroup Updater_Functions Updater functions
 * @brief API for BlueNRG Updater.
 * @{
 */

tBleStatus aci_updater_start(void);

tBleStatus aci_updater_reboot(void);

tBleStatus aci_get_updater_version(uint8_t *version);

tBleStatus aci_get_updater_buffer_size(uint8_t *buffer_size);

tBleStatus aci_erase_blue_flag(void);

tBleStatus aci_reset_blue_flag(void);

tBleStatus aci_updater_erase_sector(uint32_t address);

tBleStatus aci_updater_program_data_block(uint32_t address, uint16_t len, const uint8_t *data);

tBleStatus aci_updater_read_data_block(uint32_t address, uint16_t data_len, uint8_t *data);

tBleStatus aci_updater_calc_crc(uint32_t address, uint8_t num_sectors, uint32_t *crc);

tBleStatus aci_updater_hw_version(uint8_t *version);

/**
 * @}
 */

/**
 * @defgroup Updater_Events Updater events
 * @{
 */
/** HCI vendor specific event, raised at BlueNRG power-up or reboot. */
#define EVT_BLUE_INITIALIZED                      (0x0001)
typedef __packed struct _evt_blue_initialized{
  uint8_t reason_code;
} PACKED evt_blue_initialized;
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* __BLUENRG_UPDATER_ACI_H__ */
