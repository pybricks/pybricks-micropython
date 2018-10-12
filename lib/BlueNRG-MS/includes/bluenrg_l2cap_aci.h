/******************** (C) COPYRIGHT 2014 STMicroelectronics ********************
* File Name          : bluenrg_l2cap_aci.h
* Author             : AMS - HEA&RF BU
* Version            : V1.0.0
* Date               : 26-Jun-2014
* Description        : Header file with L2CAP commands for BlueNRG FW6.3.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#ifndef __BLUENRG_L2CAP_ACI_H__
#define __BLUENRG_L2CAP_ACI_H__

/** 
 * @addtogroup HIGH_LEVEL_INTERFACE HIGH_LEVEL_INTERFACE
 * @{
 */
 
/**
 *@addtogroup ACI_L2CAP ACI_L2CAP
 *@brief API for L2CAP layer.
 *@{
 */

/**
 *@defgroup L2CAP_Functions L2CAP functions
 *@brief API for L2CAP layer.
 *@{
 */

/**
 * @brief Send an L2CAP Connection Parameter Update request from the slave to the master.
 * @note  An @ref EVT_BLUE_L2CAP_CONN_UPD_RESP event will be raised when the master will respond to the request
 * 		  (accepts or rejects).
 * @param conn_handle Connection handle on which the connection parameter update request has to be sent.
 * @param interval_min Defines minimum value for the connection event interval in the following manner:
 *						connIntervalMin = interval_min x 1.25ms
 * @param interval_max Defines maximum value for the connection event interval in the following manner:
 * 						connIntervalMax = interval_max x 1.25ms
 * @param slave_latency Defines the slave latency parameter (number of connection events that can be skipped).
 * @param timeout_multiplier Defines connection timeout parameter in the following manner:
 * 							 timeout_multiplier x 10ms.
 * @return Value indicating success or error code.
 */
tBleStatus aci_l2cap_connection_parameter_update_request(uint16_t conn_handle, uint16_t interval_min,
							 uint16_t interval_max, uint16_t slave_latency,
							 uint16_t timeout_multiplier);
/**
 * @brief Accept or reject a connection update.
 * @note  This command should be sent in response to a @ref EVT_BLUE_L2CAP_CONN_UPD_REQ event from the controller.
 * 		  The accept parameter has to be set if the connection parameters given in the event are acceptable.
 * @param conn_handle Handle received in @ref EVT_BLUE_L2CAP_CONN_UPD_REQ event.
 * @param interval_min The connection interval parameter as received in the l2cap connection update request event
 * @param interval_max The maximum connection interval parameter as received in the l2cap connection update request event.
 * @param slave_latency The slave latency parameter as received in the l2cap connection update request event.
 * @param timeout_multiplier The supervision connection timeout parameter as received in the l2cap connection update request event.
 * @param min_ce_length Minimum length of connection event needed for the LE connection.\n
 * 						Range: 0x0000 - 0xFFFF\n
 * 						Time = N x 0.625 msec.
 * @param max_ce_length Maximum length of connection event needed for the LE connection.\n
 * 						Range: 0x0000 - 0xFFFF\n
 * 						Time = N x 0.625 msec.
 * @param id Identifier received in @ref EVT_BLUE_L2CAP_CONN_UPD_REQ event.
 * @param accept @arg 0x00: The connection update parameters are not acceptable.
 *               @arg 0x01: The connection update parameters are acceptable.
 * @return Value indicating success or error code.
 */
tBleStatus aci_l2cap_connection_parameter_update_response_IDB05A1(uint16_t conn_handle, uint16_t interval_min,
							 uint16_t interval_max, uint16_t slave_latency,
							 uint16_t timeout_multiplier, uint16_t min_ce_length, uint16_t max_ce_length,
                                                         uint8_t id, uint8_t accept);

 /**
 * @brief Accept or reject a connection update.
 * @note  This command should be sent in response to a @ref EVT_BLUE_L2CAP_CONN_UPD_REQ event from the controller.
 * 		  The accept parameter has to be set if the connection parameters given in the event are acceptable.
 * @param conn_handle Handle received in @ref EVT_BLUE_L2CAP_CONN_UPD_REQ event.
 * @param interval_min The connection interval parameter as received in the l2cap connection update request event
 * @param interval_max The maximum connection interval parameter as received in the l2cap connection update request event.
 * @param slave_latency The slave latency parameter as received in the l2cap connection update request event.
 * @param timeout_multiplier The supervision connection timeout parameter as received in the l2cap connection update request event.
 * @param id Identifier received in @ref EVT_BLUE_L2CAP_CONN_UPD_REQ event.
 * @param accept @arg 0x00: The connection update parameters are not acceptable.
 *               @arg 0x01: The connection update parameters are acceptable.
 * @return Value indicating success or error code.
 */
