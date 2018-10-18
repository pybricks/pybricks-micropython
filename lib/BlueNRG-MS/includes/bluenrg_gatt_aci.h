/******************** (C) COPYRIGHT 2014 STMicroelectronics ********************
* File Name          : bluenrg_gatt_aci.h
* Author             : AMS - AAS
* Version            : V1.0.0
* Date               : 26-Jun-2014
* Description        : Header file with GATT commands for BlueNRG FW6.3.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#ifndef __BLUENRG_GATT_ACI_H__
#define __BLUENRG_GATT_ACI_H__

#include "bluenrg_gatt_server.h"

/** 
 * @addtogroup HIGH_LEVEL_INTERFACE HIGH_LEVEL_INTERFACE
 * @{
 */

/**
 * @addtogroup ACI_GATT ACI_GATT
 * @brief API for GATT layer.
 * @{
 */
 
/**
 * @defgroup GATT_Functions GATT functions
 * @brief API for GATT layer.
 * @{
 */

/**
  * @brief  Initialize the GATT layer for server and client roles.
  * @note   It adds also the GATT service with Service Changed Characteristic.
  *         Until this command is issued the GATT channel will not process any commands
  *         even if the connection is opened. This command has to be given
  *         before using any of the GAP features.
  * @return Value indicating success or error code.
  */
tBleStatus aci_gatt_init(void);

