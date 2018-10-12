/******************** (C) COPYRIGHT 2014 STMicroelectronics ********************
* File Name          : bluenrg_gap_aci.h
* Author             : AMS - AAS
* Version            : V1.0.0
* Date               : 26-Jun-2014
* Description        : Header file with GAP commands for BlueNRG FW6.3.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#ifndef __BLUENRG_GAP_ACI_H__
#define __BLUENRG_GAP_ACI_H__

/** 
 * @addtogroup HIGH_LEVEL_INTERFACE HIGH_LEVEL_INTERFACE
 * @{
 */
 
/**
 * @addtogroup ACI_GAP ACI_GAP
 * @brief API for GAP layer.
 * @{
 */

/**
 *@defgroup GAP_Functions GAP functions
 *@brief API for GAP layer.
 *@{
 */

/**
  * @brief  Initialize the GAP layer.
  * @note   Register the GAP service with the GATT. 
  *         All the standard GAP characteristics will also be added:
  *         @li Device Name
  *         @li Appearance
  *         @li Peripheral Preferred Connection Parameters (peripheral role only)
  *         @code

              tBleStatus ret;
              uint16_t service_handle, dev_name_char_handle, appearance_char_handle;

              ret = aci_gap_init_IDB05A1(1, 0, 0x07, &service_handle, &dev_name_char_handle, &appearance_char_handle);
              if(ret){
                PRINTF("GAP_Init failed.\n");
                reboot();    
              }  
              const char *name = "BlueNRG";  
              ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, strlen(name), (uint8_t *)name);        
              if(ret){
                PRINTF("aci_gatt_update_char_value failed.\n");
              }  
  *         @endcode
  * @param       role     Bitmap of allowed roles: see @ref gap_roles "GAP roles".
  * @param       privacy_enabled     Enable (1) or disable (0) privacy.
  * @param       device_name_char_len Length of the device name characteristic
  * @param[out]  service_handle  Handle of the GAP service.
  * @param[out]  dev_name_char_handle  Device Name Characteristic handle
  * @param[out]  appearance_char_handle Appearance Characteristic handle
  * @retval tBleStatus Value indicating success or error code.
  */
tBleStatus aci_gap_init_IDB05A1(uint8_t role, uint8_t privacy_enabled,
                        uint8_t device_name_char_len,
                        uint16_t* service_handle,
                        uint16_t* dev_name_char_handle,
                        uint16_t* appearance_char_handle);

/**
  * @brief  Initialize the GAP layer.
  * @note   Register the GAP service with the GATT.
  *         All the standard GAP characteristics will also be added:
  *         @li Device Name
  *         @li Appearance
  *         @li Peripheral Privacy Flag (peripheral role only)
  *         @li Reconnection Address (peripheral role only)
  *         @li Peripheral Preferred Connection Parameters (peripheral role only)
  *         @code

              tBleStatus ret;
              uint16_t service_handle, dev_name_char_handle, appearance_char_handle;

              ret = aci_gap_init_IDB04A1(1, &service_handle, &dev_name_char_handle, &appearance_char_handle);
              if(ret){
                PRINTF("GAP_Init failed.\n");
                reboot();    
              }  
              const char *name = "BlueNRG";  
              ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, strlen(name), (uint8_t *)name);        
              if(ret){
                PRINTF("aci_gatt_update_char_value failed.\n");
              }  
  *         @endcode
  * @param       role     One of the allowed roles: @ref GAP_PERIPHERAL_ROLE or @ref GAP_CENTRAL_ROLE. See @ref gap_roles "GAP roles".
  * @param[out]  service_handle  Handle of the GAP service.
  * @param[out]  dev_name_char_handle  Device Name Characteristic handle
  * @param[out]  appearance_char_handle Appearance Characteristic handle
  * @retval tBleStatus Value indicating success or error code.
  */
tBleStatus aci_gap_init_IDB04A1(uint8_t role,
                 uint16_t* service_handle,
                 uint16_t* dev_name_char_handle,
                 uint16_t* appearance_char_handle);

/**
  * @brief   Set the Device in non-discoverable mode.
  * @note    This command will disable the LL advertising.
  * @retval  tBleStatus Value indicating success or error code.
  */
tBleStatus aci_gap_set_non_discoverable(void);

/**
 * @brief  Put the device in limited discoverable mode
 *         (as defined in GAP specification volume 3, section 9.2.3).
 * @note    The device will be discoverable for TGAP (lim_adv_timeout) = 180 seconds.
 *          The advertising can be disabled at any time by issuing
 *          aci_gap_set_non_discoverable() command.
 *          The AdvIntervMin and AdvIntervMax parameters are optional. If both
 *          are set to 0, the GAP will use default values (250 ms and 500 ms respectively).
 *          Host can set the Local Name, a Service UUID list and the Slave Connection
 *          Minimum and Maximum. If provided, these data will be inserted into the
 *          advertising packet payload as AD data. These parameters are optional
 *          in this command. These values can be also set using aci_gap_update_adv_data()
 *          separately.
 *          The total size of data in advertising packet cannot exceed 31 bytes.
 *          With this command, the BLE Stack will also add automatically the following
 *          standard AD types:
 *          @li AD Flags
 *          @li TX Power Level
 *
 *          When advertising timeout happens (i.e. limited discovery period has elapsed), controller generates
 *          @ref EVT_BLUE_GAP_LIMITED_DISCOVERABLE event.
 *
 *          Example:
 * @code
 *
 *              #define  ADV_INTERVAL_MIN_MS  100
 *              #define  ADV_INTERVAL_MAX_MS  200
 *
 *              tBleStatus ret;
 *
 *              const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','l','u','e','N','R','G'};
 *              const uint8_t serviceUUIDList[] = {AD_TYPE_16_BIT_SERV_UUID,0x34,0x12};
 *
 *              ret = aci_gap_set_limited_discoverable(ADV_IND, (ADV_INTERVAL_MIN_MS*1000)/0.625,
 *                                                     (ADV_INTERVAL_MAX_MS*1000)/0.625,
 *                                                     STATIC_RANDOM_ADDR, NO_WHITE_LIST_USE,
 *                                                    sizeof(local_name), local_name,
 *                                                     sizeof(serviceUUIDList), serviceUUIDList,
 *                                                     0, 0);
 * @endcode
 *
 * @param       AdvType     One of the advertising types:
 *               @arg @ref ADV_IND Connectable undirected advertising
 *               @arg @ref ADV_SCAN_IND Scannable undirected advertising
 *               @arg @ref ADV_NONCONN_IND Non connectable undirected advertising
 * @param       AdvIntervMin    Minimum advertising interval.
 * 								 Range: 0x0020 to 0x4000
 * 								 Default: 250 ms
 * 								 Time = N * 0.625 msec
 * 								 Time Range: 20 ms to 10.24 sec (minimum 100 ms for ADV_SCAN_IND or ADV_NONCONN_IND).
 * @param       AdvIntervMax    Maximum advertising interval.
 * 								Range: 0x0020 to 0x4000
 * 								Default: 500 ms
 * 								Time = N * 0.625 msec
 * 								Time Range: 20 ms to 10.24 sec  (minimum 100 ms for ADV_SCAN_IND or ADV_NONCONN_IND).
 * @param       OwnAddrType     Type of our address used during advertising
 *                              (@ref PUBLIC_ADDR,@ref STATIC_RANDOM_ADDR).
 * @param       AdvFilterPolicy  Filter policy:
 *                               @arg NO_WHITE_LIST_USE
 *                               @arg WHITE_LIST_FOR_ONLY_SCAN
 *                               @arg WHITE_LIST_FOR_ONLY_CONN
 *                               @arg WHITE_LIST_FOR_ALL
 * @param  LocalNameLen  Length of LocalName array.
 * @param  LocalName  Array containing the Local Name AD data. First byte is the AD type:
 *                       @ref AD_TYPE_SHORTENED_LOCAL_NAME or @ref AD_TYPE_COMPLETE_LOCAL_NAME.
 * @param  ServiceUUIDLen Length of ServiceUUIDList array.
 * @param  ServiceUUIDList  This is the list of the UUIDs AD Types as defined in Volume 3,
 *  							Section 11.1.1 of GAP Specification. First byte is the AD Type.
 *  							@arg @ref AD_TYPE_16_BIT_SERV_UUID
 *  							@arg @ref AD_TYPE_16_BIT_SERV_UUID_CMPLT_LIST
 *  							@arg @ref AD_TYPE_128_BIT_SERV_UUID
 *  							@arg @ref AD_TYPE_128_BIT_SERV_UUID_CMPLT_LIST
 * @param  SlaveConnIntervMin Slave connection interval minimum value suggested by Peripheral.
 * 							 If SlaveConnIntervMin and SlaveConnIntervMax are not 0x0000,
 * 							 Slave Connection Interval Range AD structure will be added in advertising
 * 							 data.
 * 							 Connection interval is defined in the following manner:
 * 							 connIntervalmin = Slave_Conn_Interval_Min x 1.25ms
 * 							 Slave_Conn_Interval_Min range: 0x0006 to 0x0C80
 * 							 Value of 0xFFFF indicates no specific minimum.
 * @param  SlaveConnIntervMax Slave connection interval maximum value suggested by Peripheral.
 * 							 If SlaveConnIntervMin and SlaveConnIntervMax are not 0x0000,
 * 							 Slave Connection Interval Range AD structure will be added in advertising
 * 							 data.
 * 							 ConnIntervalmax = Slave_Conn_Interval_Max x 1.25ms
 * 							 Slave_Conn_Interval_Max range: 0x0006 to 0x0C80
 * 							 Slave_ Conn_Interval_Max shall be equal to or greater than the Slave_Conn_Interval_Min.
 * 							 Value of 0xFFFF indicates no specific maximum.
 *
 * @retval tBleStatus Value indicating success or error code.
 */
