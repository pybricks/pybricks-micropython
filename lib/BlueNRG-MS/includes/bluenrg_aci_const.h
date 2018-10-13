/******************** (C) COPYRIGHT 2014 STMicroelectronics ********************
* File Name          : bluenrg_aci_const.h
* Author             : AMS - AAS
* Version            : V1.0.0
* Date               : 26-Jun-2014
* Description        : Header file with ACI definitions for BlueNRG
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#ifndef __BLUENRG_ACI_CONST_H_
#define __BLUENRG_ACI_CONST_H_

#include "link_layer.h"
#include "hci_const.h"
#include "bluenrg_gatt_server.h"
#include "bluenrg_conf.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define OCF_HAL_GET_FW_BUILD_NUMBER         0x0000
typedef struct _hal_get_fw_build_number_rp{
  uint8_t       status;
  uint16_t      build_number;
} hal_get_fw_build_number_rp;
#define HAL_GET_FW_BUILD_NUMBER_RP_SIZE 3
#define OCF_HAL_WRITE_CONFIG_DATA   0x000C
#define OCF_HAL_READ_CONFIG_DATA            0x000D
typedef struct _hal_read_config_data_cp{
  uint8_t     offset;
} hal_read_config_data_cp;
#define HAL_READ_CONFIG_DATA_RP_SIZE 1
typedef struct _hal_read_config_data_rp{
  uint8_t		status;
  uint8_t		data[HCI_MAX_PAYLOAD_SIZE-HAL_READ_CONFIG_DATA_RP_SIZE];
} hal_read_config_data_rp;

#define OCF_HAL_SET_TX_POWER_LEVEL          0x000F
typedef struct _hal_set_tx_power_level_cp{
	uint8_t	en_high_power;
    uint8_t pa_level;
} hal_set_tx_power_level_cp;
#define HAL_SET_TX_POWER_LEVEL_CP_SIZE 2

#define OCF_HAL_DEVICE_STANDBY          0x0013
#define OCF_HAL_LE_TX_TEST_PACKET_NUMBER    0x0014
typedef struct _hal_le_tx_test_packet_number_rp{
  uint8_t status;
  uint32_t number_of_packets;
} hal_le_tx_test_packet_number_rp;

#define OCF_HAL_TONE_START                  0x0015
typedef struct _hal_tone_start_cp{
	uint8_t	rf_channel;
} hal_tone_start_cp;
#define HAL_TONE_START_CP_SIZE 1

#define OCF_HAL_TONE_STOP                   0x0016
#define OCF_HAL_GET_LINK_STATUS             0x0017
typedef struct _hal_get_link_status_rp{
  uint8_t       status;
  uint8_t       link_status[8];
  uint16_t      conn_handle[8];
} hal_get_link_status_rp;

#define OCF_HAL_GET_ANCHOR_PERIOD           0x0019
typedef struct _hal_get_anchor_period_rp{
  uint8_t  status;
  uint32_t anchor_period;
  uint32_t max_free_slot;
} hal_get_anchor_period_rp;

#define OCF_UPDATER_START                   0x0020
#define OCF_UPDATER_REBOOT                  0x0021

#define OCF_GET_UPDATER_VERSION                 0x0022
typedef struct _get_updater_version_rp{
    uint8_t		    status;
	uint8_t		    version;
} get_updater_version_rp;
#define GET_UPDATER_VERSION_RP_SIZE 2

#define OCF_GET_UPDATER_BUFSIZE             0x0023
typedef struct _get_updater_bufsize_rp{
    uint8_t		    status;
	uint8_t		    buffer_size;
} get_updater_bufsize_rp;
#define GET_UPDATER_BUFSIZE_RP_SIZE 2

#define OCF_UPDATER_ERASE_BLUE_FLAG         0x0024

#define OCF_UPDATER_RESET_BLUE_FLAG         0x0025

#define OCF_UPDATER_ERASE_SECTOR            0x0026
typedef struct _updater_erase_sector_cp{
	uint32_t	address;
} updater_erase_sector_cp;
#define UPDATER_ERASE_SECTOR_CP_SIZE 4

#define OCF_UPDATER_PROG_DATA_BLOCK         0x0027
#define UPDATER_PROG_DATA_BLOCK_CP_SIZE     6
typedef struct _updater_prog_data_block_cp{
	uint32_t	address;
    uint16_t    data_len;
    uint8_t		data[HCI_MAX_PAYLOAD_SIZE-UPDATER_PROG_DATA_BLOCK_CP_SIZE];
} updater_prog_data_block_cp;

#define OCF_UPDATER_READ_DATA_BLOCK         0x0028
typedef struct _updater_read_data_block_cp{
	uint32_t	address;
    uint16_t    data_len;
} updater_read_data_block_cp;
#define UPDATER_READ_DATA_BLOCK_CP_SIZE 6
typedef struct _updater_read_data_block_rp{
    uint8_t		    status;
	uint8_t		    data[VARIABLE_SIZE];
} updater_read_data_block_rp;
#define GET_UPDATER_BUFSIZE_RP_SIZE 2

#define OCF_UPDATER_CALC_CRC                0x0029
typedef struct _updater_calc_crc_cp{
	uint32_t	address;
    uint8_t    num_sectors;
} updater_calc_crc_cp;
#define UPDATER_CALC_CRC_CP_SIZE 5
typedef struct _updater_calc_crc_rp{
    uint8_t		    status;
	uint32_t		crc;
} updater_calc_crc_rp;
#define UPDATER_CALC_CRC_RP_SIZE 5

#define OCF_UPDATER_HW_VERSION              0x002A
typedef struct _updater_hw_version_rp{
    uint8_t		    status;
	uint8_t		    version;
} updater_hw_version_rp;
#define UPDATER_HW_VERSION_RP_SIZE 2

#define OCF_GAP_SET_NON_DISCOVERABLE	    0x0081

#define OCF_GAP_SET_LIMITED_DISCOVERABLE	0x0082

#define OCF_GAP_SET_DISCOVERABLE	        0x0083

#define OCF_GAP_SET_DIRECT_CONNECTABLE      0x0084
typedef struct _gap_set_direct_conectable_cp_IDB05A1{
    uint8_t		own_bdaddr_type;
    uint8_t		directed_adv_type;
    uint8_t		direct_bdaddr_type;
    tBDAddr		direct_bdaddr;
    uint16_t            adv_interv_min;
    uint16_t            adv_interv_max;
} gap_set_direct_conectable_cp_IDB05A1;

typedef struct _gap_set_direct_conectable_cp_IDB04A1{
    uint8_t		own_bdaddr_type;
    uint8_t		direct_bdaddr_type;
    tBDAddr		direct_bdaddr;
} gap_set_direct_conectable_cp_IDB04A1;

#define OCF_GAP_SET_IO_CAPABILITY      0x0085
typedef struct _gap_set_io_capability_cp{
    uint8_t		io_capability;
} gap_set_io_capability_cp;
#define GAP_SET_IO_CAPABILITY_CP_SIZE 1

#define OCF_GAP_SET_AUTH_REQUIREMENT      0x0086
typedef struct _gap_set_auth_requirement_cp{
    uint8_t	mitm_mode;
    uint8_t     oob_enable;
    uint8_t     oob_data[16];
    uint8_t     min_encryption_key_size;
    uint8_t     max_encryption_key_size;
    uint8_t     use_fixed_pin;
    uint32_t    fixed_pin;
    uint8_t     bonding_mode;
} gap_set_auth_requirement_cp;
#define GAP_SET_AUTH_REQUIREMENT_CP_SIZE 26

#define OCF_GAP_SET_AUTHOR_REQUIREMENT      0x0087
typedef struct _gap_set_author_requirement_cp{
  uint16_t      conn_handle;
  uint8_t       authorization_enable;
} gap_set_author_requirement_cp;
#define GAP_SET_AUTHOR_REQUIREMENT_CP_SIZE 3

#define OCF_GAP_PASSKEY_RESPONSE      0x0088
typedef struct _gap_passkey_response_cp{
  uint16_t conn_handle;
  uint32_t passkey;
} gap_passkey_response_cp;
#define GAP_PASSKEY_RESPONSE_CP_SIZE 6

#define OCF_GAP_AUTHORIZATION_RESPONSE      0x0089
typedef struct _gap_authorization_response_cp{
  uint16_t conn_handle;
  uint8_t  authorize;
} gap_authorization_response_cp;
#define GAP_AUTHORIZATION_RESPONSE_CP_SIZE 3

#define OCF_GAP_INIT		        0x008A

typedef struct _gap_init_cp_IDB05A1{
    uint8_t	role;
    uint8_t	privacy_enabled;
    uint8_t device_name_char_len;
} gap_init_cp_IDB05A1;
#define GAP_INIT_CP_SIZE_IDB05A1 3

typedef struct _gap_init_cp_IDB04A1{
	uint8_t	role;
} gap_init_cp_IDB04A1;
#define GAP_INIT_CP_SIZE_IDB04A1 1

typedef struct _gap_init_rp{
    uint8_t		    status;
	uint16_t		service_handle;
    uint16_t		dev_name_char_handle;
    uint16_t		appearance_char_handle;
} gap_init_rp;
#define GAP_INIT_RP_SIZE 7

#define OCF_GAP_SET_NON_CONNECTABLE      0x008B
typedef struct _gap_set_non_connectable_cp_IDB05A1{
    uint8_t	advertising_event_type;
    uint8_t	own_address_type;
#endif
} gap_set_non_connectable_cp_IDB05A1;

typedef struct _gap_set_non_connectable_cp_IDB04A1{
    uint8_t	advertising_event_type;
} gap_set_non_connectable_cp_IDB04A1;

#define OCF_GAP_SET_UNDIRECTED_CONNECTABLE      0x008C
typedef struct _gap_set_undirected_connectable_cp{
    uint8_t	adv_filter_policy;
    uint8_t	own_addr_type;
} gap_set_undirected_connectable_cp;
#define GAP_SET_UNDIRECTED_CONNECTABLE_CP_SIZE 2

#define OCF_GAP_SLAVE_SECURITY_REQUEST      0x008D
typedef struct _gap_slave_security_request_cp{
  uint16_t conn_handle;
  uint8_t  bonding;
  uint8_t  mitm_protection;
} gap_slave_security_request_cp;
#define GAP_SLAVE_SECURITY_REQUEST_CP_SIZE 4

#define OCF_GAP_UPDATE_ADV_DATA      0x008E

#define OCF_GAP_DELETE_AD_TYPE      0x008F
typedef struct _gap_delete_ad_type_cp{
    uint8_t	ad_type;
} gap_delete_ad_type_cp;
#define GAP_DELETE_AD_TYPE_CP_SIZE 1

#define OCF_GAP_GET_SECURITY_LEVEL      0x0090
typedef struct _gap_get_security_level_rp{
    uint8_t		    status;
	uint8_t		    mitm_protection;
    uint8_t		    bonding;
    uint8_t		    oob_data;
    uint8_t         passkey_required;
} gap_get_security_level_rp;
#define GAP_GET_SECURITY_LEVEL_RP_SIZE 5

#define OCF_GAP_SET_EVT_MASK      0x0091
typedef struct _gap_set_evt_mask_cp{
    uint16_t	evt_mask;
} gap_set_evt_mask_cp;
#define GAP_SET_EVT_MASK_CP_SIZE 2

#define OCF_GAP_CONFIGURE_WHITELIST   0x0092

#define OCF_GAP_TERMINATE      0x0093
typedef struct _gap_terminate_cp{
  uint16_t handle;
  uint8_t  reason;
} gap_terminate_cp;
#define GAP_TERMINATE_CP_SIZE 3

#define OCF_GAP_CLEAR_SECURITY_DB   0x0094

#define OCF_GAP_ALLOW_REBOND_DB     0x0095

typedef struct _gap_allow_rebond_cp_IDB05A1{
  uint16_t conn_handle;
} gap_allow_rebond_cp_IDB05A1;

#define OCF_GAP_START_LIMITED_DISCOVERY_PROC   0x0096
typedef struct _gap_start_limited_discovery_proc_cp{
  uint16_t scanInterval;
  uint16_t scanWindow;
  uint8_t  own_address_type;
  uint8_t  filterDuplicates;
} gap_start_limited_discovery_proc_cp;
#define GAP_START_LIMITED_DISCOVERY_PROC_CP_SIZE 6

#define OCF_GAP_START_GENERAL_DISCOVERY_PROC   0x0097
typedef struct _gap_start_general_discovery_proc_cp{
  uint16_t scanInterval;
  uint16_t scanWindow;
  uint8_t  own_address_type;
  uint8_t  filterDuplicates;
} gap_start_general_discovery_proc_cp;
#define GAP_START_GENERAL_DISCOVERY_PROC_CP_SIZE 6

#define OCF_GAP_START_NAME_DISCOVERY_PROC   0x0098
typedef struct _gap_start_name_discovery_proc_cp{
  uint16_t scanInterval;
  uint16_t scanWindow;
  uint8_t peer_bdaddr_type;
  tBDAddr peer_bdaddr;
  uint8_t own_bdaddr_type;
  uint16_t conn_min_interval;
  uint16_t conn_max_interval;
  uint16_t conn_latency;
  uint16_t supervision_timeout;
  uint16_t min_conn_length;
  uint16_t max_conn_length;
} gap_start_name_discovery_proc_cp;
#define GAP_START_NAME_DISCOVERY_PROC_CP_SIZE 24

#define OCF_GAP_START_AUTO_CONN_ESTABLISH_PROC  0x0099

#define OCF_GAP_START_GENERAL_CONN_ESTABLISH_PROC  0x009A

typedef struct _gap_start_general_conn_establish_proc_cp_IDB05A1{
  uint8_t  scan_type;
  uint16_t scan_interval;
  uint16_t scan_window;
  uint8_t  own_address_type;
  uint8_t  filter_duplicates;
} gap_start_general_conn_establish_proc_cp_IDB05A1;

typedef struct _gap_start_general_conn_establish_proc_cp_IDB04A1{
  uint8_t  scan_type;
  uint16_t scan_interval;
  uint16_t scan_window;
  uint8_t  own_address_type;
  uint8_t  filter_duplicates;
  uint8_t  use_reconn_addr;
  tBDAddr  reconn_addr;
} gap_start_general_conn_establish_proc_cp_IDB04A1;

#define OCF_GAP_START_SELECTIVE_CONN_ESTABLISH_PROC  0x009B
#define GAP_START_SELECTIVE_CONN_ESTABLISH_PROC_CP_SIZE 8
typedef struct _gap_start_selective_conn_establish_proc_cp{
  uint8_t scan_type;
  uint16_t scan_interval;
  uint16_t scan_window;
  uint8_t own_address_type;
  uint8_t filter_duplicates;
  uint8_t num_whitelist_entries;
  uint8_t addr_array[HCI_MAX_PAYLOAD_SIZE-GAP_START_SELECTIVE_CONN_ESTABLISH_PROC_CP_SIZE];
} gap_start_selective_conn_establish_proc_cp;

#define OCF_GAP_CREATE_CONNECTION      0x009C
typedef struct _gap_create_connection_cp{
  uint16_t scanInterval;
  uint16_t scanWindow;
  uint8_t peer_bdaddr_type;
  tBDAddr peer_bdaddr;
  uint8_t own_bdaddr_type;
  uint16_t conn_min_interval;
  uint16_t conn_max_interval;
  uint16_t conn_latency;
  uint16_t supervision_timeout;
  uint16_t min_conn_length;
  uint16_t max_conn_length;
} gap_create_connection_cp;
#define GAP_CREATE_CONNECTION_CP_SIZE 24

#define OCF_GAP_TERMINATE_GAP_PROCEDURE      0x009D

#define OCF_GAP_START_CONNECTION_UPDATE      0x009E
typedef struct _gap_start_connection_update_cp{
  uint16_t conn_handle;
  uint16_t conn_min_interval;
  uint16_t conn_max_interval;
  uint16_t conn_latency;
  uint16_t supervision_timeout;
  uint16_t min_conn_length;
  uint16_t max_conn_length;
} gap_start_connection_update_cp;
#define GAP_START_CONNECTION_UPDATE_CP_SIZE 14

#define OCF_GAP_SEND_PAIRING_REQUEST      0x009F
typedef struct _gap_send_pairing_request_cp{
  uint16_t conn_handle;
  uint8_t  force_rebond;
} gap_send_pairing_request_cp;
#define GAP_GAP_SEND_PAIRING_REQUEST_CP_SIZE 3

#define OCF_GAP_RESOLVE_PRIVATE_ADDRESS   0x00A0
typedef struct _gap_resolve_private_address_cp{
  tBDAddr address;
} gap_resolve_private_address_cp;
#define GAP_RESOLVE_PRIVATE_ADDRESS_CP_SIZE 6

typedef struct _gap_resolve_private_address_rp{
  uint8_t status;
  tBDAddr address;
} gap_resolve_private_address_rp;

#define OCF_GAP_SET_BROADCAST_MODE   0x00A1
#define GAP_SET_BROADCAST_MODE_CP_SIZE 6
typedef struct _gap_set_broadcast_mode_cp{
  uint16_t adv_interv_min;
  uint16_t adv_interv_max;
  uint8_t adv_type;
  uint8_t own_addr_type;
  uint8_t var_len_data[HCI_MAX_PAYLOAD_SIZE-GAP_SET_BROADCAST_MODE_CP_SIZE];
} gap_set_broadcast_mode_cp;

#define OCF_GAP_START_OBSERVATION_PROC   0x00A2
typedef struct _gap_start_observation_proc_cp{
  uint16_t scan_interval;
  uint16_t scan_window;
  uint8_t  scan_type;
  uint8_t  own_address_type;
  uint8_t  filter_duplicates;
} gap_start_observation_proc_cp;

#define OCF_GAP_GET_BONDED_DEVICES   0x00A3
typedef struct _gap_get_bonded_devices_rp{
    uint8_t		status;
    uint8_t		num_addr;
	uint8_t		dev_list[HCI_MAX_PAYLOAD_SIZE-HCI_EVENT_HDR_SIZE-EVT_CMD_COMPLETE_SIZE-1];
} gap_get_bonded_devices_rp;

#define OCF_GAP_IS_DEVICE_BONDED   0x00A4
typedef struct _gap_is_device_bonded_cp{
  uint8_t peer_address_type;
  tBDAddr peer_address;
} gap_is_device_bonded_cp;


#define OCF_GATT_INIT		        0x0101

#define OCF_GATT_ADD_SERV		    0x0102
typedef struct _gatt_add_serv_rp{
    uint8_t		    status;
	uint16_t		handle;
} gatt_add_serv_rp;
#define GATT_ADD_SERV_RP_SIZE 3

#define OCF_GATT_INCLUDE_SERV		0x0103
typedef struct _gatt_include_serv_rp{
	uint8_t		    status;
    uint16_t		handle;
} gatt_include_serv_rp;
#define GATT_INCLUDE_SERV_RP_SIZE 3

#define OCF_GATT_ADD_CHAR		    0x0104
typedef struct _gatt_add_char_rp{
    uint8_t		    status;
	uint16_t		handle;
} gatt_add_char_rp;
#define GATT_ADD_CHAR_RP_SIZE 3

#define OCF_GATT_ADD_CHAR_DESC	    0x0105
typedef struct _gatt_add_char_desc_rp{
    uint8_t		    status;
	uint16_t		handle;
} gatt_add_char_desc_rp;
#define GATT_ADD_CHAR_DESC_RP_SIZE 3

#define OCF_GATT_UPD_CHAR_VAL		0x0106

#define OCF_GATT_DEL_CHAR   		0x0107
typedef struct _gatt_del_char_cp{
	uint16_t	service_handle;
	uint16_t	char_handle;
} gatt_del_char_cp;
#define GATT_DEL_CHAR_CP_SIZE 4

#define OCF_GATT_DEL_SERV   		0x0108
typedef struct _gatt_del_serv_cp{
	uint16_t	service_handle;
} gatt_del_serv_cp;
#define GATT_DEL_SERV_CP_SIZE 2

#define OCF_GATT_DEL_INC_SERV   	0x0109
typedef struct _gatt_del_inc_serv_cp{
	uint16_t	service_handle;
    uint16_t	inc_serv_handle;
} gatt_del_inc_serv_cp;
#define GATT_DEL_INC_SERV_CP_SIZE 4

#define OCF_GATT_SET_EVT_MASK      0x010A
typedef struct _gatt_set_evt_mask_cp{
    uint32_t	evt_mask;
} gatt_set_evt_mask_cp;
#define GATT_SET_EVT_MASK_CP_SIZE 4

#define OCF_GATT_EXCHANGE_CONFIG      0x010B
typedef struct _gatt_exchange_config_cp{
    uint16_t	conn_handle;
} gatt_exchange_config_cp;
#define GATT_EXCHANGE_CONFIG_CP_SIZE 2

#define OCF_ATT_FIND_INFO_REQ      0x010C
typedef struct _att_find_info_req_cp{
    uint16_t	conn_handle;
    uint16_t	start_handle;
    uint16_t	end_handle;
} att_find_info_req_cp;
#define ATT_FIND_INFO_REQ_CP_SIZE 6

#define OCF_ATT_FIND_BY_TYPE_VALUE_REQ  0x010D
#define ATT_FIND_BY_TYPE_VALUE_REQ_CP_SIZE 9
typedef struct _att_find_by_type_value_req_cp{
    uint16_t	conn_handle;
    uint16_t	start_handle;
    uint16_t	end_handle;
    uint8_t     uuid[2];
    uint8_t     attr_val_len;
    uint8_t     attr_val[ATT_MTU - 7];
} att_find_by_type_value_req_cp;

#define OCF_ATT_READ_BY_TYPE_REQ  0x010E
#define ATT_READ_BY_TYPE_REQ_CP_SIZE 7  // without UUID
typedef struct _att_read_by_type_req_cp{
    uint16_t	conn_handle;
    uint16_t	start_handle;
    uint16_t	end_handle;
    uint8_t     uuid_type;
    uint8_t     uuid[16];
} att_read_by_type_req_cp;

#define OCF_ATT_READ_BY_GROUP_TYPE_REQ  0x010F
#define ATT_READ_BY_GROUP_TYPE_REQ_CP_SIZE 7  // without UUID
typedef struct _att_read_by_group_type_req_cp{
    uint16_t	conn_handle;
    uint16_t	start_handle;
    uint16_t	end_handle;
    uint8_t     uuid_type;
    uint8_t     uuid[16];
} att_read_by_group_type_req_cp;

#define OCF_ATT_PREPARE_WRITE_REQ  0x0110
#define ATT_PREPARE_WRITE_REQ_CP_SIZE 7  // without attr_val
typedef struct _att_prepare_write_req_cp{
    uint16_t	conn_handle;
    uint16_t	attr_handle;
    uint16_t	value_offset;
    uint8_t     attr_val_len;
    uint8_t     attr_val[ATT_MTU-5];
} att_prepare_write_req_cp;

#define OCF_ATT_EXECUTE_WRITE_REQ  0x0111
typedef struct _att_execute_write_req_cp{
    uint16_t	conn_handle;
    uint8_t     execute;
} att_execute_write_req_cp;
#define ATT_EXECUTE_WRITE_REQ_CP_SIZE 3

#define OCF_GATT_DISC_ALL_PRIM_SERVICES 0X0112
typedef struct _gatt_disc_all_prim_serivces_cp{
  uint16_t conn_handle;
} gatt_disc_all_prim_services_cp;
#define GATT_DISC_ALL_PRIM_SERVICES_CP_SIZE 2

#define OCF_GATT_DISC_PRIM_SERVICE_BY_UUID 0x0113
typedef struct _gatt_disc_prim_service_by_uuid_cp{
  uint16_t    conn_handle;
  uint8_t     uuid_type;
  uint8_t     uuid[16];
} gatt_disc_prim_service_by_uuid_cp;
#define GATT_DISC_PRIM_SERVICE_BY_UUID_CP_SIZE 3 // Without uuid

#define OCF_GATT_FIND_INCLUDED_SERVICES 0X0114
typedef struct _gatt_disc_find_included_services_cp{
  uint16_t conn_handle;
  uint16_t start_handle;
  uint16_t end_handle;
} gatt_find_included_services_cp;
#define GATT_FIND_INCLUDED_SERVICES_CP_SIZE 6

#define OCF_GATT_DISC_ALL_CHARAC_OF_SERV 0X0115
typedef struct _gatt_disc_all_charac_of_serv_cp{
  uint16_t conn_handle;
  uint16_t start_attr_handle;
  uint16_t end_attr_handle;
} gatt_disc_all_charac_of_serv_cp;
#define GATT_DISC_ALL_CHARAC_OF_SERV_CP_SIZE 6

#define OCF_GATT_DISC_CHARAC_BY_UUID 0X0116

#define OCF_GATT_DISC_ALL_CHARAC_DESCRIPTORS 0X0117
typedef struct _gatt_disc_all_charac_descriptors_cp{
  uint16_t conn_handle;
  uint16_t char_val_handle;
  uint16_t char_end_handle;
} gatt_disc_all_charac_descriptors_cp;
#define GATT_DISC_ALL_CHARAC_DESCRIPTORS_CP_SIZE 6

#define OCF_GATT_READ_CHARAC_VAL   0x0118
typedef struct _gatt_read_charac_val_cp{
  uint16_t conn_handle;
  uint16_t attr_handle;
} gatt_read_charac_val_cp;
#define GATT_READ_CHARAC_VAL_CP_SIZE 4

#define OCF_GATT_READ_USING_CHARAC_UUID  0x0109
typedef struct _gatt_read_using_charac_uuid_cp{
    uint16_t	conn_handle;
    uint16_t	start_handle;
    uint16_t	end_handle;
    uint8_t     uuid_type;
    uint8_t     uuid[16];
} gatt_read_using_charac_uuid_cp;
#define GATT_READ_USING_CHARAC_UUID_CP_SIZE 7  // without UUID

#define OCF_GATT_READ_LONG_CHARAC_VAL   0x011A
typedef struct _gatt_read_long_charac_val_cp{
  uint16_t conn_handle;
  uint16_t attr_handle;
  uint16_t val_offset;
} gatt_read_long_charac_val_cp;
#define GATT_READ_LONG_CHARAC_VAL_CP_SIZE 6

#define OCF_GATT_READ_MULTIPLE_CHARAC_VAL   0x011B
#define GATT_READ_MULTIPLE_CHARAC_VAL_CP_SIZE 3  // without set_of_handles
typedef struct _gatt_read_multiple_charac_val_cp{
  uint16_t conn_handle;
  uint8_t num_handles;
  uint8_t  set_of_handles[HCI_MAX_PAYLOAD_SIZE-GATT_READ_MULTIPLE_CHARAC_VAL_CP_SIZE];
} gatt_read_multiple_charac_val_cp;

#define OCF_GATT_WRITE_CHAR_VALUE   0x011C

#define OCF_GATT_WRITE_LONG_CHARAC_VAL   0x011D
#define GATT_WRITE_LONG_CHARAC_VAL_CP_SIZE 7  // without set_of_handles
typedef struct _gatt_write_long_charac_val_cp{
  uint16_t conn_handle;
  uint16_t attr_handle;
  uint16_t val_offset;
  uint8_t  val_len;
  uint8_t  attr_val[HCI_MAX_PAYLOAD_SIZE-GATT_WRITE_LONG_CHARAC_VAL_CP_SIZE];
} gatt_write_long_charac_val_cp;

#define OCF_GATT_WRITE_CHARAC_RELIABLE   0x011E
#define GATT_WRITE_CHARAC_RELIABLE_CP_SIZE 7  // without set_of_handles
typedef struct _gatt_write_charac_reliable_cp{
  uint16_t conn_handle;
  uint16_t attr_handle;
  uint16_t val_offset;
  uint8_t  val_len;
  uint8_t  attr_val[HCI_MAX_PAYLOAD_SIZE-GATT_WRITE_CHARAC_RELIABLE_CP_SIZE];
} gatt_write_charac_reliable_cp;

#define OCF_GATT_WRITE_LONG_CHARAC_DESC   0x011F
#define GATT_WRITE_LONG_CHARAC_DESC_CP_SIZE 7  // without set_of_handles
typedef struct _gatt_write_long_charac_desc_cp{
  uint16_t conn_handle;
  uint16_t attr_handle;
  uint16_t val_offset;
  uint8_t  val_len;
  uint8_t  attr_val[HCI_MAX_PAYLOAD_SIZE-GATT_WRITE_LONG_CHARAC_DESC_CP_SIZE];
} gatt_write_long_charac_desc_cp;

#define OCF_GATT_READ_LONG_CHARAC_DESC   0x0120
typedef struct _gatt_read_long_charac_desc_cp{
  uint16_t conn_handle;
  uint16_t attr_handle;
  uint16_t val_offset;
} gatt_read_long_charac_desc_cp;
#define GATT_READ_LONG_CHARAC_DESC_CP_SIZE 6

#define OCF_GATT_WRITE_CHAR_DESCRIPTOR      0x0121

#define OCF_GATT_READ_CHAR_DESCRIPTOR       0x0122
typedef struct _gatt_read_charac_desc_cp{
  uint16_t conn_handle;
  uint16_t attr_handle;
} gatt_read_charac_desc_cp;
#define GATT_READ_CHAR_DESCRIPTOR_CP_SIZE 4

#define OCF_GATT_WRITE_WITHOUT_RESPONSE     0x0123
#define GATT_WRITE_WITHOUT_RESPONSE_CP_SIZE 5  // without attr_val
typedef struct _gatt_write_without_resp_cp{
  uint16_t conn_handle;
  uint16_t attr_handle;
  uint8_t  val_len;
  uint8_t  attr_val[ATT_MTU - 3];
} gatt_write_without_resp_cp;

#define OCF_GATT_SIGNED_WRITE_WITHOUT_RESPONSE     0x0124
#define GATT_SIGNED_WRITE_WITHOUT_RESPONSE_CP_SIZE 5  // without attr_val
typedef struct _gatt_signed_write_without_resp_cp{
  uint16_t conn_handle;
  uint16_t attr_handle;
  uint8_t  val_len;
  uint8_t  attr_val[ATT_MTU - 13];
} gatt_signed_write_without_resp_cp;

#define OCF_GATT_CONFIRM_INDICATION                0x0125
typedef struct _gatt_confirm_indication_cp{
	uint16_t	conn_handle;
} gatt_confirm_indication_cp;
#define GATT_CONFIRM_INDICATION_CP_SIZE 2

#define OCF_GATT_WRITE_RESPONSE                    0x0126

#define OCF_GATT_ALLOW_READ		    0x0127
typedef struct _gatt_allow_read_cp{
	uint16_t	conn_handle;
} gatt_allow_read_cp;
#define GATT_ALLOW_READ_CP_SIZE 2

#define OCF_GATT_SET_SECURITY_PERMISSION		    0x0128
typedef struct _gatt_set_security_permission_cp{
	uint16_t	service_handle;
    uint16_t	attr_handle;
    uint8_t	    security_permission;
} gatt_set_security_permission_cp;
#define GATT_GATT_SET_SECURITY_PERMISSION_CP_SIZE 5

#define OCF_GATT_SET_DESC_VAL		0x0129

#define OCF_GATT_READ_HANDLE_VALUE      0x012A
typedef struct _gatt_read_handle_val_cp{
	uint16_t	attr_handle;
} gatt_read_handle_val_cp;
#define GATT_READ_HANDLE_VALUE_RP_SIZE 3
typedef struct _gatt_read_handle_val_rp{
    uint8_t		status;
    uint16_t	value_len;
	uint8_t		value[HCI_MAX_PAYLOAD_SIZE-GATT_READ_HANDLE_VALUE_RP_SIZE];
} gatt_read_handle_val_rp;

#define OCF_GATT_READ_HANDLE_VALUE_OFFSET      0x012B
typedef struct _gatt_read_handle_val_offset_cp{
	uint16_t	attr_handle;
    uint8_t     offset;
} gatt_read_handle_val_offset_cp;
#define GATT_READ_HANDLE_VALUE_OFFSET_RP_SIZE 2
typedef struct _gatt_read_handle_val_offset_rp{
    uint8_t		status;
    uint8_t		value_len;
	uint8_t		value[HCI_MAX_PAYLOAD_SIZE-GATT_READ_HANDLE_VALUE_OFFSET_RP_SIZE];
} gatt_read_handle_val_offset_rp;

#define OCF_GATT_UPD_CHAR_VAL_EXT		        0x012C
#define GATT_UPD_CHAR_VAL_EXT_CP_SIZE 10  // without value
typedef struct _gatt_upd_char_val_ext_cp{
  uint16_t service_handle;
  uint16_t char_handle;
  uint8_t  update_type;
  uint16_t char_length;
  uint16_t value_offset;
  uint8_t  value_length;
  uint8_t  value[HCI_MAX_PAYLOAD_SIZE-GATT_UPD_CHAR_VAL_EXT_CP_SIZE];
} gatt_upd_char_val_ext_cp;

#define OCF_L2CAP_CONN_PARAM_UPDATE_REQ  0x0181
typedef struct _l2cap_conn_param_update_req_cp{
  uint16_t conn_handle;
  uint16_t interval_min;
  uint16_t interval_max;
  uint16_t slave_latency;
  uint16_t timeout_multiplier;
} l2cap_conn_param_update_req_cp;
#define L2CAP_CONN_PARAM_UPDATE_REQ_CP_SIZE 10

#define OCF_L2CAP_CONN_PARAM_UPDATE_RESP  0x0182

typedef struct _l2cap_conn_param_update_resp_cp_IDB05A1{
  uint16_t conn_handle;
  uint16_t interval_min;
  uint16_t interval_max;
  uint16_t slave_latency;
  uint16_t timeout_multiplier;
  uint16_t min_ce_length;
  uint16_t max_ce_length;
  uint8_t id;
  uint8_t accept;
} l2cap_conn_param_update_resp_cp_IDB05A1;

typedef struct _l2cap_conn_param_update_resp_cp_IDB04A1{
  uint16_t conn_handle;
  uint16_t interval_min;
  uint16_t interval_max;
  uint16_t slave_latency;
  uint16_t timeout_multiplier;
  uint8_t id;
  uint8_t accept;
} l2cap_conn_param_update_resp_cp_IDB04A1;

/**
 * @addtogroup HIGH_LEVEL_INTERFACE HIGH_LEVEL_INTERFACE
 * @{
 */

/**
 * @addtogroup ACI ACI
 * @brief API for ACI layer.
 * @{
 */

/**
 * @defgroup ACI_EVENTS ACI_EVENTS
 * @brief BlueNRG events (vendor specific)
 * @{
 */

/**
 * @name Vendor Specific Event
 * @brief Vendor specific event for BlueNRG.
 */
typedef struct _evt_blue_aci{
  uint16_t ecode; /**< One of the BlueNRG event codes. */
  uint8_t  data[VARIABLE_SIZE];
} evt_blue_aci;

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

#endif /* __BLUENRG_ACI_CONST_H_ */