/**
 * @brief Add a service to the GATT Server. When a service is created in the server, the Host needs
 *        to reserve the handle ranges for this service using max_attr_records parameter. This
 *        parameter specifies the maximum number of attribute records that can be added to this
 *        service (including the service attribute, include attribute, characteristic attribute,
 *        characteristic value attribute and characteristic descriptor attribute). Handle of the
 *        created service is returned.
 * @note  Service declaration is taken from the service pool. The attributes for characteristics and descriptors
 *        are allocated from the attribute pool.
 * @param service_uuid_type Type of service UUID (16-bit or 128-bit). See @ref UUID_Types "UUID Types".
 * @param[in] service_uuid 16-bit or 128-bit UUID based on the UUID Type field
 * @param service_type Primary or secondary service. See @ref Service_type "Service Type".
 * @param max_attr_records Maximum number of attribute records that can be added to this service
 *                         (including the service declaration itself)
 * @param[out] serviceHandle Handle of the Service. When this service is added to the service,
 * 							 a handle is allocated by the server to this service. Server also
 * 							 allocates a range of handles for this service from serviceHandle to
 * 							 <serviceHandle + max_attr_records>.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_add_serv(uint8_t service_uuid_type,
			     const uint8_t* service_uuid,
			     uint8_t service_type,
			     uint8_t max_attr_records,
			     uint16_t *serviceHandle);

/**
 * @brief Include a service given by included_start_handle and included_end_handle to another service
 * 		  given by service_handle. Attribute server creates an INCLUDE definition attribute and return
 * 		  the handle of this attribute in included_handle.
 * @param service_handle Handle of the service to which another service has to be included
 * @param included_start_handle Start Handle of the service which has to be included in service
 * @param included_end_handle End Handle of the service which has to be included in service
 * @param included_uuid_type Type of UUID for included service (16-bit or 128-bit). See @ref Well-Known_UUIDs "Well-Known UUIDs".
 * @param[in] included_uuid 16-bit or 128-bit UUID.
 * @param[out] included_handle Handle of the include declaration.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_include_service(uint16_t service_handle, uint16_t included_start_handle,
				    uint16_t included_end_handle, uint8_t included_uuid_type,
				    const uint8_t* included_uuid, uint16_t *included_handle);

/**
 * @brief Add a characteristic to a service.
 * @param serviceHandle Handle of the service to which the characteristic has to be added.
 * @param charUuidType Type of characteristic UUID (16-bit or 128-bit). See @ref UUID_Types "UUID Types".
 *         @arg @ref UUID_TYPE_16
 *         @arg @ref UUID_TYPE_128
 * @param charUuid 16-bit or 128-bit UUID.
 * @param charValueLen Maximum length of the characteristic value.
 * @param charProperties Bitwise OR values of Characteristic Properties (defined in Volume 3,
 *        Section 3.3.3.1 of Bluetooth Specification 4.0). See @ref Char_properties "Characteristic properties".
 * @param secPermissions Security permissions for the added characteristic. See @ref Security_permissions "Security permissions".
 * 			@arg ATTR_PERMISSION_NONE
 * 			@arg ATTR_PERMISSION_AUTHEN_READ
 * 			@arg ATTR_PERMISSION_AUTHOR_READ
 * 			@arg ATTR_PERMISSION_ENCRY_READ
 * 			@arg ATTR_PERMISSION_AUTHEN_WRITE
 * 			@arg ATTR_PERMISSION_AUTHOR_WRITE
 * 			@arg ATTR_PERMISSION_ENCRY_WRITE
 * @param gattEvtMask Bit mask that enables events that will be sent to the application by the GATT server
 * 					  on certain ATT requests. See @ref Gatt_Event_Mask "Gatt Event Mask".
 * 		   @arg GATT_DONT_NOTIFY_EVENTS
 * 		   @arg GATT_NOTIFY_ATTRIBUTE_WRITE
 * 		   @arg GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP
 * 		   @arg GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP
 * @param encryKeySize The minimum encryption key size requirement for this attribute. Valid Range: 7 to 16.
 * @param isVariable If the attribute has a variable length value field (1) or not (0).
 * @param charHandle Handle of the Characteristic that has been added. It is the handle of the characteristic declaration.
 * 		  The attribute that holds the characteristic value is allocated at the next handle, followed by the Client
 * 		  Characteristic Configuration descriptor if the characteristic has @ref CHAR_PROP_NOTIFY or @ref CHAR_PROP_INDICATE
 * 		  properties.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_add_char(uint16_t serviceHandle,
			     uint8_t charUuidType,
			     const uint8_t* charUuid, 
			     uint8_t charValueLen, 
			     uint8_t charProperties,
			     uint8_t secPermissions,
			     uint8_t gattEvtMask,
			     uint8_t encryKeySize,
			     uint8_t isVariable,
			     uint16_t* charHandle);

/**
 * Add a characteristic descriptor to a service.
 * @param serviceHandle Handle of the service to which the characteristic belongs
 * @param charHandle Handle of the characteristic to which description has to be added.
 * @param descUuidType 16-bit or 128-bit UUID. See @ref UUID_Types "UUID Types".
 *         @arg @ref UUID_TYPE_16
 *         @arg @ref UUID_TYPE_128
 * @param[in] uuid UUID of the Characteristic descriptor. It can be one of the UUID assigned by Bluetooth SIG
 * 		(Well_known_UUIDs) or a user-defined one.
 * @param descValueMaxLen The maximum length of the descriptor value
 * @param descValueLen Current Length of the characteristic descriptor value
 * @param[in] descValue Value of the characteristic description
 * @param secPermissions Security permissions for the added descriptor. See @ref Security_permissions "Security permissions".
 * 			@arg ATTR_PERMISSION_NONE
 * 			@arg ATTR_PERMISSION_AUTHEN_READ
 * 			@arg ATTR_PERMISSION_AUTHOR_READ
 * 			@arg ATTR_PERMISSION_ENCRY_READ
 * 			@arg ATTR_PERMISSION_AUTHEN_WRITE
 * 			@arg ATTR_PERMISSION_AUTHOR_WRITE
 * 			@arg ATTR_PERMISSION_ENCRY_WRITE
 * @param accPermissions Access permissions for the added descriptor. See @ref Access_permissions "Access permissions".
 * 			@arg ATTR_NO_ACCESS
 * 			@arg ATTR_ACCESS_READ_ONLY
 * 			@arg ATTR_ACCESS_WRITE_REQ_ONLY
 * 			@arg ATTR_ACCESS_READ_WRITE
 * 			@arg ATTR_ACCESS_WRITE_WITHOUT_RESPONSE
 * 			@arg ATTR_ACCESS_SIGNED_WRITE_ALLOWED
 * @param gattEvtMask Bit mask that enables events that will be sent to the application by the GATT server
 * 					  on certain ATT requests. See @ref Gatt_Event_Mask "Gatt Event Mask".
 * @param encryKeySize The minimum encryption key size requirement for this attribute. Valid Range: 7 to 16.
 * @param isVariable If the attribute has a variable length value field (1) or not (0).
 * @param[out] descHandle Handle of the Characteristic Descriptor.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_add_char_desc(uint16_t serviceHandle,
                                  uint16_t charHandle,
                                  uint8_t descUuidType,
                                  const uint8_t* uuid, 
                                  uint8_t descValueMaxLen,
                                  uint8_t descValueLen,
                                  const void* descValue, 
                                  uint8_t secPermissions,
                                  uint8_t accPermissions,
                                  uint8_t gattEvtMask,
                                  uint8_t encryKeySize,
                                  uint8_t isVariable,
                                  uint16_t* descHandle);

/**
 * @brief Update a characteristic value in a service.
 * @note If notifications (or indications) are enabled on that characteristic, a notification (or indication)
 *   	 will be sent to the client after sending this command to the BlueNRG. The command is queued into the
 *   	 BlueNRG command queue. If the buffer is full, because previous commands could not be still processed,
 *   	 the function will return @ref BLE_STATUS_INSUFFICIENT_RESOURCES. This will happen if notifications (or
 *   	 indications) are enabled and the application calls aci_gatt_update_char_value() at an higher rate
 *   	 than what is allowed by the link. Throughput on BLE link depends on connection interval and
 *   	 connection length parameters (decided by the master, see aci_l2cap_connection_parameter_update_request()
 *   	 for more info on how to suggest new connection parameters from a slave). If the application does not
 *   	 want to lose notifications because BlueNRG buffer becomes full, it has to retry again till the function
 *   	 returns @ref BLE_STATUS_SUCCESS or any other error code.\n
 *   	 Example:\n
 *   	 Here if BlueNRG buffer become full because BlueNRG was not able to send packets for a while, some
 *   	 notifications will be lost.
 *   	 @code
 *   	 tBleStatus Free_Fall_Notify(void)
 *		 {
 *		 	uint8_t val;
 * 			tBleStatus ret;
 *
 *			val = 0x01;
 *			ret = aci_gatt_update_char_value(accServHandle, freeFallCharHandle, 0, 1, &val);
 *
 *			if (ret != BLE_STATUS_SUCCESS){
 *			  PRINTF("Error while updating FFall characteristic.\n") ;
 *			  return BLE_STATUS_ERROR ;
 *			}
 *		    return BLE_STATUS_SUCCESS;
 *		 }
 *		 @endcode
 *		 Here if BlueNRG buffer become full, the application try again to send the notification.
 *		 @code
 *       struct timer t;
 *       Timer_Set(&t, CLOCK_SECOND*10);
 *       while(aci_gatt_update_char_value(chatServHandle,TXCharHandle,0,len,array_val)==BLE_STATUS_INSUFFICIENT_RESOURCES){
 *         // Radio is busy (buffer full).
 *         if(Timer_Expired(&t))
 *           break;
 *       }
 *       @endcode
 *
 * @param servHandle Handle of the service to which characteristic belongs
 * @param charHandle Handle of the characteristic
 * @param charValOffset The offset from which the attribute value has to be updated. If this is set to 0,
 * 						and the attribute value is of variable length, then the length of the attribute will
 * 						be set to the charValueLen. If the charValOffset is set to a value greater than 0,
 * 						then the length of the attribute will be set to the maximum length as specified for
 * 						the attribute while adding the characteristic.
 * @param charValueLen Length of the characteristic value in octets
 * @param[in] charValue Characteristic value
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_update_char_value(uint16_t servHandle, 
				      uint16_t charHandle,
				      uint8_t charValOffset,
				      uint8_t charValueLen,   
				      const void *charValue);
/**
 * @brief Delete the specified characteristic from the service.
 * @param servHandle Handle of the service to which characteristic belongs
 * @param charHandle Handle of the characteristic to be deleted
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_del_char(uint16_t servHandle, uint16_t charHandle);

/**
 * @brief Delete the specified service from the GATT server database.
 * @param servHandle Handle of the service to be deleted
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_del_service(uint16_t servHandle);

/**
 * @brief Delete the Include definition from the service.
 * @param servHandle Handle of the service to which Include definition belongs
 * @param includeServHandle Handle of the Included definition to be deleted
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_del_include_service(uint16_t servHandle, uint16_t includeServHandle);

/**
 * @brief Perform an ATT MTU exchange procedure.
 * @note  When the ATT MTU exchange procedure is completed, a @ref EVT_BLUE_ATT_EXCHANGE_MTU_RESP
 * 		  event is generated. A @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is also generated
 * 		  to indicate the end of the procedure.
 * @param conn_handle Connection handle for which the command is given.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_exchange_configuration(uint16_t conn_handle);

/**
 * @brief Send a @a Find @a Information @a Request.
 * @note This command is used to obtain the mapping of attribute handles with their associated
 * 		 types. The responses of the procedure are given through the
 * 		 @ref EVT_BLUE_ATT_FIND_INFORMATION_RESP event. The end of the procedure is indicated by
 * 		 a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event.
 * @param conn_handle Connection handle for which the command is given
 * @param start_handle Starting handle of the range of attributes to be discovered on the server
 * @param end_handle Ending handle of the range of attributes to be discovered on the server
 * @return Value indicating success or error code.
 */
tBleStatus aci_att_find_information_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle);

