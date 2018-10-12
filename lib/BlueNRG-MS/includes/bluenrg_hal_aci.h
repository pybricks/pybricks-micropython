/******************** (C) COPYRIGHT 2014 STMicroelectronics ********************
* File Name          : bluenrg_hal_aci.h
* Author             : AMS - AAS
* Version            : V1.0.0
* Date               : 26-Jun-2014
* Description        : Header file with HCI commands for BlueNRG
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#ifndef __BLUENRG_HAL_ACI_H__
#define __BLUENRG_HAL_ACI_H__

/** 
 * @addtogroup HIGH_LEVEL_INTERFACE HIGH_LEVEL_INTERFACE
 * @{
 */
 
/**
 *@addtogroup ACI_HAL ACI_HAL
 *@brief API for HAL layer.
 *@{
 */

/**
 * @defgroup HAL_Functions HAL functions
 * @brief API for BlueNRG HAL layer.
 * @{
 */

/**
 * @brief This command retrieves the buid number of the firmware.
 * @param[out] build_number Build number identifying the firmware release.
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_get_fw_build_number(uint16_t *build_number);

/**
 * @brief This command writes a value to a low level configure data structure.
 * @note  It is useful to setup directly some low level parameters for the system at runtime.
 * @param offset Offset in the data structure. The starting member in the data structure will have an offset 0.\n
 * 				 See @ref Config_vals.
 *
 * @param len Length of data to be written
 * @param[out] val Data to be written
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_write_config_data(uint8_t offset, 
                                    uint8_t len,
                                    const uint8_t *val);
/**
 * @brief This command requests the value in the low level configure data structure.
 *        The number of read bytes changes for different Offset.
 * @param offset Offset in the data structure. The starting member in the data structure will have an offset 0.\n
 * 				 See @ref Config_vals.
 * @param data_len Length of the data buffer
 * @param[out] data_len_out_p length of the data returned by the read.
 * @param[out] data Read data
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_read_config_data(uint8_t offset, uint16_t data_len, uint8_t *data_len_out_p, uint8_t *data);

/**
 * @brief This command sets the TX power level of the BlueNRG.
 * @note  By controlling the EN_HIGH_POWER and the PA_LEVEL, the combination of the 2 determines
 *        the output power level (dBm).
 *        When the system starts up or reboots, the default TX power level will be used, which is
 *        the maximum value of 8dBm. Once this command is given, the output power will be changed
 *        instantly, regardless if there is Bluetooth communication going on or not. For example,
 *        for debugging purpose, the BlueNRG can be set to advertise all the time and use this
 *        command to observe the signal strength changing. The system will keep the last received
 *        TX power level from the command, i.e. the 2nd command overwrites the previous TX power
 *        level. The new TX power level remains until another Set TX Power command, or the system
 *        reboots.\n
 * @param en_high_power Can be only 0 or 1. Set high power bit on or off. It is strongly adviced to use the
 * 						right value, depending on the selected hardware configuration for the RF network:
 * 						normal mode or high power mode.
 * @param pa_level Can be from 0 to 7. Set the PA level value.
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_set_tx_power_level(uint8_t en_high_power, uint8_t pa_level);

/**
 * @brief This command returns the number of packets sent in Direct Test Mode.
 * @note  When the Direct TX test is started, a 32-bit counter is used to count how many packets
 *        have been transmitted. This command can be used to check how many packets have been sent
 *        during the Direct TX test.\n
 *        The counter starts from 0 and counts upwards. The counter can wrap and start from 0 again.
 *        The counter is not cleared until the next Direct TX test starts.
 * @param[out] number_of_packets Number of packets sent during the last Direct TX test.
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_le_tx_test_packet_number(uint32_t *number_of_packets);

/**
 * @brief Put the device in standby mode.
 * @note Normally the BlueNRG will automatically enter sleep mode to save power. This command puts the
 * 		 device into the Standby mode instead of the sleep mode. The difference is that, in sleep mode,
 * 		 the device can still wake up itself with the internal timer. But in standby mode, this timer is
 * 		 disabled. So the only possibility to wake up the device is by external signals, e.g. a HCI command
 * 		 sent via SPI bus.
 * 		 The command is only accepted when there is no other Bluetooth activity. Otherwise an error code
 * 		 ERR_COMMAND_DISALLOWED will be returned.
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_device_standby(void);

/**
 * @brief This command starts a carrier frequency, i.e. a tone, on a specific channel.
 * @note  The frequency sine wave at the specific channel may be used for test purpose only.
 * 		  The channel ID is a parameter from 0 to 39 for the 40 BLE channels, e.g. 0 for 2.402GHz, 1 for 2.404GHz etc.
 * 		  This command shouldn't be used when normal Bluetooth activities are ongoing.
 * 		  The tone should be stopped by aci_hal_tone_stop() command.
 *
 * @param rf_channel BLE Channel ID, from 0 to 39 meaning (2.402 + 2*N) GHz. Actually the tone will be emitted at the
 * 					 channel central frequency minus 250 kHz.
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_tone_start(uint8_t rf_channel);

/**
 * This command is used to stop the previously started aci_hal_tone_start() command.
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_tone_stop(void);

/**
 * @brief This command returns the status of all the connections.
 * @note  This command returns the status of the 8 Bluetooth low energy links managed by the device.
 * @param[out] link_status Array of link status (8 links). See @ref Link_Status.
 * @param[out] conn_handle Array of connection handles for each link.
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_get_link_status(uint8_t link_status[8], uint16_t conn_handle[8]);

/**
 * @brief This command returns the anchor period and the largest available slot.
 * @note  This command returns information about the anchor period to help application in selecting
 * 		  slot timings when operating in multi-link scenarios.
 * @param anchor_period Current anchor period (multiple of 0.625 ms).
 * @param max_free_slot Maximum available time (multiple of 0.625 ms) that can be allocated for a new slot.
 * @return Value indicating success or error code.
 */