tBleStatus aci_gap_set_limited_discoverable(uint8_t AdvType, uint16_t AdvIntervMin, uint16_t AdvIntervMax,
					    uint8_t OwnAddrType, uint8_t AdvFilterPolicy, uint8_t LocalNameLen,
					    const char *LocalName, uint8_t ServiceUUIDLen, uint8_t* ServiceUUIDList,
					    uint16_t SlaveConnIntervMin, uint16_t SlaveConnIntervMax);
/**
 * @brief Put the Device in general discoverable mode (as defined in GAP specification volume 3, section 9.2.4).
 * @note  The device will be discoverable until the Host issue Aci_Gap_Set_Non_Discoverable command.
 * 		  The Adv_Interval_Min and Adv_Interval_Max parameters are optional. If both are set to 0, the GAP uses
 * 		  the default values for advertising intervals (1.28 s and 2.56 s respectively for IDB04A1).
 *                When using connectable undirected advertising events:\n
 *                @li Adv_Interval_Min = 30 ms
 *                @li Adv_Interval_Max = 60 ms
 *                \nWhen using non-connectable advertising events or scannable undirected advertising events:\n
 *                @li Adv_Interval_Min = 100 ms
 *                @li Adv_Interval_Max = 150 ms
 * 		  Host can set the Local Name, a Service UUID list and the Slave Connection Interval Range. If provided,
 * 		  these data will be inserted into the advertising packet payload as AD data. These parameters are optional
 * 		  in this command. These values can be also set using aci_gap_update_adv_data() separately.
 * 		  The total size of data in advertising packet cannot exceed 31 bytes.
 * 		  With this command, the BLE Stack will also add automatically the following standard AD types:
 * 		  @li AD Flags
 * 		  @li TX Power Level
 *
 * 		  Usage example:
 *
 * 		  @code
 *
 *              #define  ADV_INTERVAL_MIN_MS  800
 *              #define  ADV_INTERVAL_MAX_MS  900
 *              #define  CONN_INTERVAL_MIN_MS 100
 *              #define  CONN_INTERVAL_MAX_MS 300
 *
 *              tBleStatus ret;
 *
 *              const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','l','u','e','N','R','G'};
 *              const uint8_t serviceUUIDList[] = {AD_TYPE_16_BIT_SERV_UUID,0x34,0x12};
 *
 *              ret = aci_gap_set_discoverable(ADV_IND, (ADV_INTERVAL_MIN_MS*1000)/625,
 *                                                     (ADV_INTERVAL_MAX_MS*1000)/625,
 *                                                     STATIC_RANDOM_ADDR, NO_WHITE_LIST_USE,
 *                                                     sizeof(local_name), local_name,
 *                                                     0, NULL,
 *                                                     (CONN_INTERVAL_MIN_MS*1000)/1250,
 *                                                     (CONN_INTERVAL_MAX_MS*1000)/1250);
 * 			@endcode
 *
 * @param AdvType One of the advertising types:
 *                @arg @ref ADV_IND Connectable undirected advertising
 *                @arg @ref ADV_SCAN_IND Scannable undirected advertising
 *                @arg @ref ADV_NONCONN_IND Non connectable undirected advertising
 * @param       AdvIntervMin    Minimum advertising interval.
 * 								 Range: 0x0020 to 0x4000
 * 								 Default: 1.28 s
 * 								 Time = N * 0.625 msec
 * 								 Time Range: 20 ms to 10.24 sec (minimum 100 ms for ADV_SCAN_IND or ADV_NONCONN_IND).
 * @param       AdvIntervMax    Maximum advertising interval.
 * 								Range: 0x0020 to 0x4000
 * 								Default: 2.56 s
 * 								Time = N * 0.625 msec
 * 								Time Range: 20 ms to 10.24 sec  (minimum 100 ms for ADV_SCAN_IND or ADV_NONCONN_IND).
 * @param       OwnAddrType     Type of our address used during advertising
 *                              (@ref PUBLIC_ADDR,@ref STATIC_RANDOM_ADDR).
 * @param       AdvFilterPolicy  Filter policy:
 *                               @arg @ref NO_WHITE_LIST_USE
 *                               @arg @ref WHITE_LIST_FOR_ONLY_SCAN
 *                               @arg @ref WHITE_LIST_FOR_ONLY_CONN
 *                               @arg @ref WHITE_LIST_FOR_ALL
 * @param  LocalNameLen  Length of LocalName array.
 * @param  LocalName  Array containing the Local Name AD data. First byte is the AD type:
 *                       @ref AD_TYPE_SHORTENED_LOCAL_NAME or @ref AD_TYPE_COMPLETE_LOCAL_NAME.
 * @param  ServiceUUIDLen Length of ServiceUUIDList array.
 * @param  ServiceUUIDList  This is the list of the UUIDs AD Types as defined in Volume 3,
 *  							Section 11.1.1 of GAP Specification. First byte is the AD Type.
 *  							@arg @ref AD_TYPE_16_BIT_SERV_UUID
 *  							@arg @ref AD_TYPE_16_BIT_SERV_UUID_CMPLT_LIST
 *  							@arg @ref AD_TYPE_128_BIT_SERV_UUID
 *  							@arg @ref AD_TYPE_128_BIT_SERV_UUID_CMPLT_LIST
 * @param  SlaveConnIntervMin Slave connection interval minimum value suggested by Peripheral.
 * 							 If SlaveConnIntervMin and SlaveConnIntervMax are not 0x0000,
 * 							 Slave Connection Interval Range AD structure will be added in advertising
 * 							 data.
 * 							 Connection interval is defined in the following manner:
 * 							 connIntervalmin = Slave_Conn_Interval_Min x 1.25ms
 * 							 Slave_Conn_Interval_Min range: 0x0006 to 0x0C80
 * 							 Value of 0xFFFF indicates no specific minimum.
 * @param  SlaveConnIntervMax Slave connection interval maximum value suggested by Peripheral.
 * 							 If SlaveConnIntervMin and SlaveConnIntervMax are not 0x0000,
 * 							 Slave Connection Interval Range AD structure will be added in advertising
 * 							 data.
 * 							 ConnIntervalmax = Slave_Conn_Interval_Max x 1.25ms
 * 							 Slave_Conn_Interval_Max range: 0x0006 to 0x0C80
 * 							 Slave_ Conn_Interval_Max shall be equal to or greater than the Slave_Conn_Interval_Min.
 * 							 Value of 0xFFFF indicates no specific maximum.
 *
 * @retval tBleStatus Value indicating success or error code.
 */
tBleStatus aci_gap_set_discoverable(uint8_t AdvType, uint16_t AdvIntervMin, uint16_t AdvIntervMax,
                             uint8_t OwnAddrType, uint8_t AdvFilterPolicy, uint8_t LocalNameLen,
                             const char *LocalName, uint8_t ServiceUUIDLen, uint8_t* ServiceUUIDList,
                             uint16_t SlaveConnIntervMin, uint16_t SlaveConnIntervMax);