/**
 * @brief Send a @a Find @a By @a Type @a Value @a Request
 * @note The Find By Type Value Request is used to obtain the handles of attributes that
 * 		 have a given 16-bit UUID attribute type and a given attribute value.
 * 		 The responses of the procedure are given through the @ref EVT_BLUE_ATT_FIND_BY_TYPE_VAL_RESP event.
 * 		 The end of the procedure is indicated by a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event.
 * @param conn_handle Connection handle for which the command is given.
 * @param start_handle 	First requested handle number
 * @param end_handle 	Last requested handle number
 * @param uuid			2 octet UUID to find (little-endian)
 * @param attr_val_len  Length of attribute value (maximum value is ATT_MTU - 7).
 * @param attr_val		Attribute value to find
 * @return Value indicating success or error code.
 */
tBleStatus aci_att_find_by_type_value_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
                                          uint8_t* uuid, uint8_t attr_val_len, uint8_t* attr_val);

/**
 * @brief Send a @a Read @a By @a Type @a Request
 * @note  The Read By Type Request is used to obtain the values of attributes where the attribute type
 * 		  is known but the handle is not known.
 * 		  The responses of the procedure are given through the @ref EVT_BLUE_ATT_READ_BY_TYPE_RESP event.
 * 		  The end of the procedure is indicated by a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event.
 * @param conn_handle Connection handle for which the command is given.
 * @param start_handle First requested handle number
 * @param end_handle Last requested handle number
 * @param uuid_type @arg @ref UUID_TYPE_16
 *         			@arg @ref UUID_TYPE_128
 * @param uuid 2 or 16 octet UUID
 * @return Value indicating success or error code.
 */
tBleStatus aci_att_read_by_type_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
                                    uint8_t  uuid_type, uint8_t* uuid);

/**
 * @brief Send a @a Read @a By @a Group @a Type @a Request
 * @note The Read By Group Type Request is used to obtain the values of grouping attributes where the attribute
 * 		 type is known but the handle is not known. Grouping attributes are defined at GATT layer. The grouping
 * 		 attribute types are: «Primary Service», «Secondary Service» and «Characteristic».
 * 		 The responses of the procedure are given through the @ref EVT_BLUE_ATT_READ_BY_GROUP_TYPE_RESP event.
 * 		 The end of the procedure is indicated by a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE.
 * @param conn_handle Connection handle for which the command is given.
 * @param start_handle First requested handle number
 * @param end_handle Last requested handle number
 * @param uuid_type @arg @ref UUID_TYPE_16
 *         			@arg @ref UUID_TYPE_128
 * @param uuid 2 or 16 octet UUID
 * @return Value indicating success or error code.
 */
tBleStatus aci_att_read_by_group_type_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
                                    uint8_t  uuid_type, uint8_t* uuid);

/**
 * @brief Send a @a Prepare @a Write @a Request
 * @note  The Prepare Write Request is used to request the server to prepare to write the value of an attribute.
 * 		 The responses of the procedure are given through the @ref EVT_BLUE_ATT_PREPARE_WRITE_RESP event.
 * 		 The end of the procedure is indicated by a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE.
 * @param conn_handle Connection handle for which the command is given.
 * @param attr_handle The handle of the attribute to be written
 * @param value_offset The offset of the first octet to be written
 * @param attr_val_len Length of attribute value (maximum value is ATT_MTU - 5).
 * @param attr_val 	   The value of the attribute to be written
 * @return Value indicating success or error code.
 */
tBleStatus aci_att_prepare_write_req(uint16_t conn_handle, uint16_t attr_handle, uint16_t value_offset,
                                    uint8_t  attr_val_len, uint8_t* attr_val);

/**
 * @brief Send an @a Execute @a Write @a Request
 * @note The Execute Write Request is used to request the server to write or cancel the write of all the
 *  	 prepared values currently held in the prepare queue from this client.
 *  	 The result of the procedure is given through the @ref EVT_BLUE_ATT_EXEC_WRITE_RESP event.
 *  	 The end of the procedure is indicated by a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event.
 * @param conn_handle Connection handle for which the command is given.
 * @param execute @arg 0x00  Cancel all prepared writes
 * 				  @arg 0x01  Immediately write all pending prepared values.
 * @return Value indicating success or error code.
 */
tBleStatus aci_att_execute_write_req(uint16_t conn_handle, uint8_t execute);

/**
 * @brief This command will start the GATT client procedure to discover all primary services on the server.
 * @note  The responses of the procedure are given through the @ref EVT_BLUE_ATT_READ_BY_GROUP_TYPE_RESP event.
 * 		  The end of the procedure is indicated by a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event.
 * @param conn_handle Connection handle for which the command is given.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_disc_all_prim_services(uint16_t conn_handle);

/**
 * @brief Start the procedure to discover the primary services of the specified UUID on the server.
 * @note The responses of the procedure are given through the @ref EVT_BLUE_ATT_FIND_BY_TYPE_VAL_RESP event.
 *  	 The end of the procedure is indicated by a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event.
 * @param conn_handle Connection handle for which the command is given.
 * @param uuid_type @arg @ref UUID_TYPE_16
 *         			@arg @ref UUID_TYPE_128
 * @param uuid 2 or 16 octet UUID
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_disc_prim_service_by_uuid(uint16_t conn_handle, uint8_t uuid_type, uint8_t* uuid);

/**
 * @brief Start the procedure to find all included services.
 * @note The responses of the procedure are given through the @ref EVT_BLUE_ATT_READ_BY_TYPE_RESP event.
 * 		 The end of the procedure is indicated by a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event.
 * @param conn_handle Connection handle for which the command is given.
 * @param start_handle Start handle of the service
 * @param end_handle End handle of the service
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_find_included_services(uint16_t conn_handle, uint16_t start_handle,
					   uint16_t end_handle);

/**
 * @brief Start the procedure to discover all the characteristics of a given service.
 * @note When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * 		 Before procedure completion the response packets are given through @ref EVT_BLUE_ATT_READ_BY_TYPE_RESP event.
 * @param conn_handle Connection handle for which the command is given
 * @param start_attr_handle Start attribute handle of the service
 * @param end_attr_handle End attribute handle of the service
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_disc_all_charac_of_serv(uint16_t conn_handle, uint16_t start_attr_handle,
					    uint16_t end_attr_handle);

/**
 * @brief Start the procedure to discover all the characteristics specified by a UUID.
 * @note  When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 *  	  Before procedure completion the response packets are given through
 *        @ref EVT_BLUE_GATT_DISC_READ_CHAR_BY_UUID_RESP event.
 * @param conn_handle Connection handle for which the command is given
 * @param start_handle 	Start attribute handle of the service
 * @param end_handle 	End attribute handle of the service
 * @param uuid_type @arg @ref UUID_TYPE_16
 *         			@arg @ref UUID_TYPE_128
 * @param uuid 2 or 16 octet UUID
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_disc_charac_by_uuid(uint16_t conn_handle, uint16_t start_handle,
				                     uint16_t end_handle, uint8_t uuid_type, const uint8_t* uuid);

/**
 * @brief Start the procedure to discover all characteristic descriptors on the server.
 * @note When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * 		 Before procedure completion the response packets are given through @ref EVT_BLUE_ATT_FIND_INFORMATION_RESP event.
 * @param conn_handle Connection handle for which the command is given.
 * @param char_val_handle Starting handle of the characteristic
 * @param char_end_handle End handle of the characteristic
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_disc_all_charac_descriptors(uint16_t conn_handle, uint16_t char_val_handle,
						uint16_t char_end_handle);

/**
 * @brief Start the procedure to read the attribute value.
 * @note  When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * 		  Before procedure completion the response packet is given through @ref EVT_BLUE_ATT_READ_RESP event.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the characteristic to be read
 * @return Value indicating success or error code.\n
 * 		 It can be @ref BLE_STATUS_NOT_ALLOWED in the following cases:\n
 *		- If the exchange has already taken place\n
 *		- If GATT is expecting response for previous request\n
 *		- Already a request is in the queue to be sent\n
 *		- Channel not open\n
 *		- Already one GATT procedure is started
 */