tBleStatus aci_hal_get_anchor_period(uint32_t *anchor_period, uint32_t *max_free_slot);

/**
 * @}
 */

/**
 * @defgroup HAL_Events HAL events
 * The structures are the data field of @ref evt_blue_aci.
 * @{
 */

/** HCI vendor specific event, raised at BlueNRG power-up or reboot. */
#define EVT_BLUE_HAL_INITIALIZED                 (0x0001)
typedef __packed struct _evt_hal_initialized{
  uint8_t reason_code; /**< Reset reason. See @ref Reset_Reasons */
} PACKED evt_hal_initialized;

/**
 * This event is generated when an overflow occurs in the event queue read by the external microcontroller.
 * This is normally caused when the external microcontroller does not read pending events.
 * The returned bitmap indicates which event has been lost. Please note that one bit set to 1 indicates one or
 * more occurrences of the particular events. The event EVT_BLUE_HAL_EVENTS_LOST cannot be lost and it will
 * be inserted in the event queue as soon as a position is freed in the event queue. This event should not
 * happen under normal operating condition where external microcontroller promptly reads events signaled by
 * IRQ pin. It is provided to detected unexpected behavior of the external microcontroller or to allow
 * application to recover situations where critical events are lost.
 */
#define EVT_BLUE_HAL_EVENTS_LOST_IDB05A1                (0x0002)
typedef __packed struct _evt_hal_events_lost{
  uint8_t  lost_events[8]; /**< Bitmap of lost events. Each bit indicates one or more occurrences of the specific event. See @ref Lost_Events */
} PACKED evt_hal_events_lost_IDB05A1;


/**
 * This event is given to the application after the @ref ACI_BLUE_INITIALIZED_EVENT
 * when a system crash is detected. This events returns system crash information for debugging purposes.
 * Information reported are useful to understand the root cause of the crash.
 */
#define EVT_BLUE_HAL_CRASH_INFO_IDB05A1                 (0x0003)
typedef __packed struct _evt_hal_crash_info{
  uint8_t  crash_type; /**< Type of crash: Assert failed (0), NMI Fault (1), Hard Fault (2)  */
  uint32_t sp; /**< SP register */
  uint32_t r0; /**< R0 register  */
  uint32_t r1; /**< R1 register  */
  uint32_t r2; /**< R2 register  */
  uint32_t r3; /**< R3 register  */
  uint32_t r12; /**< R12 register  */
  uint32_t lr; /**< LR register  */
  uint32_t pc; /**< PC register  */
  uint32_t xpsr; /**< xPSR register  */
  uint8_t  debug_data_len; /**< length of debug_data field  */
  uint8_t  debug_data[VARIABLE_SIZE]; /**< Debug data */  
} PACKED evt_hal_crash_info_IDB05A1;


/**
 * @}
 */


/**
 * @anchor Reset_Reasons
 * @name Reset Reasons
 * See @ref EVT_BLUE_HAL_INITIALIZED.
 * @{
 */