/**
 * @brief Set the Device in direct connectable mode (as defined in GAP specification Volume 3, Section 9.3.3).
 * @note  If the privacy is enabled, the reconnection address is used for advertising, otherwise the address
 * 		  of the type specified in OwnAddrType is used. The device will be in directed connectable mode only
 * 		  for 1.28 seconds. If no connection is established within this duration, the device enters non
 * 		  discoverable mode and advertising will have to be again enabled explicitly.
 * 		  The controller generates a @ref EVT_LE_CONN_COMPLETE event with the status set to @ref HCI_DIRECTED_ADV_TIMEOUT
 * 		  if the connection was not established and 0x00 if the connection was successfully established.
 *
 * 		  Usage example:
 * 		  @code
 *
 * 		  tBleStatus ret;
 *
 * 		  const uint8_t central_address[] = {0x43,0x27,0x84,0xE1,0x80,0x02};
 * 		  ret = aci_gap_set_direct_connectable_IDB05A1(PUBLIC_ADDR, HIGH_DUTY_CYCLE_DIRECTED_ADV, PUBLIC_ADDR, central_address);
 * 		  @endcode
 *
 *
 *
 * @param own_addr_type  Type of our address used during advertising (@ref PUBLIC_ADDR,@ref STATIC_RANDOM_ADDR).
 * @param directed_adv_type  Type of directed advertising (@ref HIGH_DUTY_CYCLE_DIRECTED_ADV, @ref LOW_DUTY_CYCLE_DIRECTED_ADV).
 * @param initiator_addr_type Type of peer address (@ref PUBLIC_ADDR,@ref STATIC_RANDOM_ADDR).
 * @param initiator_addr     Initiator's address (Little Endian).
 * @param adv_interv_min     Minimum advertising interval for low duty cycle directed advertising.
 *                           Range: 0x0020 to 0x4000
 *							 Time = N * 0.625 msec
 *							 Time Range: 20 ms to 10.24 sec.
 * @param adv_interv_max     Maximum advertising interval for low duty cycle directed advertising.
 *                           Range: 0x0020 to 0x4000
 *							 Time = N * 0.625 msec
 *							 Time Range: 20 ms to 10.24 sec.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_direct_connectable_IDB05A1(uint8_t own_addr_type, uint8_t directed_adv_type, uint8_t initiator_addr_type,
                                          const uint8_t *initiator_addr, uint16_t adv_interv_min, uint16_t adv_interv_max);

/**
 * @brief Set the Device in direct connectable mode (as defined in GAP specification Volume 3, Section 9.3.3).
 * @note  If the privacy is enabled, the reconnection address is used for advertising, otherwise the address
 * 		  of the type specified in OwnAddrType is used. The device will be in directed connectable mode only
 * 		  for 1.28 seconds. If no connection is established within this duration, the device enters non
 * 		  discoverable mode and advertising will have to be again enabled explicitly.
 * 		  The controller generates a @ref EVT_LE_CONN_COMPLETE event with the status set to @ref HCI_DIRECTED_ADV_TIMEOUT
 * 		  if the connection was not established and 0x00 if the connection was successfully established.
 *
 * 		  Usage example:
 * 		  @code
 *
 * 		  tBleStatus ret;
 *
 * 		  const uint8_t central_address = {0x43,0x27,0x84,0xE1,0x80,0x02};
 * 		  ret = aci_gap_set_direct_connectable_IDB04A1(PUBLIC_ADDR, PUBLIC_ADDR, central_address);
 * 		  @endcode
 *
 *
 *
 * @param own_addr_type  Type of our address used during advertising (@ref PUBLIC_ADDR,@ref STATIC_RANDOM_ADDR).
 * @param initiator_addr_type Type of peer address (@ref PUBLIC_ADDR,@ref STATIC_RANDOM_ADDR).
 * @param initiator_addr     Initiator's address (Little Endian).
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_direct_connectable_IDB04A1(uint8_t own_addr_type, uint8_t initiator_addr_type, const uint8_t *initiator_addr);

/**
 * @brief Set the IO capabilities of the device.
 * @note This command has to be given only when the device is not in a connected state.
 * @param io_capability One of the allowed codes for IO Capability:
 * 		  @arg @ref IO_CAP_DISPLAY_ONLY
 * 		  @arg @ref IO_CAP_DISPLAY_YES_NO
 * 		  @arg @ref IO_CAP_KEYBOARD_ONLY
 * 		  @arg @ref IO_CAP_NO_INPUT_NO_OUTPUT
 * 		  @arg @ref IO_CAP_KEYBOARD_DISPLAY
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_io_capability(uint8_t io_capability);

/**
 * @brief Set the authentication requirements for the device.
 * @note  If the oob_enable is set to 0, oob_data will be ignored.
 *        This command has to be given only when the device is not in a connected state.
 * @param mitm_mode MITM mode:
 * 					@arg @ref MITM_PROTECTION_NOT_REQUIRED
 * 					@arg @ref MITM_PROTECTION_REQUIRED
 * @param oob_enable If OOB data are present or not:
 * 					 @arg @ref OOB_AUTH_DATA_ABSENT
 * 					 @arg @ref OOB_AUTH_DATA_PRESENT
 * @param oob_data   Out-Of-Band data
 * @param min_encryption_key_size Minimum size of the encryption key to be used during the pairing process
 * @param max_encryption_key_size Maximum size of the encryption key to be used during the pairing process
 * @param use_fixed_pin If application wants to use a fixed pin or not:
 * 						@arg @ref USE_FIXED_PIN_FOR_PAIRING
 * 						@arg @ref DONOT_USE_FIXED_PIN_FOR_PAIRING
 * 						If a fixed pin is not used, it has to be provided by the application with
 * 						aci_gap_pass_key_response() after @ref EVT_BLUE_GAP_PASS_KEY_REQUEST event.
 * @param fixed_pin If use_fixed_pin is USE_FIXED_PIN_FOR_PAIRING, this is the value of the pin that will
 * 					be used during pairing if MIMT protection is enabled. Any value between 0 to 999999 is
 * 					accepted.
 * @param bonding_mode One of the bonding modes:
 * 					   @arg @ref BONDING
 * 					   @arg @ref NO_BONDING
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_auth_requirement(uint8_t mitm_mode,
                                        uint8_t oob_enable,
                                        uint8_t oob_data[16],
                                        uint8_t min_encryption_key_size,
                                        uint8_t max_encryption_key_size,
                                        uint8_t use_fixed_pin,
                                        uint32_t fixed_pin,
                                        uint8_t bonding_mode);
 /**
  * @brief Set the authorization requirements of the device.
  * @note This command has to be given when connected to a device if authorization is required to access services
  *       which require authorization.
  * @param conn_handle Handle of the connection.
  * @param authorization_enable @arg @ref AUTHORIZATION_NOT_REQUIRED : Authorization not required
  * 							@arg @ref AUTHORIZATION_REQUIRED : Authorization required. This enables
  * 							the authorization requirement in the device and when a remote device
  * 							tries to read/write a characeristic with authorization requirements, the stack will
  *                             send back an error response with "Insufficient authorization" error code.
  *                             After pairing is complete a @ref EVT_BLUE_GAP_AUTHORIZATION_REQUEST event
  * 							will be sent to the Host.
  * @return Value indicating success or error code.
  */
tBleStatus aci_gap_set_author_requirement(uint16_t conn_handle, uint8_t authorization_enable);