tBleStatus aci_gatt_read_charac_val(uint16_t conn_handle, uint16_t attr_handle);

/**
 * @brief Start the procedure to read all the characteristics specified by the UUID.
 * @note When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * 		 Before procedure completion the response packets are given through
 * 		 @ref EVT_BLUE_GATT_DISC_READ_CHAR_BY_UUID_RESP event.
 * @param conn_handle Connection handle for which the command is given
 * @param start_handle Starting handle of the range to be searched
 * @param end_handle End handle of the range to be searched
 * @param uuid_type @arg @ref UUID_TYPE_16
 *         			@arg @ref UUID_TYPE_128
 * @param uuid 2 or 16 octet UUID
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_read_using_charac_uuid(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
                                    uint8_t  uuid_type, uint8_t* uuid);

/**
 * @brief Start the procedure to read a long characteristic value.
 * @note When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * 		 Before procedure completion the response packets are given through @ref EVT_BLUE_ATT_READ_BLOB_RESP event.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the characteristic to be read
 * @param val_offset Offset from which the value needs to be read
 * @return Value indicating success or error code.\n
 * 		 It can be @ref BLE_STATUS_NOT_ALLOWED in the following cases:\n
 *		- If the exchange has already taken place\n
 *		- If GATT is expecting response for previous request\n
 *		- Already a request is in the queue to be sent\n
 *		- Channel not open\n
 *		- Already one GATT procedure is started
 */
tBleStatus aci_gatt_read_long_charac_val(uint16_t conn_handle, uint16_t attr_handle,
					 uint16_t val_offset);

/**
 * @brief Start a procedure to read multiple characteristic values from a server.
 * @note This sub-procedure is used to read multiple Characteristic Values from a server when the
 *  	 client knows the Characteristic Value Handles.
 *  	 When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 *  	 Before procedure completion the response packets are given through
 *  	 @ref EVT_BLUE_ATT_READ_MULTIPLE_RESP event.
 * @param conn_handle Connection handle for which the command is given
 * @param num_handles The number of handles for which the value has to be read
 * @param set_of_handles The handles for which the attribute value has to be read
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_read_multiple_charac_val(uint16_t conn_handle, uint8_t num_handles, 
                                             uint8_t* set_of_handles);

/**
 * @brief Start the procedure to write a characteristic value.
 * @note When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the characteristic to be written
 * @param value_len Length of the value to be written
 * @param[in] attr_value Value to be written
 * @return Value indicating success or error code.\n
 * 		 It can be @ref BLE_STATUS_NOT_ALLOWED in the following cases:\n
 *		- If the exchange has already taken place\n
 *		- If GATT is expecting response for previous request\n
 *		- Already a request is in the queue to be sent\n
 *		- Channel not open\n
 *		- Already one GATT procedure is started
 */
tBleStatus aci_gatt_write_charac_value(uint16_t conn_handle, uint16_t attr_handle,
				       uint8_t value_len, uint8_t *attr_value);

/**
 * @brief Start the procedure to write a long characteristic value.
 * @note When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * 		 During the procedure, @ref EVT_BLUE_ATT_PREPARE_WRITE_RESP and @ref EVT_BLUE_ATT_EXEC_WRITE_RESP
 * 		 events are raised.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the attribute to be written
 * @param val_offset Offset at which the attribute has to be written
 * @param val_len Length of the value to be written
 * @param attr_val Value to be written
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_write_long_charac_val(uint16_t conn_handle, uint16_t attr_handle,
                                          uint16_t val_offset, uint8_t val_len, const uint8_t* attr_val);

/**
 * @brief Start the procedure to write a characteristic reliably.
 * @note When the procedure is completed, a  @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 *  	 During the procedure, @ref EVT_BLUE_ATT_PREPARE_WRITE_RESP and @ref EVT_BLUE_ATT_EXEC_WRITE_RESP
 * 		 events are raised.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the attribute to be written
 * @param val_offset Offset at which the attribute has to be written
 * @param val_len Length of the value to be written
 * @param attr_val Value to be written
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_write_charac_reliable(uint16_t conn_handle, uint16_t attr_handle,
                                          uint16_t val_offset, uint8_t val_len, uint8_t* attr_val);

/**
 * @brief Start the procedure to write a long characteristic descriptor.
 * @note When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * 		 During the procedure, @ref EVT_BLUE_ATT_PREPARE_WRITE_RESP and @ref EVT_BLUE_ATT_EXEC_WRITE_RESP
 * 		 events are raised.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the attribute to be written
 * @param val_offset Offset at which the attribute has to be written
 * @param val_len Length of the value to be written
 * @param attr_val Value to be written
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_write_long_charac_desc(uint16_t conn_handle, uint16_t attr_handle,
                                          uint16_t val_offset, uint8_t val_len, uint8_t* attr_val);

/**
 * @brief Start the procedure to read a long characteristic value.
 * @note  When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * 		  Before procedure completion the response packets are given through @ref EVT_BLUE_ATT_READ_BLOB_RESP
 * 		  event.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the characteristic descriptor
 * @param val_offset Offset from which the value needs to be read
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_read_long_charac_desc(uint16_t conn_handle, uint16_t attr_handle,
                                          uint16_t val_offset);

/**
 * @brief Start the procedure to write a characteristic descriptor.
 * @note When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the attribute to be written
 * @param value_len Length of the value to be written
 * @param[in] attr_value Value to be written
 * @return Value indicating success or error code.\n
 * 		 It can be @ref BLE_STATUS_NOT_ALLOWED in the following cases:\n
 *		- If the exchange has already taken place\n
 *		- If GATT is expecting response for previous request\n
 *		- Already a request is in the queue to be sent\n
 *		- Channel not open\n
 *		- Already one GATT procedure is started
 */
tBleStatus aci_gatt_write_charac_descriptor(uint16_t conn_handle, uint16_t attr_handle,
					   uint8_t value_len, uint8_t *attr_value);