#define RESET_NORMAL            1 /**< Normal startup. */
#define RESET_UPDATER_ACI       2 /**< Updater mode entered with ACI command */
#define RESET_UPDATER_BAD_FLAG  3 /**< Updater mode entered due to a bad BLUE flag */
#define RESET_UPDATER_PIN       4 /**< Updater mode entered with IRQ pin */
#define RESET_WATCHDOG          5 /**< Reset caused by watchdog */
#define RESET_LOCKUP            6 /**< Reset due to lockup */
#define RESET_BROWNOUT          7 /**< Brownout reset */
#define RESET_CRASH             8 /**< Reset caused by a crash (NMI or Hard Fault) */
#define RESET_ECC_ERR           9 /**< Reset caused by an ECC error */
/**
 * @}
 */


/**
 * @defgroup Config_vals Offsets and lengths for configuration values.
 * @brief Offsets and lengths for configuration values.
 * 		  See aci_hal_write_config_data().
 * @{
 */

/**
 * @name Configuration values.
 * See @ref aci_hal_write_config_data().
 * @{
 */
#define CONFIG_DATA_PUBADDR_OFFSET          (0x00) /**< Bluetooth public address */
#define CONFIG_DATA_DIV_OFFSET              (0x06) /**< DIV used to derive CSRK */
#define CONFIG_DATA_ER_OFFSET               (0x08) /**< Encryption root key used to derive LTK and CSRK */
#define CONFIG_DATA_IR_OFFSET               (0x18) /**< Identity root key used to derive LTK and CSRK */
#define CONFIG_DATA_LL_WITHOUT_HOST         (0x2C) /**< Switch on/off Link Layer only mode. Set to 1 to disable Host.
 	 	 	 	 	 	 	 	 	 	 	 	 	 It can be written only if aci_hal_write_config_data() is the first command 	 	 	 	 	 	 	 	 	 	 	 	 after reset. */
#define CONFIG_DATA_RANDOM_ADDRESS          (0x80) /**< Stored static random address. Read-only. */

/**
 * Select the BlueNRG mode configurations.\n
 * @li Mode 1: slave or master, 1 connection, RAM1 only (small GATT DB)
 * @li Mode 2: slave or master, 1 connection, RAM1 and RAM2 (large GATT DB)
 * @li Mode 3: master/slave, 8 connections, RAM1 and RAM2.
 * @li Mode 4: master/slave, 4 connections, RAM1 and RAM2 simultaneous scanning and advertising.
 */
#define CONFIG_DATA_MODE_OFFSET 			(0x2D)

#define CONFIG_DATA_WATCHDOG_DISABLE 		(0x2F) /**< Set to 1 to disable watchdog. It is enabled by default. */

/**
 * @}
 */

/**
 * @name Length for configuration values.
 * See @ref aci_hal_write_config_data().
 * @{
 */
#define CONFIG_DATA_PUBADDR_LEN             (6)
#define CONFIG_DATA_DIV_LEN                 (2)
#define CONFIG_DATA_ER_LEN                  (16)
#define CONFIG_DATA_IR_LEN                  (16)
#define CONFIG_DATA_LL_WITHOUT_HOST_LEN     (1)
#define CONFIG_DATA_MODE_LEN                (1)
#define CONFIG_DATA_WATCHDOG_DISABLE_LEN    (1)
/**
 * @}
 */

/**
 * @anchor Link_Status
 * @name Status of the link
 * See @ref aci_hal_get_link_status().
 * @{
 */
#define STATUS_IDLE                         0
#define STATUS_ADVERTISING                  1
#define STATUS_CONNECTED_AS_SLAVE           2
#define STATUS_SCANNING                     3
#define STATUS_CONNECTED_AS_MASTER          5
#define STATUS_TX_TEST                      6
#define STATUS_RX_TEST                      7
/**
 * @}
 */

/**
 * @}
 */

 /**
 * @anchor Lost_Events
 * @name Lost events bitmap
 * See @ref EVT_BLUE_HAL_EVENTS_LOST.
 * @{
 */   