/**
 * @brief Provide the pass key that will be used during pairing.
 * @note This command should be sent by the Host in response to @ref EVT_BLUE_GAP_PASS_KEY_REQUEST event.
 * @param conn_handle Connection handle
 * @param passkey	  Pass key that will be used during the pairing process. Must be a number between
 * 					  0 and 999999.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_pass_key_response(uint16_t conn_handle, uint32_t passkey);

/**
 * @brief Authorize a device to access attributes.
 * @note Application should send this command after it has received a @ref EVT_BLUE_GAP_AUTHORIZATION_REQUEST.
 *
 * @param conn_handle Connection handle
 * @param authorize   @arg @ref CONNECTION_AUTHORIZED : Authorize (accept connection)
 * 					  @arg @ref CONNECTION_REJECTED : Reject (reject connection)
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_authorization_response(uint16_t conn_handle, uint8_t authorize);

/**
 * @brief Put the device into non-connectable mode.
 * @param adv_type One of the allowed advertising types:
 *                 @arg @ref ADV_SCAN_IND : Scannable undirected advertising
 *                 @arg @ref ADV_NONCONN_IND : Non-connectable undirected advertising
 * @param own_address_type If Privacy is disabled, then the peripheral address can be
 * 			               @arg @ref PUBLIC_ADDR.
 * 			               @arg @ref STATIC_RANDOM_ADDR.
 *                         If Privacy is enabled, then the peripheral address can be 
 *                         @arg @ref RESOLVABLE_PRIVATE_ADDR
 *                         @arg @ref NON_RESOLVABLE_PRIVATE_ADDR
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_non_connectable_IDB05A1(uint8_t adv_type, uint8_t own_address_type);

/**
 * @brief Put the device into non-connectable mode.
 * @param adv_type One of the allowed advertising types:
 *                 @arg @ref ADV_SCAN_IND : Scannable undirected advertising
 *                 @arg @ref ADV_NONCONN_IND : Non-connectable undirected advertising
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_non_connectable_IDB04A1(uint8_t adv_type);

/**
 * @brief Put the device into undirected connectable mode.
 * @note  If privacy is enabled in the device, a resolvable private address is generated and used
 *        as the advertiser's address. If not, the address of the type specified in own_addr_type
 *        is used for advertising.
 * @param own_addr_type Type of our address used during advertising:
 *                      if BLUENRG (IDB04A1)
 * 			            @arg @ref PUBLIC_ADDR.
 * 			            @arg @ref STATIC_RANDOM_ADDR.
 *                      else if BLUENRG_MS (IDB05A1)
 *                      If Privacy is disabled:
 * 			            @arg @ref PUBLIC_ADDR.
 * 			            @arg @ref STATIC_RANDOM_ADDR.
 *                      If Privacy is enabled:
 *                      @arg @ref RESOLVABLE_PRIVATE_ADDR
 *                      @arg @ref NON_RESOLVABLE_PRIVATE_ADDR
 * @param adv_filter_policy  Filter policy:
 *                         @arg @ref NO_WHITE_LIST_USE
 *                         @arg @ref WHITE_LIST_FOR_ALL
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_undirected_connectable(uint8_t own_addr_type, uint8_t adv_filter_policy);

/**
 * @brief Send a slave security request to the master.
 * @note This command has to be issued to notify the master of the security requirements of the slave.
 * 		 The master may encrypt the link, initiate the pairing procedure, or reject the request.
 * @param conn_handle Connection handle
 * @param bonding	   One of the bonding modes:
 * 					   @arg @ref BONDING
 * 					   @arg @ref NO_BONDING
 * @param mitm_protection  If MITM protection is required or not:
 * 						   @arg @ref MITM_PROTECTION_NOT_REQUIRED
 * 					       @arg @ref MITM_PROTECTION_REQUIRED
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_slave_security_request(uint16_t conn_handle, uint8_t bonding, uint8_t mitm_protection);

/**
 * @brief Update advertising data.
 * @note This command can be used to update the advertising data for a particular AD type.
 *       If the AD type specified does not exist, then it is added to the advertising data.
 *       If the overall advertising data length is more than 31 octets after the update, then
 *       the command is rejected and the old data is retained.
 * @param AdvLen Length of AdvData array
 * @param AdvData Advertisement Data,  formatted as specified in Bluetooth specification
 *        (Volume 3, Part C, 11), including data length. It can contain more than one AD type.
 *        Example
 * @code
 *  tBleStatus ret;
 *  const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','l','u','e','N','R','G'};
 *  const uint8_t serviceUUIDList[] = {AD_TYPE_16_BIT_SERV_UUID,0x34,0x12};
 *  const uint8_t manuf_data[] = {4, AD_TYPE_MANUFACTURER_SPECIFIC_DATA, 0x05, 0x02, 0x01};
 *
 *  ret = aci_gap_set_discoverable(ADV_IND, 0, 0, STATIC_RANDOM_ADDR, NO_WHITE_LIST_USE,
 *                                 8, local_name, 3, serviceUUIDList, 0, 0);
 *  ret = aci_gap_update_adv_data(5, manuf_data);
 * @endcode
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_update_adv_data(uint8_t AdvLen, const uint8_t *AdvData);

/**
 * @brief Delete an AD Type
 * @note This command can be used to delete the specified AD type from the advertisement data if
 * 		 present.
 * @param ad_type One of the allowed AD types (see @ref AD_Types)
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_delete_ad_type(uint8_t ad_type);

/**
 * @brief Get the current security settings
 * @note This command can be used to get the current security settings of the device.
 * @param mitm_protection 	@arg 0: Not required
 * 							@arg 1: Required
 * @param bonding			@arg 0: No bonding mode
 * 							@arg 1: Bonding mode
 * @param oob_data			@arg 0: Data absent
 * 							@arg 1: Data present
 * @param passkey_required  @arg 0: Not required
 * 							@arg 1: Fixed pin is present which is being used
 * 							@arg 2: Passkey required for pairing. An event will be generated
 * 							when required.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_get_security_level(uint8_t* mitm_protection, uint8_t* bonding,
                                      uint8_t* oob_data, uint8_t* passkey_required);

/**
 * @brief Add addresses of bonded devices into the controller's whitelist.
 * @note  The command will return an error if there are no devices in the database or if it was unable
 * 		  to add the device into the whitelist.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_configure_whitelist(void);

/**
 * @brief Terminate a connection.
 * @note  A @ref EVT_DISCONN_COMPLETE event will be generated when the link is disconnected.
 * @param conn_handle Connection handle
 * @param reason  Reason for requesting disconnection. The error code can be any of ones as specified
 * 				  for the disconnected command in the HCI specification (See @ref HCI_Error_codes).
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_terminate(uint16_t conn_handle, uint8_t reason);

/**
 * @brief Clear the security database.
 * @note  All the devices in the security database will be removed.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_clear_security_database(void);

/**
 * @brief Allows the security manager to complete the pairing procedure and re-bond with the master.
 * @note This command can be issued by the application if a @ref EVT_BLUE_GAP_BOND_LOST event is generated.
 * @param conn_handle 
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_allow_rebond_IDB05A1(uint16_t conn_handle);

/**
 * @brief Allows the security manager to complete the pairing procedure and re-bond with the master.
 * @note This command can be issued by the application if a @ref EVT_BLUE_GAP_BOND_LOST event is generated.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_allow_rebond_IDB04A1(void);

/**
 * @brief Start the limited discovery procedure.
 * @note  The controller is commanded to start active scanning. When this procedure is started,
 *        only the devices in limited discoverable mode are returned to the upper layers.
 *        The procedure is terminated when either the upper layers issue a command to terminate the
 *        procedure by issuing the command aci_gap_terminate_gap_procedure() with the procedure code
 *        set to @ref GAP_LIMITED_DISCOVERY_PROC or a timeout happens. When the procedure is terminated
 *        due to any of the above  reasons, @ref EVT_BLUE_GAP_PROCEDURE_COMPLETE event is returned with
 *        the procedure code set to @ref GAP_LIMITED_DISCOVERY_PROC.
 *        The device found when the procedure is ongoing is returned to the upper layers through the
 *        event @ref EVT_BLUE_GAP_DEVICE_FOUND (for IDB04A1) or @ref EVT_LE_ADVERTISING_REPORT (for IDB05A1).
 * @param scanInterval Time interval from when the Controller started its last LE scan until it begins
 * 					   the subsequent LE scan. The scan interval should be a number in the range
 * 					   0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					   For a number N, Time = N x 0.625 msec.
 * @param scanWindow Amount of time for the duration of the LE scan. Scan_Window shall be less than
 * 					 or equal to Scan_Interval. The scan window should be a number in the range
 * 					 0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					 For a number N, Time = N x 0.625 msec.
 * @param own_address_type Type of our address used during advertising (@ref PUBLIC_ADDR, @ref STATIC_RANDOM_ADDR).
 * @param filterDuplicates Duplicate filtering enabled or not.
 * 						   @arg 0x00: Do not filter the duplicates
 * 						   @arg 0x01: Filter duplicates
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_limited_discovery_proc(uint16_t scanInterval, uint16_t scanWindow,
						uint8_t own_address_type, uint8_t filterDuplicates);

/**
 * @brief Start the general discovery procedure.
 * @note  The controller is commanded to start active scanning. The procedure is terminated when
 * 		  either the upper layers issue a command to terminate the procedure by issuing the command
 * 		  aci_gap_terminate_gap_procedure() with the procedure code set to GAP_GENERAL_DISCOVERY_PROC
 * 		  or a timeout happens. When the procedure is terminated due to any of the above reasons,
 * 		  @ref EVT_BLUE_GAP_PROCEDURE_COMPLETE event is returned with the procedure code set to
 * 		  @ref GAP_GENERAL_DISCOVERY_PROC. The device found when the procedure is ongoing is returned to
 * 		  the upper layers through the event @ref EVT_BLUE_GAP_DEVICE_FOUND  (for IDB04A1) or @ref EVT_LE_ADVERTISING_REPORT (for IDB05A1).
 * @param scanInterval Time interval from when the Controller started its last LE scan until it begins
 * 					   the subsequent LE scan. The scan interval should be a number in the range
 * 					   0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					   For a number N, Time = N x 0.625 msec.
 * @param scanWindow Amount of time for the duration of the LE scan. Scan_Window shall be less than
 * 					 or equal to Scan_Interval. The scan window should be a number in the range
 * 					 0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					 For a number N, Time = N x 0.625 msec.
 * @param own_address_type Type of our address used during advertising (@ref PUBLIC_ADDR, @ref STATIC_RANDOM_ADDR).
 * @param filterDuplicates Duplicate filtering enabled or not.
 * 						   @arg 0x00: Do not filter the duplicates
 * 						   @arg 0x01: Filter duplicates
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_general_discovery_proc(uint16_t scanInterval, uint16_t scanWindow,
						uint8_t own_address_type, uint8_t filterDuplicates);

/**
 * @brief Start the name discovery procedure.
 * @note  A LE_Create_Connection call will be made to the controller by GAP with the initiator filter
 * 		  policy set to “ignore whitelist and process connectable advertising packets only for the
 * 		  specified device”. Once a connection is established, GATT procedure is started to read the
 * 		  device name characteristic. When the read is completed (successfully or unsuccessfully),
 * 		  a @ref EVT_BLUE_GAP_PROCEDURE_COMPLETE event is given to the upper layer. The event also
 * 		  contains the name of the device if the device name was read successfully.
 * @param scanInterval Time interval from when the Controller started its last LE scan until it begins
 * 					   the subsequent LE scan. The scan interval should be a number in the range
 * 					   0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					   For a number N, Time = N x 0.625 msec.
 * @param scanWindow Amount of time for the duration of the LE scan. Scan_Window shall be less than
 * 					 or equal to Scan_Interval. The scan window should be a number in the range
 * 					 0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					 For a number N, Time = N x 0.625 msec.
 * @param peer_bdaddr_type Type of the peer address (@ref PUBLIC_ADDR, @ref STATIC_RANDOM_ADDR).
 * @param peer_bdaddr Address of the peer device with which a connection has to be established.
 * @param own_bdaddr_type Type of our address used during advertising (PUBLIC_ADDR,STATIC_RANDOM_ADDR).
 * @param conn_min_interval Minimum value for the connection event interval. This shall be less than or
 * 							equal to Conn_Interval_Max.\n
 * 							Range: 0x0006 to 0x0C80\n
 * 							Time = N x 1.25 msec\n
 * 							Time Range: 7.5 msec to 4 seconds
 * @param conn_max_interval Maximum value for the connection event interval. This shall be greater than or
 * 							equal to Conn_Interval_Min.\n
 * 							Range: 0x0006 to 0x0C80\n
 * 							Time = N x 1.25 msec\n
 * 							Time Range: 7.5 msec to 4 seconds
 * @param conn_latency Slave latency for the connection in number of connection events.\n
 * 						 Range: 0x0000 to 0x01F4
 * @param supervision_timeout Supervision timeout for the LE Link.\n
 * 							  Range: 0x000A to 0x0C80\n
 * 							  Time = N x 10 msec\n
 * 							  Time Range: 100 msec to 32 seconds
 * @param min_conn_length Minimum length of connection needed for the LE connection.\n
 * 						  Range: 0x0000 - 0xFFFF\n
 * 						  Time = N x 0.625 msec.
 * @param max_conn_length Maximum length of connection needed for the LE connection.\n
 * 						  Range: 0x0000 - 0xFFFF\n
 * 						  Time = N x 0.625 msec.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_name_discovery_proc(uint16_t scanInterval, uint16_t scanWindow,
				     uint8_t peer_bdaddr_type, tBDAddr peer_bdaddr,	
				     uint8_t own_bdaddr_type, uint16_t conn_min_interval,	
				     uint16_t conn_max_interval, uint16_t conn_latency,	
				     uint16_t supervision_timeout, uint16_t min_conn_length, 
				     uint16_t max_conn_length);

/**
 * @brief Start the auto connection establishment procedure.
 * @note The devices specified are added to the white list of the controller and a LE_Create_Connection
 * 		 call will be made to the controller by GAP with the initiator filter policy set to
 * 		 “use whitelist to determine which advertiser to connect to”. When a command is issued to
 * 		 terminate the procedure by upper layer, a LE_Create_Connection_Cancel call will be made to the
 * 		 controller by GAP.
 * 		 The procedure is terminated when either a connection is successfully established with one of
 * 		 the specified devices in the white list or the procedure is explicitly terminated by issuing
 * 		 the command aci_gap_terminate_gap_procedure() with the procedure code set to
 * 		 @ref GAP_AUTO_CONNECTION_ESTABLISHMENT_PROC. A @ref EVT_BLUE_GAP_PROCEDURE_COMPLETE event is returned with
 * 		 the procedure code set to @ref GAP_AUTO_CONNECTION_ESTABLISHMENT_PROC.
 * @param scanInterval Time interval from when the Controller started its last LE scan until it begins
 * 					   the subsequent LE scan. The scan interval should be a number in the range
 * 					   0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					   For a number N, Time = N x 0.625 msec.
 * @param scanWindow Amount of time for the duration of the LE scan. Scan_Window shall be less than
 * 					 or equal to Scan_Interval. The scan window should be a number in the range
 * 					 0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					 For a number N, Time = N x 0.625 msec.
 * @param own_bdaddr_type Type of our address used during advertising (PUBLIC_ADDR,STATIC_RANDOM_ADDR).
 * @param conn_min_interval Minimum value for the connection event interval. This shall be less than or
 * 							equal to Conn_Interval_Max.\n
 * 							Range: 0x0006 to 0x0C80\n
 * 							Time = N x 1.25 msec\n
 * 							Time Range: 7.5 msec to 4 seconds
 * @param conn_max_interval Maximum value for the connection event interval. This shall be greater than or
 * 							equal to Conn_Interval_Min.\n
 * 							Range: 0x0006 to 0x0C80\n
 * 							Time = N x 1.25 msec\n
 * 							Time Range: 7.5 msec to 4 seconds
 * @param conn_latency Slave latency for the connection in number of connection events.\n
 * 						 Range: 0x0000 to 0x01F4
 * @param supervision_timeout Supervision timeout for the LE Link.\n
 * 							  Range: 0x000A to 0x0C80\n
 * 							  Time = N x 10 msec\n
 * 							  Time Range: 100 msec to 32 seconds
 * @param min_conn_length Minimum length of connection needed for the LE connection.\n
 * 						  Range: 0x0000 - 0xFFFF\n
 * 						  Time = N x 0.625 msec.
 * @param max_conn_length Maximum length of connection needed for the LE connection.\n
 * 						  Range: 0x0000 - 0xFFFF\n
 * 						  Time = N x 0.625 msec.
 * @cond BLUENRG
 * @param use_reconn_addr If 1, the provided reconnection address is used as our address during the procedure (the address
 * 						  has been previously notified to the application through @ref EVT_BLUE_GAP_RECONNECTION_ADDRESS event).\n
 * @param reconn_addr	Reconnection address used if use_reconn_addr is 1.
 * @endcond
 * @param num_whitelist_entries Number of devices that have to be added to the whitelist.
 * @param addr_array addr_array will contain the addresses that have to be added into the whitelist. The
 * 					 format of the addr_array should be: address type followed by address.
 * 					 Example:
 * 					 @code
 * 					 uint8_t addr_array[] = {PUBLIC_ADDR,0x01,0x00,0x00,0xe1,0x80,0x02,
 * 					 	PUBLIC_ADDR,0x02,0x00,0x00,0xe1,0x80,0x02};
 * 					 @endcode
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_auto_conn_establish_proc_IDB05A1(uint16_t scanInterval, uint16_t scanWindow,
						 uint8_t own_bdaddr_type, uint16_t conn_min_interval,	
						 uint16_t conn_max_interval, uint16_t conn_latency,	
						 uint16_t supervision_timeout, uint16_t min_conn_length, 
						 uint16_t max_conn_length,
                         uint8_t num_whitelist_entries,
                         const uint8_t *addr_array);

tBleStatus aci_gap_start_auto_conn_establish_proc_IDB04A1(uint16_t scanInterval, uint16_t scanWindow,
						 uint8_t own_bdaddr_type, uint16_t conn_min_interval,	
						 uint16_t conn_max_interval, uint16_t conn_latency,	
						 uint16_t supervision_timeout, uint16_t min_conn_length, 
						 uint16_t max_conn_length,
                                                 uint8_t use_reconn_addr,
                                                 const tBDAddr reconn_addr,
                                                 uint8_t num_whitelist_entries,
                                                 const uint8_t *addr_array);
/**
 * @brief Start a general connection establishment procedure.
 * @note  The host enables scanning in the controller with the scanner filter policy set
 *        to “accept all advertising packets” and from the scanning results all the devices
 *        are sent to the upper layer using the event @ref EVT_BLUE_GAP_DEVICE_FOUND (for IDB04A1) or @ref EVT_LE_ADVERTISING_REPORT (for IDB05A1).
 *        The upper layer then has to select one of the devices to which it wants to connect
 *        by issuing the command aci_gap_create_connection(). The procedure is terminated
 *        when a connection is established or the upper layer terminates the procedure by
 *        issuing the command aci_gap_terminate_gap_procedure() with the procedure code set to
 *        @ref GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC. On completion of the procedure a
 *        @ref EVT_BLUE_GAP_PROCEDURE_COMPLETE event is generated with the procedure code set to
 *        @ref GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC.
 * @param scan_type Passive or active scanning (@ref PASSIVE_SCAN, @ref ACTIVE_SCAN)
 * @param scan_interval Time interval from when the Controller started its last LE scan until it begins
 * 					   the subsequent LE scan. The scan interval should be a number in the range
 * 					   0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					   For a number N, Time = N x 0.625 msec.
 * @param scan_window Amount of time for the duration of the LE scan. Scan_Window shall be less than
 * 					 or equal to Scan_Interval. The scan window should be a number in the range
 * 					 0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					 For a number N, Time = N x 0.625 msec.
 * @param own_address_type Type of our address used during active scanning (@ref PUBLIC_ADDR, @ref STATIC_RANDOM_ADDR).
 * @param filter_duplicates Duplicate filtering enabled or not.
 * 						   @arg 0x00: Do not filter the duplicates
 * 						   @arg 0x01: Filter duplicates
 * @cond BLUENRG
 * @param use_reconn_addr If 1, the provided reconnection address is used as our address during the procedure (the address
 * 						  has been previously notified to the application through @ref EVT_BLUE_GAP_RECONNECTION_ADDRESS event).\n
 * @param reconn_addr	Reconnection address used if use_reconn_addr is 1.
 * @endcond
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_general_conn_establish_proc_IDB05A1(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window,
						 uint8_t own_address_type, uint8_t filter_duplicates);
tBleStatus aci_gap_start_general_conn_establish_proc_IDB04A1(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window,
						 uint8_t own_address_type, uint8_t filter_duplicates, uint8_t use_reconn_addr, const tBDAddr reconn_addr);

/**
 * @brief Start a selective connection establishment procedure.
 * @note  The GAP adds the specified device addresses into white list and enables scanning in
 * 		  the controller with the scanner filter policy set to “accept packets only from
 * 		  devices in whitelist”. All the devices found are sent to the upper layer by the
 * 		  event @ref EVT_BLUE_GAP_DEVICE_FOUND (for IDB04A1) or @ref EVT_LE_ADVERTISING_REPORT (for IDB05A1). The upper layer then has to select one of the
 * 		  devices to which it wants to connect by issuing the command aci_gap_create_connection().
 * 		  On completion of the procedure a  @ref EVT_BLUE_GAP_PROCEDURE_COMPLETE event is generated
 * 		  with the procedure code set to @ref GAP_SELECTIVE_CONNECTION_ESTABLISHMENT_PROC.
 * 		  The procedure is terminated when a connection is established or the upper layer terminates
 * 		  the procedure by issuing the command aci_gap_terminate_gap_procedure with the procedure
 * 		  code set to @ref GAP_SELECTIVE_CONNECTION_ESTABLISHMENT_PROC.
 * @param scan_type Passive or active scanning (@ref PASSIVE_SCAN, @ref ACTIVE_SCAN)
 * @param scan_interval Time interval from when the Controller started its last LE scan until it begins
 * 					   the subsequent LE scan. The scan interval should be a number in the range
 * 					   0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					   For a number N, Time = N x 0.625 msec.
 * @param scan_window Amount of time for the duration of the LE scan. Scan_Window shall be less than
 * 					 or equal to Scan_Interval. The scan window should be a number in the range
 * 					 0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					 For a number N, Time = N x 0.625 msec.
 * @param own_address_type Type of our address used during active scanning (@ref PUBLIC_ADDR, @ref STATIC_RANDOM_ADDR).
 * @param filter_duplicates Duplicate filtering enabled or not.
 * 						   @arg 0x00: Do not filter the duplicates
 * 						   @arg 0x01: Filter duplicates
 * @param num_whitelist_entries Number of devices that have to be added to the whitelist.
 * @param addr_array addr_array will contain the addresses that have to be added into the whitelist. The
 * 					 format of the addr_array should be: address type followed by address.
 * 					 Example:
 * 					 @code
 * 					 uint8_t addr_array[] = {PUBLIC_ADDR,0x01,0x00,0x00,0xe1,0x80,0x02,
 * 					 	PUBLIC_ADDR,0x02,0x00,0x00,0xe1,0x80,0x02};
 * 					 @endcode
 *
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_selective_conn_establish_proc(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window,
						 uint8_t own_address_type, uint8_t filter_duplicates, uint8_t num_whitelist_entries,
                         const uint8_t *addr_array);

/**
 * @brief Start the direct connection establishment procedure.
 * @note  A LE_Create_Connection call will be made to the controller by GAP with the initiator filter
 * 		  policy set to “ignore whitelist and process connectable advertising packets only for the
 * 		  specified device”. The procedure can be terminated explicitly by the upper layer by issuing
 * 		  the command aci_gap_terminate_gap_procedure(). When a command is issued to terminate the
 * 		  procedure by upper layer, a LE_Create_Connection_Cancel call will be made to the controller
 * 		  by GAP.
 * 		  On termination of the procedure, a @ref EVT_LE_CONN_COMPLETE event is returned. The procedure can
 * 		  be explicitly terminated by the upper layer by issuing the command
 * 		  aci_gap_terminate_gap_procedure() with the procedure_code set to @ref GAP_DIRECT_CONNECTION_ESTABLISHMENT_PROC.
 * @param scanInterval Time interval from when the Controller started its last LE scan until it begins
 * 					   the subsequent LE scan. The scan interval should be a number in the range
 * 					   0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					   For a number N, Time = N x 0.625 msec.
 * @param scanWindow Amount of time for the duration of the LE scan. Scan_Window shall be less than
 * 					 or equal to Scan_Interval. The scan window should be a number in the range
 * 					 0x0004 to 0x4000. This corresponds to a time range 2.5 msec to 10240 msec.
 * 					 For a number N, Time = N x 0.625 msec.
 * @param peer_bdaddr_type Type of the peer address (@ref PUBLIC_ADDR, @ref STATIC_RANDOM_ADDR).
 * @param peer_bdaddr Address of the peer device with which a connection has to be established.
 * @param own_bdaddr_type Type of our address used during advertising (PUBLIC_ADDR,STATIC_RANDOM_ADDR).
 * @param conn_min_interval Minimum value for the connection event interval. This shall be less than or
 * 							equal to Conn_Interval_Max.\n
 * 							Range: 0x0006 to 0x0C80\n
 * 							Time = N x 1.25 msec\n
 * 							Time Range: 7.5 msec to 4 seconds
 * @param conn_max_interval Maximum value for the connection event interval. This shall be greater than or
 * 							equal to Conn_Interval_Min.\n
 * 							Range: 0x0006 to 0x0C80\n
 * 							Time = N x 1.25 msec\n
 * 							Time Range: 7.5 msec to 4 seconds
 * @param conn_latency Slave latency for the connection in number of connection events.\n
 * 						 Range: 0x0000 to 0x01F4
 * @param supervision_timeout Supervision timeout for the LE Link.\n
 * 							  Range: 0x000A to 0x0C80\n
 * 							  Time = N x 10 msec\n
 * 							  Time Range: 100 msec to 32 seconds
 * @param min_conn_length Minimum length of connection needed for the LE connection.\n
 * 						  Range: 0x0000 - 0xFFFF\n
 * 						  Time = N x 0.625 msec.
 * @param max_conn_length Maximum length of connection needed for the LE connection.\n
 * 						  Range: 0x0000 - 0xFFFF\n
 * 						  Time = N x 0.625 msec.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_create_connection(uint16_t scanInterval, uint16_t scanWindow,
				     uint8_t peer_bdaddr_type, tBDAddr peer_bdaddr,	
				     uint8_t own_bdaddr_type, uint16_t conn_min_interval,	
				     uint16_t conn_max_interval, uint16_t conn_latency,	
				     uint16_t supervision_timeout, uint16_t min_conn_length, 
				     uint16_t max_conn_length);

/**
 * @brief Terminate the specified GAP procedure. @ref EVT_BLUE_GAP_PROCEDURE_COMPLETE event is
 *  	  returned with the procedure code set to the corresponding procedure.
 * @param procedure_code One of the procedure codes (@ref gap_procedure_codes "GAP procedure codes").
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_terminate_gap_procedure(uint8_t procedure_code);

/**
 * @brief Start the connection parameter update procedure.
 * @note  Allowed by the Central to update the connection parameter of the specified connection.
 * 		  A Link Layer Connection Update procedure is started on the controller.
 * 		  On completion of the procedure, a @ref EVT_LE_CONN_UPDATE_COMPLETE event is returned to
 * 		  the upper layer.
 * @param conn_handle Handle of the connection for which the update procedure has to be started.
 * @param conn_min_interval Minimum value for the connection event interval. This shall be less than or
 * 							equal to Conn_Interval_Max.\n
 * 							Range: 0x0006 to 0x0C80\n
 * 							Time = N x 1.25 msec\n
 * 							Time Range: 7.5 msec to 4 seconds
 * @param conn_max_interval Maximum value for the connection event interval. This shall be greater than or
 * 							equal to Conn_Interval_Min.\n
 * 							Range: 0x0006 to 0x0C80\n
 * 							Time = N x 1.25 msec\n
 * 							Time Range: 7.5 msec to 4 seconds
 * @param conn_latency Slave latency for the connection in number of connection events.\n
 * 						 Range: 0x0000 to 0x01F4
 * @param supervision_timeout Supervision timeout for the LE Link.\n
 * 							  Range: 0x000A to 0x0C80\n
 * 							  Time = N x 10 msec\n
 * 							  Time Range: 100 msec to 32 seconds
 * @param min_conn_length Minimum length of connection needed for the LE connection.\n
 * 						  Range: 0x0000 - 0xFFFF\n
 * 						  Time = N x 0.625 msec.
 * @param max_conn_length Maximum length of connection needed for the LE connection.\n
 * 						  Range: 0x0000 - 0xFFFF\n
 * 						  Time = N x 0.625 msec.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_connection_update(uint16_t conn_handle, uint16_t conn_min_interval,	
                                           uint16_t conn_max_interval, uint16_t conn_latency,	
                                           uint16_t supervision_timeout, uint16_t min_conn_length, 
                                           uint16_t max_conn_length);

/**
 * @brief Send a pairing request.
 * @note  Send the SM pairing request to start a pairing process from a Central. The authentication
 * 		  requirements and  IO capabilities should be set before issuing this command using
 * 		  aci_gap_set_io_capability() and aci_gap_set_auth_requirement().
 * 		  A @ref EVT_BLUE_GAP_PAIRING_CMPLT event is returned after the pairing process is completed.
 * @param conn_handle Handle of the connection for which the pairing request has to be sent.
 * @param force_rebond  @arg 0x00: Pairing request is sent only if the device has not previously bonded
 * 						@arg 0x01: Pairing request will be sent even if the device was previously bonded
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_send_pairing_request(uint16_t conn_handle, uint8_t force_rebond);

/**
 * @brief Resolve a private address.
 * @note  This command tries to resolve the address provided with the IRKs present in its database. If
 * 		  the address is resolved successfully with any one of the IRKs present in the database, it
 * 		  returns success.
 * @param[in] address Address to be resolved.
 * @param[out] actual_address The public or static random address of the peer device, distributed during pairing phase.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_resolve_private_address_IDB05A1(const tBDAddr private_address, tBDAddr actual_address);

/**
 * @brief Resolve a private address.
 * @note  This command tries to resolve the address provided with the IRKs present in its database. If
 * 		  the address is resolved successfully with any one of the IRKs present in the database, it
 * 		  returns success.
 * @param address Address to be resolved.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_resolve_private_address_IDB04A1(const tBDAddr private_address);

/**
 * @brief This command gets the list of bonded devices.
 * @note  It returns the number of addresses and the corresponding address types and values.
 *        Example:
 *        @code
 *        tBleStatus ret;
 *        uint8_t num_devices = 0;
 *        uint8_t device_list[12*7];
 *        ret = aci_gap_get_bonded_devices(&num_devices, device_list, sizeof(device_list));
 *        for(int i = 0; i < num_devices; i+=7){
 *        	uint8_t addr_type = device_list[i];
 *        	uint8_t addr = device_list[i+1];
 *        	printf("Type: %d, Addr: %02X%02X%02X%02X%02X%02X\n",addr_type,addr[5],addr[4],addr[3],addr[2],addr[1],addr[0]);
 *        }
 *        @endcode
 *
 * @param[out] num_devices The number of bonded devices.
 * @param[out] device_list List of addresses. It contains a sequence of [address type, address] pairs, where address
 *        				  type can be @ref PUBLIC_ADDR or @arg @ref STATIC_RANDOM_ADDR.
 * @param device_list_size Maximum size of the device_list buffer used to return the device list.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_get_bonded_devices(uint8_t *num_devices, uint8_t *device_list, uint8_t device_list_size);

/**
 * @brief Puts the device into broadcast mode
 * @note  A privacy enabled device uses either a resolvable private address or a non-resolvable private address
 * 		  as specified in the own_addr_type parameter of the command.
 * @param adv_interv_min    Minimum advertising interval.
 * 							Range: 0x00A0 to 0x4000
 * 							Time = N * 0.625 msec
 * 							Time Range: 100 ms to 10.24 sec
 * @param adv_interv_max    Maximum advertising interval.
 * 						    Range: 0x00A0 to 0x4000
 * 							Time = N * 0.625 msec
 * 							Time Range: 100 ms to 10.24 sec
 * @param adv_type One of the allowed advertising types:
 *                @arg @ref ADV_SCAN_IND Scannable undirected advertising
 *                @arg @ref ADV_NONCONN_IND Non connectable undirected advertising
 * @param own_addr_type If Privacy is disabled, the broadcaster address can be
 * 			               @arg @ref PUBLIC_ADDR.
 * 			               @arg @ref STATIC_RANDOM_ADDR.
 *                         If Privacy is enabled, then the broadcaster address can be
 *                         @arg @ref RESOLVABLE_PRIVATE_ADDR
 *                         @arg @ref NON_RESOLVABLE_PRIVATE_ADDR
 * @param adv_data_length Length of the advertising data in the advertising packet
 * @param adv_data		  Advertising data used by the device while advertising
 * @param num_whitelist_entries Number of devices to be added to whitelist
 * @param addr_array It will contain the addresses that have to be added into the whitelist. The
 * 					 format of the addr_array should be: address type followed by address.
 * 					 Example:
 * 					 @code
 * 					 uint8_t addr_array[] = {PUBLIC_ADDR,0x01,0x00,0x00,0xe1,0x80,0x02,
 * 					 	PUBLIC_ADDR,0x02,0x00,0x00,0xe1,0x80,0x02};
 * 					 @endcode
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_set_broadcast_mode(uint16_t adv_interv_min, uint16_t adv_interv_max, uint8_t adv_type,
                                      uint8_t own_addr_type, uint8_t adv_data_length, const uint8_t *adv_data,  uint8_t num_whitelist_entries,
                                      const uint8_t *addr_array);

/**
 * @brief Starts an observation procedure, when the device is in Observer role.
 * @note  The host enables scanning in the controller. The advertising reports are sent to the upper layer
 *        using standard @ref EVT_LE_ADVERTISING_REPORT subevent in @ref EVT_LE_META_EVENT. See Bluetooth
 *        Core v4.0, Vol. 2, part E, Ch. 7.7.65.2, LE Advertising Report Event.
 * @param scan_interval Time interval from when the Controller started its last LE scan until it begins the subsequent LE scan.
 * 		  The scan interval should be a number in the range 0x0004 to 0x4000. This corresponds to a time range from 2.5 msec
 * 		  to 10240 msec. For a number N, Time = N * 0.625 msec.
 * @param scan_window Amount of time for the duration of the LE scan. scan_window shall be less than or equal to scan_interval.
 * 		  The scan window should be a number in the range 0x0004 to 0x4000. This corresponds to a time range from 2.5 msec
 * 		  to 10240 msec. For a number N, Time = N * 0.625 msec.
 * @param scan_type Passive or active scanning (@ref PASSIVE_SCAN, @ref ACTIVE_SCAN)
 * @param own_address_type If Privacy is disabled, then the scanner address can be
 * 			               @arg @ref PUBLIC_ADDR.
 * 			               @arg @ref STATIC_RANDOM_ADDR.
 *                         If Privacy is enabled, then the scanner address can be
 *                         @arg @ref RESOLVABLE_PRIVATE_ADDR
 *                         @arg @ref NON_RESOLVABLE_PRIVATE_ADDR
 * @param filter_duplicates Duplicate filtering enabled or not.
 * 						   @arg 0x00: Do not filter the duplicates
 * 						   @arg 0x01: Filter duplicates
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_start_observation_procedure(uint16_t scan_interval, uint16_t scan_window, uint8_t scan_type,
						 uint8_t own_address_type, uint8_t filter_duplicates);

/**
 * @brief The command finds whether a device is bonded.
 * @note If the device is using a resolvable private address and it has been bonded, then the command will return
 *       BLE_STATUS_SUCCESS.
 * @param peer_address_type The address type of the peer device
 *                         @arg @ref PUBLIC_ADDR.
 * 			               @arg @ref RANDOM_ADDR.
 * @param peer_address Address used by the peer device while advertising.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gap_is_device_bonded(uint8_t peer_address_type, const tBDAddr peer_address);

/**
 * @}
 */