/**
 * @brief Start the procedure to read the descriptor specified.
 * @note When the procedure is completed, a @ref EVT_BLUE_GATT_PROCEDURE_COMPLETE event is generated.
 * 		 Before procedure completion the response packet is given through @ref EVT_BLUE_ATT_READ_RESP event.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the descriptor to be read
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_read_charac_desc(uint16_t conn_handle, uint16_t attr_handle);

/**
 * @brief Start the procedure to write a characteristic value without waiting for any response from the server.
 * @note No events are generated after this command is executed.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the attribute to be written
 * @param val_len Length of the value to be written (up to ATT_MTU - 3)
 * @param[in] attr_val Value to be written
 * @return Value indicating success or error code.\n
 * 		 It can be @ref BLE_STATUS_NOT_ALLOWED in the following cases:\n
 *		- If the exchange has already taken place\n
 *		- If GATT is expecting response for previous request\n
 *		- Already a request is in the queue to be sent\n
 *		- Channel not open\n
 *		- Already one GATT procedure is started
 */
tBleStatus aci_gatt_write_without_response(uint16_t conn_handle, uint16_t attr_handle,
                                              uint8_t val_len, const uint8_t* attr_val);

/**
 * @brief Start a signed write without response from the server.
 * @note  The procedure i used to write a characteristic value with an authentication signature without waiting
 * 		  for any response from the server. It cannot be used when the link is encrypted.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the attribute to be written
 * @param val_len Length of the value to be written (up to ATT_MTU - 13).
 * @param attr_val Value to be written
 * @return Value indicating success or error code
 */
tBleStatus aci_gatt_signed_write_without_resp(uint16_t conn_handle, uint16_t attr_handle,
                                              uint8_t val_len, uint8_t* attr_val);

/**
 * @brief Confirm an indication
 * @note  This command has to be sent when the application receives the event @ref EVT_BLUE_GATT_INDICATION.
 * @param conn_handle Connection handle for which the command is given.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_confirm_indication(uint16_t conn_handle);

/**
 * @brief Allow or reject a write request from a client.
 * @note This command has to be sent by the application when it receives the @ref EVT_BLUE_GATT_WRITE_PERMIT_REQ.
 *  	 If the write is allowed, then the status and error code has to be set to 0. If the write is not allowed,
 *  	 then the status has to be set to 1 and the error code has to be set to the error code that has to be
 *  	 passed to the client.
 * @param conn_handle Connection handle for which the command is given
 * @param attr_handle Handle of the attribute that was passed in the event @ref EVT_BLUE_GATT_WRITE_PERMIT_REQ.
 * @param write_status 0x00: The value can be written to the attribute specified by attr_handle\n
 * 					   0x01: The value cannot be written to the attribute specified by the attr_handle.
 * @param err_code  The error code that has to be passed to the client in case the write has to be rejected.
 * @param att_val_len Length of the value to be written as passed in the event @ref EVT_BLUE_GATT_WRITE_PERMIT_REQ.
 * @param att_val Value as passed in the event @ref EVT_BLUE_GATT_WRITE_PERMIT_REQ.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_write_response(uint16_t conn_handle,
                                   uint16_t attr_handle,
                                   uint8_t write_status,
                                   uint8_t err_code,
                                   uint8_t att_val_len,
                                   uint8_t *att_val);

/**
 * @brief Allow the GATT server to send a response to a read request from a client.
 * @note The application has to send this command when it receives the @ref EVT_BLUE_GATT_READ_PERMIT_REQ
 * 		 or @ref EVT_BLUE_GATT_READ_MULTI_PERMIT_REQ. This command indicates to the stack that the response
 * 		 can be sent to the client. So if the application wishes to update any of the attributes before
 * 		 they are read by the client, it has to update the characteristic values using the aci_gatt_update_char_value
 * 		 and then give this command. The application should perform the required operations within 30 seconds,
 * 		 otherwise the GATT procedure will go to timeout.
 * @param conn_handle Connection handle for which the command is given.
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_allow_read(uint16_t conn_handle);

/**
 * @brief Set the security permission for the attribute handle specified.
 * @note  Currently the setting of security permission is allowed only for client configuration descriptor.
 * @param service_handle Handle of the service which contains the attribute whose security
 * 						 permission has to be modified.
 * @param attr_handle	Handle of the attribute whose security permission has to be modified.
 * @param security_permission Security permissions for the descriptor. See @ref Security_permissions "Security permissions".
 * 			@arg ATTR_PERMISSION_NONE
 * 			@arg ATTR_PERMISSION_AUTHEN_READ
 * 			@arg ATTR_PERMISSION_AUTHOR_READ
 * 			@arg ATTR_PERMISSION_ENCRY_READ
 * 			@arg ATTR_PERMISSION_AUTHEN_WRITE
 * 			@arg ATTR_PERMISSION_AUTHOR_WRITE
 * 			@arg ATTR_PERMISSION_ENCRY_WRITE
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_set_security_permission(uint16_t service_handle, uint16_t attr_handle,
                                            uint8_t security_permission);

/**
 * @brief This command sets the value of the descriptor specified by charDescHandle.
 * @param servHandle Handle of the service which contains the descriptor.
 * @param charHandle Handle of the characteristic which contains the descriptor.
 * @param charDescHandle Handle of the descriptor whose value has to be set.
 * @param charDescValOffset Offset from which the descriptor value has to be updated.
 * @param charDescValueLen Length of the descriptor value
 * @param[in] charDescValue descriptor value
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_set_desc_value(uint16_t servHandle,
				   uint16_t charHandle,
				   uint16_t charDescHandle,
				   uint16_t charDescValOffset,
				   uint8_t charDescValueLen,
				   const void *charDescValue);

/**
 * @brief Reads the value of the attribute handle specified from the local GATT database.
 * @param attr_handle Handle of the attribute to read
 * @param data_len Length of the data buffer.
 * @param[out] data_len_out_p Length of the read attribute.
 * @param[out] data Pointer to the buffer that will contain the read value.
 * 				The buffer will be filled with the attribute value.
 * 				The length will be the minimum between the provided data_len and the actual length of the
 * 				attribute (in data_len_out_p).
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_read_handle_value(uint16_t attr_handle, uint16_t data_len, uint16_t *data_len_out_p, uint8_t *data);

/**
 * @brief Reads the value of the attribute handle specified from the local GATT database, starting from a given offset.
 * @param attr_handle Handle of the attribute to read
 * @param offset      Offset from which the value needs to be read
 * @param data_len    Length of the data buffer.
 * @param[out] data_len_out_p Length of the read attribute.
 * @param[out] data Pointer to the buffer that will contain the read value.
 * 				The buffer will be filled with the attribute value.
 * 				The length will be the minimum between the provided data_len and the actual length of the
 * 				attribute (in data_len_out_p).
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_read_handle_value_offset_IDB05A1(uint16_t attr_handle, uint8_t offset, uint16_t data_len, uint16_t *data_len_out_p, uint8_t *data);

/**
 * @brief Update the value of a characteristic and sends notifications or indications.
 * @note This command is a more flexible version of ACI_GATT_UPDATE_CHAR_VALUE to support update of long attribute
 *  	 up to 512 bytes and indicate selectively the generation of indications and notifications.
 * @param service_handle Handle of the service to which the characteristic belongs.
 * @param char_handle Handle of the characteristic
 * @param update_type Bitmask that controls generation of notifications and indications. It can be a combination
 * 						@arg @ref NOTIFICATION (0x01): send notification, if enabled.
 * 						@arg @ref INDICATION (0x02): send indication, if enabled.
 * 					  If set to 0 no notifications or indications are sent.
 * @param char_length Total length of the characteristic value. In case of a variable size characteristic,
 * 					  this field specifies the new length of the characteristic value after the update;
 * 					  in case of fixed length characteristic this field is ignored.
 * @param value_offset The offset from which the attribute value has to be updated
 * @param value_length Length of the value to be updated
 * @param[out] value   Updated characteristic value
 * @return Value indicating success or error code.
 */