#define EVT_DISCONN_COMPLETE_BIT                                     0
#define EVT_ENCRYPT_CHANGE_BIT                                       1
#define EVT_READ_REMOTE_VERSION_COMPLETE_BIT                         2
#define EVT_CMD_COMPLETE_BIT                                         3
#define EVT_CMD_STATUS_BIT                                           4
#define EVT_HARDWARE_ERROR_BIT                                       5
#define EVT_NUM_COMP_PKTS_BIT                                        6
#define EVT_ENCRYPTION_KEY_REFRESH_BIT                               7
#define EVT_BLUE_HAL_INITIALIZED_BIT                                 8
#define EVT_BLUE_GAP_SET_LIMITED_DISCOVERABLE_BIT                    9
#define EVT_BLUE_GAP_PAIRING_CMPLT_BIT                               10
#define EVT_BLUE_GAP_PASS_KEY_REQUEST_BIT                            11
#define EVT_BLUE_GAP_AUTHORIZATION_REQUEST_BIT                       12
#define EVT_BLUE_GAP_SECURITY_REQ_INITIATED_BIT                      13
#define EVT_BLUE_GAP_BOND_LOST_BIT                                   14
#define EVT_BLUE_GAP_PROCEDURE_COMPLETE_BIT                          15
#define EVT_BLUE_GAP_ADDR_NOT_RESOLVED_BIT                           16
#define EVT_BLUE_L2CAP_CONN_UPDATE_RESP_BIT                          17
#define EVT_BLUE_L2CAP_PROCEDURE_TIMEOUT_BIT                         18
#define EVT_BLUE_L2CAP_CONN_UPDATE_REQ_BIT                           19
#define EVT_BLUE_GATT_ATTRIBUTE_MODIFIED_BIT                         20
#define EVT_BLUE_GATT_PROCEDURE_TIMEOUT_BIT                          21
#define EVT_BLUE_EXCHANGE_MTU_RESP_BIT                               22
#define EVT_BLUE_ATT_FIND_INFORMATION_RESP_BIT                       23
#define EVT_BLUE_ATT_FIND_BY_TYPE_VAL_RESP_BIT                       24
#define EVT_BLUE_ATT_READ_BY_TYPE_RESP_BIT                           25
#define EVT_BLUE_ATT_READ_RESP_BIT                                   26
#define EVT_BLUE_ATT_READ_BLOB_RESP_BIT                              27
#define EVT_BLUE_ATT_READ_MULTIPLE_RESP_BIT                          28
#define EVT_BLUE_ATT_READ_BY_GROUP_RESP_BIT                          29
#define EVT_BLUE_ATT_WRITE_RESP_BIT                                  30
#define EVT_BLUE_ATT_PREPARE_WRITE_RESP_BIT                          31
#define EVT_BLUE_ATT_EXEC_WRITE_RESP_BIT                             32
#define EVT_BLUE_GATT_INDICATION_BIT                                 33
#define EVT_BLUE_GATT_NOTIFICATION_BIT                               34
#define EVT_BLUE_GATT_PROCEDURE_COMPLETE_BIT                         35
#define EVT_BLUE_GATT_ERROR_RESP_BIT                                 36
#define EVT_BLUE_GATT_DISC_READ_CHARAC_BY_UUID_RESP_BIT              37
#define EVT_BLUE_GATT_WRITE_PERMIT_REQ_BIT                           38
#define EVT_BLUE_GATT_READ_PERMIT_REQ_BIT                            39
#define EVT_BLUE_GATT_READ_MULTI_PERMIT_REQ_BIT                      40
#define EVT_BLUE_GATT_TX_POOL_AVAILABLE_BIT                          41
#define EVT_BLUE_GATT_SERVER_RX_CONFIRMATION_BIT                     42
#define EVT_BLUE_GATT_PREPARE_WRITE_PERMIT_REQ_BIT                   43
#define EVT_LL_CONNECTION_COMPLETE_BIT                               44
#define EVT_LL_ADVERTISING_REPORT_BIT                                45
#define EVT_LL_CONNECTION_UPDATE_COMPLETE_BIT                        46
#define EVT_LL_READ_REMOTE_USED_FEATURES_BIT                         47
#define EVT_LL_LTK_REQUEST_BIT                                       48   
/**
 * @}
 */

/**
 * @name Hardware error event codes
 * See @ref EVT_HARDWARE_ERROR.
 * @{
 */
/**
 * Error on the SPI bus has been detected, most likely caused by incorrect SPI configuration on the external micro-controller.
 */
#define SPI_FRAMING_ERROR           0
/**
 * Caused by a slow crystal startup and they are an indication that the HS_STARTUP_TIME
 * in the device configuration needs to be tuned. After this event is recommended to hardware reset the device.
 */
#define RADIO_STATE_ERROR           1
/**
 * Caused by a slow crystal startup and they are an indication that the HS_STARTUP_TIME
 * in the device configuration needs to be tuned. After this event is recommended to hardware reset the device.
 */
#define TIMER_OVERRUN_ERROR         2 
/**
 * @}
 */
 
/**
 * @}
 */

/**
 * @}
 */

#endif /* __BLUENRG_HAL_ACI_H__ */