/**
 * @defgroup GAP_Events GAP events
 * @{
 */

/**
 * This event is generated by the controller when the limited discoverable
 *  mode ends due to timeout (180 seconds). No parameters in the event.
 */
#define EVT_BLUE_GAP_LIMITED_DISCOVERABLE     (0x0400)


/**
 * This event is generated when the pairing process has completed successfully
 * or a pairing procedure timeout has occurred or the pairing has failed.
 * This is to notify the application that we have paired with a remote device
 * so that it can take further actions or to notify that a timeout has occurred
 *  so that the upper layer can decide to disconnect the link. See @ref _evt_gap_pairing_cmplt.
 */
#define EVT_BLUE_GAP_PAIRING_CMPLT                (0x0401)
typedef __packed struct _evt_gap_pairing_cmplt{
  uint16_t conn_handle; /**< Connection handle on which the pairing procedure completed */
  /**
   * 0x00: Pairing Success. Pairing with a remote device was successful\n
   * 0x01: Pairing Timeout. The SMP timeout has elapsed and no further SMP commands will be processed until reconnection\n
   * 0x02: Pairing Failed. The pairing failed with the remote device.
   */
  uint8_t  status;
} PACKED evt_gap_pairing_cmplt;


/**
 * This event is generated by the Security manager to the application when a pass key is required for pairing.
 * When this event is received, the application has to respond with the aci_gap_pass_key_response() command.
 * See @ref _evt_gap_pass_key_req.
 */