tBleStatus aci_gatt_update_char_value_ext_IDB05A1(uint16_t service_handle, uint16_t char_handle,
                                                  uint8_t update_type, uint16_t char_length,
                                                  uint16_t value_offset, uint8_t value_length,
                                                  const uint8_t* value);

tBleStatus aci_gatt_set_event_mask(uint32_t event_mask);

/**
 * @}
 */


/**
 * @defgroup GATT_Events GATT events
 * The structures are the data field of @ref evt_blue_aci.
 * @{
 */

/**
 * This event (if enabled, see @ref Gatt_Event_Mask "Gatt Event Mask") is raised to the application
 * by the GATT server when a client modifies any attribute on the server, as consequence of one of
 * the following GATT procedures:
 * @li write without response
 * @li signed write without response
 * @li write characteristic value
 * @li write long characteristic value
 * @li reliable write.
 * See @ref _evt_gatt_attr_modified_IDB04A1 or @ref _evt_gatt_attr_modified__IDB05A1.
 */
#define EVT_BLUE_GATT_ATTRIBUTE_MODIFIED          (0x0C01)

typedef __packed struct _evt_gatt_attr_modified_IDB05A1{
  uint16_t conn_handle; /**< The connection handle which modified the attribute. */
  uint16_t attr_handle; /**< Handle of the attribute that was modified. */
  uint8_t  data_length; /**< The length of the data */
  uint16_t  offset; /**< Offset from which the write has been performed by the peer device */
  uint8_t  att_data[VARIABLE_SIZE]; /**< The new value (length is data_length) */
} PACKED evt_gatt_attr_modified_IDB05A1;

typedef __packed struct _evt_gatt_attr_modified_IDB04A1{
  uint16_t conn_handle; /**< The connection handle which modified the attribute. */
  uint16_t attr_handle; /**< Handle of the attribute that was modified. */
  uint8_t  data_length; /**< The length of the data */
  uint8_t  att_data[VARIABLE_SIZE]; /**< The new value (length is data_length) */
} PACKED evt_gatt_attr_modified_IDB04A1;

/**
 * This event is generated by the client/server to the application on a GATT timeout (30 seconds).
 * See @ref _evt_gatt_procedure_timeout.
 */
#define EVT_BLUE_GATT_PROCEDURE_TIMEOUT           (0x0C02)
typedef __packed struct _evt_gatt_procedure_timeout{
	uint16_t conn_handle; /**< The connection handle on which the GATT procedure has timed out */
} PACKED evt_gatt_procedure_timeout;

/**
 * This event is generated in response to an Exchange MTU request. See aci_gatt_exchange_configuration().
 * See @ref _evt_att_exchange_mtu_resp.
 */
#define EVT_BLUE_ATT_EXCHANGE_MTU_RESP		  (0x0C03)
typedef __packed struct _evt_att_exchange_mtu_resp{
  uint16_t conn_handle; /**< The connection handle related to the response */
  uint8_t  event_data_length; /**< Length of following data (always 1). */
  uint16_t server_rx_mtu; /**< Attribute server receive MTU size */
} PACKED evt_att_exchange_mtu_resp;

/**
 * This event is generated in response to a @a Find @a Information @a Request. See aci_att_find_information_req() and
 * Find Information Response in Bluetooth Core v4.0 spec. See @ref _evt_att_find_information_resp.
 */
#define EVT_BLUE_ATT_FIND_INFORMATION_RESP	  (0x0C04)
typedef __packed struct _evt_att_find_information_resp{
  uint16_t conn_handle;			/**< The connection handle related to the response */
  uint8_t  event_data_length;	/**< Length of following data. */
  uint8_t  format;				/**< The format of the handle_uuid_pair. @arg 1: 16-bit UUIDs @arg 2: 128-bit UUIDs */
  /**
   *  A sequence of handle-uuid pairs.\n
   *  @li if format=1, each pair is:\n
   *  [2 octets for handle, 2 octets for UUIDs] \n
   *  @li if format=2, each pair is:\n
   *  [2 octets for handle, 16 octets for UUIDs]
   */
  uint8_t  handle_uuid_pair[VARIABLE_SIZE];
} PACKED evt_att_find_information_resp;

/**
 * This event is generated in response to a @a Find @a By @a Type @a Value @a Request. See
 * Find By Type Value Response in Bluetooth Core v4.0 spec. See @ref _evt_att_find_by_type_val_resp.
 */
#define EVT_BLUE_ATT_FIND_BY_TYPE_VAL_RESP	  (0x0C05)
typedef __packed struct _evt_att_find_by_type_val_resp{
  uint16_t conn_handle;				/**< The connection handle related to the response */
  uint8_t  event_data_length;		/**< Length of following data. */
  /**
   *  Handles Information List as defined in Bluetooth Core v4.0 spec.
   *  A sequence of handle pairs: [2 octets for Found Attribute Handle, 2 octets for Group End Handle]
   */
  uint8_t  handles_info_list[VARIABLE_SIZE];
} PACKED evt_att_find_by_type_val_resp;

/**
 * This event is generated in response to a @a Read @a By @a Type @a Request. See aci_gatt_find_included_services() and
 * aci_gatt_disc_all_charac_of_serv().
 * For more info see Read By Type Response in Bluetooth Core v4.0 spec. See @ref _evt_att_read_by_type_resp.
 */
#define EVT_BLUE_ATT_READ_BY_TYPE_RESP		  (0x0C06)
typedef __packed struct _evt_att_read_by_type_resp{
  uint16_t conn_handle;				/**< The connection handle related to the response */
  uint8_t  event_data_length;		/**< Length of following data. */
  uint8_t  handle_value_pair_length; /**< The size of each attribute handle-value pair */
  /**
   *  Attribute Data List as defined in Bluetooth Core v4.0 spec.
   *  A sequence of handle-value pairs: [2 octets for Attribute Handle, (handle_value_pair_length - 2 octets) for Attribute Value]
   */
  uint8_t  handle_value_pair[VARIABLE_SIZE];
} PACKED evt_att_read_by_type_resp;