tBleStatus aci_l2cap_connection_parameter_update_response_IDB04A1(uint16_t conn_handle, uint16_t interval_min,
							 uint16_t interval_max, uint16_t slave_latency,
							 uint16_t timeout_multiplier, uint8_t id, uint8_t accept);

/**
 * @}
 */
   
/**
 * @defgroup L2CAP_Events L2CAP events
 * @{
 */

/**
 * This event is generated when the master responds to the L2CAP connection update request packet.
 * For more info see CONNECTION PARAMETER UPDATE RESPONSE and COMMAND REJECT in Bluetooth Core v4.0 spec.
 */
#define EVT_BLUE_L2CAP_CONN_UPD_RESP		  (0x0800)
typedef __packed struct _evt_l2cap_conn_upd_resp{
  uint16_t conn_handle;         /**< The connection handle related to the event. */
  uint8_t  event_data_length;  /**< Length of following data. */
/**
 * @li 0x13 in case of valid L2CAP Connection Parameter Update Response packet.
 * @li 0x01 in case of Command Reject.
 */
  uint8_t  code;
  uint8_t  identifier;    /**< Identifier of the response. It is equal to the request. */
  uint16_t l2cap_length;  /**< Length of following data. It should always be 2 */
/**
 * Result code (parameters accepted or rejected) in case of Connection Parameter Update
 * Response (code=0x13) or reason code for rejection in case of Command Reject (code=0x01).
 */
  uint16_t result;
} PACKED evt_l2cap_conn_upd_resp;

/**
 * This event is generated when the master does not respond to the connection update request
 * within 30 seconds.
 */
#define EVT_BLUE_L2CAP_PROCEDURE_TIMEOUT      (0x0801)
typedef __packed struct _evt_l2cap_procedure_timeout{
  uint16_t conn_handle;         /**< The connection handle related to the event. */
  uint8_t  event_data_length;  /**< Length of following data. It should be always 0 for this event. */
} PACKED evt_l2cap_procedure_timeout;

/**
 * The event is given by the L2CAP layer when a connection update request is received from the slave.
 * The application has to respond by calling aci_l2cap_connection_parameter_update_response().
 */
#define EVT_BLUE_L2CAP_CONN_UPD_REQ		  	  (0x0802)
typedef __packed struct _evt_l2cap_conn_upd_req{
/**
 * Handle of the connection for which the connection update request has been received.
 * The same handle has to be returned while responding to the event with the command
 * aci_l2cap_connection_parameter_update_response().
 */
  uint16_t conn_handle; 
  uint8_t  event_data_length; /**< Length of following data. */
/**
 * This is the identifier which associates the request to the
 * response. The same identifier has to be returned by the upper
 * layer in the command aci_l2cap_connection_parameter_update_response().
 */
  uint8_t  identifier; 
  uint16_t l2cap_length;  /**< Length of the L2CAP connection update request. */
  uint16_t interval_min;  /**< Value as defined in Bluetooth 4.0 spec, Volume 3, Part A 4.20. */
  uint16_t interval_max;  /**< Value as defined in Bluetooth 4.0 spec, Volume 3, Part A 4.20. */
  uint16_t slave_latency; /**< Value as defined in Bluetooth 4.0 spec, Volume 3, Part A 4.20. */
  uint16_t timeout_mult;  /**< Value as defined in Bluetooth 4.0 spec, Volume 3, Part A 4.20. */
} PACKED evt_l2cap_conn_upd_req;

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* __BLUENRG_L2CAP_ACI_H__ */