#define EVT_BLUE_GAP_PASS_KEY_REQUEST             (0x0402)
typedef __packed struct _evt_gap_pass_key_req{
  uint16_t conn_handle; /**< Connection handle for which the passkey has been requested. */
} PACKED evt_gap_pass_key_req;


/**
 * This event is generated by the Security manager to the application when the application
 * has set that authorization is required for reading/writing of attributes. This event will
 * be generated as soon as the pairing is complete. When this event is received,
 * aci_gap_authorization_response() command should be used by the application.
 * See @ref _evt_gap_author_req.
 */
#define EVT_BLUE_GAP_AUTHORIZATION_REQUEST        (0x0403)
typedef __packed struct _evt_gap_author_req{
  uint16_t conn_handle; /**< Connection handle for which authorization has been requested. */
} PACKED evt_gap_author_req;

/**
 * This event is generated when the slave security request is successfully sent to the master.
 * No parameters for this event.
 */
#define EVT_BLUE_GAP_SLAVE_SECURITY_INITIATED     (0X0404)

/**
 * This event is generated on the slave when a aci_gap_slave_security_request()  is called to reestablish the bond
 * with a master but the master has lost the bond. When this event is received, the upper layer has to issue the
 * command aci_gap_allow_rebond() in order to allow the slave to continue the pairing process with the master.
 * On the master this event is raised when aci_gap_send_pairing_request() is called to reestablish a bond with a slave
 * but the slave has lost the bond. In order to create a new bond the master has to launch aci_gap_send_pairing_request()
 * with force_rebond set to 1.
 * No parameters for this event
 */