/**
 * This event is generated in response to a @a Read @a Request. See aci_gatt_read_charac_val().
 * For more info see Read Response in Bluetooth Core v4.0 spec. See @ref _evt_att_read_resp.
 */
#define EVT_BLUE_ATT_READ_RESP			  (0x0C07)
typedef __packed struct _evt_att_read_resp{
  uint16_t conn_handle;				/**< The connection handle related to the response. */
  uint8_t  event_data_length;		/**< Length of following data. */
  uint8_t  attribute_value[VARIABLE_SIZE]; /**< The value of the attribute. */
} PACKED evt_att_read_resp;

/**
 * This event is generated in response to a @a Read @a Blob @a Request. See aci_gatt_read_long_charac_val().
 * For more info see Read Blob Response in Bluetooth Core v4.0 spec. See @ref _evt_att_read_blob_resp.
 */
#define EVT_BLUE_ATT_READ_BLOB_RESP		  (0x0C08)
typedef __packed struct _evt_att_read_blob_resp{
  uint16_t conn_handle;				/**< The connection handle related to the response. */
  uint8_t  event_data_length;		/**< Length of following data. */
  uint8_t  part_attribute_value[VARIABLE_SIZE]; /**< Part of the attribute value. */
} PACKED evt_att_read_blob_resp;

/**
 * This event is generated in response to a @a Read @a Multiple @a Request.
 * For more info see Read Multiple Response in Bluetooth Core v4.0 spec. See @ref _evt_att_read_mult_resp.
 */
#define EVT_BLUE_ATT_READ_MULTIPLE_RESP		  (0x0C09)
typedef __packed struct _evt_att_read_mult_resp{
  uint16_t conn_handle;				/**< The connection handle related to the response. */
  uint8_t  event_data_length;		/**< Length of following data. */
  uint8_t  set_of_values[VARIABLE_SIZE]; /**< A set of two or more values.*/
} PACKED evt_att_read_mult_resp;

/**
 * This event is generated in response to a @a Read @a By @a Group @a Type @a Request. See aci_gatt_disc_all_prim_services().
 * For more info see Read By Group type Response in Bluetooth Core v4.0 spec. See @ref _evt_att_read_by_group_resp.
 */
#define EVT_BLUE_ATT_READ_BY_GROUP_TYPE_RESP           (0x0C0A)
typedef __packed struct _evt_att_read_by_group_resp{
  uint16_t conn_handle;				/**< The connection handle related to the response. */
  uint8_t  event_data_length;		/**< Length of following data. */
  uint8_t  attribute_data_length;   /**< The size of each Attribute Data. */
  /**
   *  A list of Attribute Data where the attribute data is composed by:
   *  @li 2 octets for Attribute Handle
   *  @li 2 octets for End Group Handle
   *  @li (attribute_data_length - 4) octets for Attribute Value
   */
  uint8_t  attribute_data_list[VARIABLE_SIZE];
} PACKED evt_att_read_by_group_resp;

/**
 * This event is generated in response to a @a Prepare @a Write @a Request.
 * For more info see Prepare Write Response in Bluetooth Core v4.0 spec. See @ref _evt_att_prepare_write_resp.
 */
#define EVT_BLUE_ATT_PREPARE_WRITE_RESP		  (0x0C0C)
typedef __packed struct _evt_att_prepare_write_resp{
  uint16_t conn_handle;				/**< The connection handle related to the response. */
  uint8_t  event_data_length;		/**< Length of following data. */
  uint16_t  attribute_handle;		/**< The handle of the attribute to be written. */
  uint16_t  offset;					/**< The offset of the first octet to be written. */
  uint8_t  part_attr_value[VARIABLE_SIZE]; /**< The value of the attribute to be written. */
} PACKED evt_att_prepare_write_resp;

/**
 * This event is generated in response to an @a Execute @a Write @a Request.
 * For more info see Execute Write Response in Bluetooth Core v4.0 spec. See @ref _evt_att_exec_write_resp.
 */
#define EVT_BLUE_ATT_EXEC_WRITE_RESP		  (0x0C0D)
typedef __packed struct _evt_att_exec_write_resp{
  uint16_t conn_handle;			/**< The connection handle related to the response. */
  uint8_t  event_data_length; 	/**< Always 0. */
} PACKED evt_att_exec_write_resp;

/**
 * This event is generated when an indication is received from the server.
 * For more info see Handle Value Indication in Bluetooth Core v4.0 spec. See @ref _evt_gatt_indication.
 */
#define EVT_BLUE_GATT_INDICATION		  (0x0C0E)
typedef __packed struct _evt_gatt_indication{
  uint16_t conn_handle;		  			/**< The connection handle related to the event. */
  uint8_t  event_data_length; 			/**< Length of following data. */
  uint16_t attr_handle;					/**< The handle of the attribute. */
  uint8_t  attr_value[VARIABLE_SIZE]; 	/**< The current value of the attribute. */
} PACKED evt_gatt_indication;

/**
 * This event is generated when a notification is received from the server.
 * For more info see Handle Value Notification in Bluetooth Core v4.0 spec. See @ref _evt_gatt_notification.
 */
#define EVT_BLUE_GATT_NOTIFICATION		  (0x0C0F)
typedef __packed struct _evt_gatt_notification{
  uint16_t conn_handle;					/**< The connection handle related to the event. */
  uint8_t  event_data_length; 			/**< Length of following data. */
  uint16_t attr_handle;					/**< The handle of the attribute. */
  uint8_t  attr_value[VARIABLE_SIZE]; 	/**< The current value of the attribute. */
} PACKED evt_gatt_attr_notification;

/**
 * This event is generated when a GATT client procedure completes either with error or successfully.
 * See @ref _evt_gatt_procedure_complete.
 */
#define EVT_BLUE_GATT_PROCEDURE_COMPLETE          (0x0C10)
typedef __packed struct _evt_gatt_procedure_complete{
  uint16_t conn_handle; /**< The connection handle on which the GATT procedure has completed */
  uint8_t  data_length; /**< Length of error_code field (always 1). */
  /**
   * Indicates whether the procedure completed with error (BLE_STATUS_FAILED) or was successful (BLE_STATUS_SUCCESS).
   */
  uint8_t  error_code;
} PACKED evt_gatt_procedure_complete;

/**
 * This event is generated when an Error Response is received from the server. The error response can be given
 * by the server at the end of one of the GATT discovery procedures. This does not mean that the procedure ended
 * with an error, but this error event is part of the procedure itself. See @ref _evt_gatt_error_resp.
 */