#define EVT_BLUE_GAP_BOND_LOST                    (0X0405)

/**
 * The event is given by the GAP layer to the upper layers when a device is discovered during scanning
 * as a consequence of one of the GAP procedures started by the upper layers. See @ref _evt_gap_device_found.
 */
#define EVT_BLUE_GAP_DEVICE_FOUND                 (0x0406)
typedef __packed struct _evt_gap_device_found{
  	uint8_t		evt_type;     /**< Type of event (@ref ADV_IND, @ref ADV_DIRECT_IND, @ref ADV_SCAN_IND, @ref ADV_NONCONN_IND, @ref SCAN_RSP) */
	uint8_t		bdaddr_type;  /**< Type of the peer address (@ref PUBLIC_ADDR, @ref RANDOM_ADDR). */
	tBDAddr	    bdaddr;       /**< Address of the peer device found during scanning. */
	uint8_t		data_length;  /**< Length of advertising or scan response data. */
	uint8_t		data_RSSI[VARIABLE_SIZE]; /**< Advertising or scan response data + RSSI. RSSI is last octect (signed integer). */
} PACKED evt_gap_device_found;

/**
 * This event is sent by the GAP to the upper layers when a procedure previously started has been terminated
 * by the upper layer or has completed for any other reason. See @ref _evt_gap_procedure_complete.
 */