#define EVT_BLUE_GATT_ERROR_RESP                  (0x0C11)
typedef __packed struct _evt_gatt_error_resp{
  uint16_t conn_handle;			/**< The connection handle related to the event. */
  uint8_t  event_data_length;	/**< Length of following data. */
  uint8_t  req_opcode;			/**< The request that generated this error response. */
  uint16_t attr_handle;			/**< The attribute handle that generated this error response. */
  uint8_t  error_code;			/**< The reason why the request has generated an error response. See Error Response in Bluetooth Core v4.0 spec.  */
} PACKED evt_gatt_error_resp;

/**
 * This event can be generated during a "Discover Characteristics By UUID" procedure or a "Read using
 * Characteristic UUID" procedure.
 * The attribute value will be a service declaration as defined in Bluetooth Core v4.0 spec (vol.3, Part G, ch. 3.3.1),
 * when a "Discover Characteristics By UUID" has been started. It will be the value of the Characteristic if a
 * "Read using Characteristic UUID" has been performed. See @ref _evt_gatt_disc_read_char_by_uuid_resp.
 */
#define EVT_BLUE_GATT_DISC_READ_CHAR_BY_UUID_RESP (0x0C12)
typedef __packed struct _evt_gatt_disc_read_char_by_uuid_resp{
  uint16_t conn_handle;						/**< The connection handle related to the event. */
  uint8_t  event_data_length;				/**< Length of following data. */
  uint16_t attr_handle;						/**< The handle of the attribute. */
  /**
   * The attribute value will be a service declaration as defined in Bluetooth Core v4.0 spec (vol.3, Part G, ch. 3.3.1),
   * when a "Discover Characteristics By UUID" has been started. It will be the value of the Characteristic if a
   * "Read using Characteristic UUID" has been performed.
   */
  uint8_t  attr_value[VARIABLE_SIZE];
} PACKED evt_gatt_disc_read_char_by_uuid_resp;

/**
 * This event is given to the application when a write request, write command or signed write command
 * is received by the server from the client. This event will be given to the application only if the
 * event bit for this event generation is set when the characteristic was added.
 * When this event is received, the application has to check whether the value being requested for write
 * is allowed to be written and respond with the command aci_gatt_write_response().
 * If the write is rejected by the application, then the value of the attribute will not be modified.
 * In case of a write request, an error response will be sent to the client, with the error code as specified by the application.
 * In case of write/signed write commands, no response is sent to the client but the attribute is not modified.
 * See @ref evt_gatt_write_permit_req.
 */
#define EVT_BLUE_GATT_WRITE_PERMIT_REQ            (0x0C13)
typedef __packed struct _evt_gatt_write_permit_req{
  uint16_t conn_handle; /**< Handle of the connection on which there was the request to write the attribute. */
  uint16_t attr_handle; /**< The handle of the attribute for which the write request has been made by the client */
  uint8_t  data_length; /**< Length of data field. */
  uint8_t  data[VARIABLE_SIZE]; /**< The data that the client has requested to write */
} PACKED evt_gatt_write_permit_req;

/**
 * This event is given to the application when a read request or read blob request is received by the server
 * from the client. This event will be given to the application only if the event bit for this event generation
 * is set when the characteristic was added.
 * On receiving this event, the application can update the value of the handle if it desires and when done
 * it has to use the aci_gatt_allow_read() command to indicate to the stack that it can send the response to the client.
 * See @ref evt_gatt_read_permit_req.
 *
 */
#define EVT_BLUE_GATT_READ_PERMIT_REQ             (0x0C14)
typedef __packed struct _evt_gatt_read_permit_req{
  uint16_t conn_handle; /**< Handle of the connection on which there was the request to read the attribute. */
  uint16_t attr_handle; /**< The handle of the attribute for which the read request has been made by the client */
  uint8_t  data_length; /**< Length of offset field. */
  uint16_t offset;      /**< Contains the offset from which the read has been requested */
} PACKED evt_gatt_read_permit_req;

/**
 * This event is given to the application when a read multiple request or read by type request is received
 * by the server from the client. This event will be given to the application only if the event bit for this
 * event generation is set when the characteristic was added.
 * On receiving this event, the application can update the values of the handles if it desires and when done
 * it has to send the aci_gatt_allow_read command to indicate to the stack that it can send the response to the client.
 * See @ref evt_gatt_read_multi_permit_req.
 *
 */
#define EVT_BLUE_GATT_READ_MULTI_PERMIT_REQ       (0x0C15)
typedef __packed struct _evt_gatt_read_multi_permit_req{
  uint16_t conn_handle; /**< Handle of the connection on which there was the request to read the attribute. */
  uint8_t  data_length; /**< Length of data field. */
  uint8_t  data[VARIABLE_SIZE]; /**< The handles of the attributes that have been requested by the client for a read. */
} PACKED evt_gatt_read_multi_permit_req;

/**
 * This event is raised when the number of available TX buffers is above a threshold TH (TH = 2).
 * The event will be given only if a previous ACI command returned with BLE_STATUS_INSUFFICIENT_RESOURCES.
 * On receiving this event, the application can continue to send notifications by calling aci_gatt_update_char_value().
 * See @ref evt_gatt_tx_pool_vailable.
 *
 */
#define EVT_BLUE_GATT_TX_POOL_AVAILABLE           (0x0C16)
typedef __packed struct _evt_gatt_tx_pool_available{
  uint16_t conn_handle; /**< Handle of the connection on which there was the request to read the attribute. */
  uint16_t available_buffers; /**< Length of data field. */
} PACKED evt_gatt_tx_pool_available;

/**
 * This event is raised on the server when the client confirms the reception of an indication.
 */
#define EVT_BLUE_GATT_SERVER_CONFIRMATION_EVENT    (0x0C17)
typedef __packed struct _evt_gatt_server_confirmation{
  uint16_t conn_handle; /**< Handle of the connection on which there was the request to read the attribute. */
} PACKED evt_gatt_server_confirmation;


/**
 * This event is given to the application when a prepare write request
 * is received by the server from the client. This event will be given to the application only if the
 * event bit for this event generation is set when the characteristic was added.
 * When this event is received, the application has to check whether the value being requested for write
 * is allowed to be written and respond with the command aci_gatt_write_response().
 * Based on the response from the application, the attribute value will be modified by the stack.
 * If the write is rejected by the application, then the value of the attribute will not be modified
 * and an error response will be sent to the client, with the error code as specified by the application.
 * See @ref evt_gatt_write_permit_req.
 */
#define EVT_BLUE_GATT_PREPARE_WRITE_PERMIT_REQ            (0x0C18)
typedef __packed struct _evt_gatt_prepare_write_permit_req{
  uint16_t conn_handle; /**< Handle of the connection on which there was the request to write the attribute. */
  uint16_t attr_handle; /**< The handle of the attribute for which the write request has been made by the client */
  uint16_t offset; /**< The offset from which the prepare write has been requested */
  uint8_t  data_length; /**< Length of data field. */
  uint8_t  data[VARIABLE_SIZE]; /**< The data that the client has requested to write */
} PACKED evt_gatt_prepare_write_permit_req;

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* __BLUENRG_GATT_ACI_H__ */