#define EVT_BLUE_GAP_PROCEDURE_COMPLETE           (0x0407)
typedef __packed struct _evt_gap_procedure_complete{
  uint8_t procedure_code; /**< Terminated procedure. See @ref gap_procedure_codes "GAP procedure codes". */
  /**
   * @ref BLE_STATUS_SUCCESS, @ref BLE_STATUS_FAILED or @ref ERR_AUTH_FAILURE (procedure failed
   * due to authentication requirements).
   */
  uint8_t status;
  /**
   * Procedure specific data.\n
   * @li For Name Discovery Procedure:\n
   * the name of the peer device if the procedure completed successfully.
   * @li For General Connection Establishment Procedure:\n
   * The reconnection address written to the peripheral device if the peripheral is privacy enabled
   */
  uint8_t data[VARIABLE_SIZE];
} PACKED evt_gap_procedure_complete;

/**
 * This event is sent only by a privacy enabled Peripheral. The event is sent to the upper layers when the peripheral
 * is not able to resolve the private address of the peer device after connecting to it.
 */
#define EVT_BLUE_GAP_ADDR_NOT_RESOLVED_IDB05A1          (0x0408)
typedef __packed struct _evt_gap_addr_not_resolved_IDB05A1{
  uint16_t conn_handle; /**< Connection handle for which the private address could not be resolved with any of the stored IRK's.  */
} PACKED evt_gap_addr_not_resolved_IDB05A1;

/**
 * This event is raised when the reconnection address is generated during the general connection
 * establishment procedure. The same address is set into the peer device also as a part of the general
 * connection establishment procedure. In order to make use of the reconnection address the next time
 * while connecting to the bonded peripheral, the application needs to use this reconnection address
 * as its own address as well as the peer address to which it wants to connect. See aci_gap_start_general_conn_establish_proc()
 * and aci_gap_start_auto_conn_establish_proc().
 */
#define EVT_BLUE_GAP_RECONNECTION_ADDRESS_IDB04A1       (0x0408)
typedef __packed struct _evt_gap_reconnection_addr_IDB04A1{
  uint8_t reconnection_address[6]; /**< 6 bytes of reconnection address that has been generated */
} PACKED evt_gap_reconnection_addr_IDB04A1;

/**
 * @}
 */

/**
 * @}
 */

 /**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* __BLUENRG_GAP_ACI_H__ */
