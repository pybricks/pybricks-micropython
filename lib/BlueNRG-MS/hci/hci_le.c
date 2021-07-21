/**
  ******************************************************************************
  * @file    hci_le.c
  * @author  AMG RF  Application Team
  * @brief   Function for managing HCI interface.
  ******************************************************************************
  *
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  */

#include <string.h>

#include "hci_le.h"
#include "bluenrg_types.h"
#include "bluenrg_def.h"
#include "hci_const.h"
#include "bluenrg_conf.h"

#define MIN(a,b)            ((a) < (b) )? (a) : (b)
#define MAX(a,b)            ((a) > (b) )? (a) : (b)

int hci_reset(void)
{
  struct hci_request_and_response rq;
  uint8_t status;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_HOST_CTL, OCF_RESET);
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_disconnect(uint16_t handle, uint8_t reason)
{
  struct hci_request_and_response rq;
  disconnect_cp cp;
  uint8_t status;

  cp.handle = handle;
  cp.reason = reason;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LINK_CTL, OCF_DISCONNECT);
  rq.cparam = &cp;
  rq.clen = DISCONNECT_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_le_read_local_version(uint8_t *hci_version, uint16_t *hci_revision, uint8_t *lmp_pal_version,
                              uint16_t *manufacturer_name, uint16_t *lmp_pal_subversion)
{
  struct hci_request_and_response rq;
  read_local_version_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_INFO_PARAM, OCF_READ_LOCAL_VERSION);
  rq.cparam = NULL;
  rq.clen = 0;
  rq.rparam = &resp;
  rq.rlen = READ_LOCAL_VERSION_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }


  *hci_version = resp.hci_version;
  *hci_revision =  btohs(resp.hci_revision);
  *lmp_pal_version = resp.lmp_pal_version;
  *manufacturer_name = btohs(resp.manufacturer_name);
  *lmp_pal_subversion = btohs(resp.lmp_pal_subversion);

  return 0;
}

int hci_le_read_buffer_size(uint16_t *pkt_len, uint8_t *max_pkt)
{
  struct hci_request_and_response rq;
  le_read_buffer_size_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_READ_BUFFER_SIZE);
  rq.cparam = NULL;
  rq.clen = 0;
  rq.rparam = &resp;
  rq.rlen = LE_READ_BUFFER_SIZE_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  *pkt_len = resp.pkt_len;
  *max_pkt = resp.max_pkt;

  return 0;
}

int hci_le_set_advertising_parameters(uint16_t min_interval, uint16_t max_interval, uint8_t advtype,
                                      uint8_t own_bdaddr_type, uint8_t direct_bdaddr_type, const tBDAddr direct_bdaddr, uint8_t chan_map,
                                      uint8_t filter)
{
  struct hci_request_and_response rq;
  le_set_adv_parameters_cp adv_cp;
  uint8_t status;

  memset(&adv_cp, 0, sizeof(adv_cp));
  adv_cp.min_interval = min_interval;
  adv_cp.max_interval = max_interval;
  adv_cp.advtype = advtype;
  adv_cp.own_bdaddr_type = own_bdaddr_type;
  adv_cp.direct_bdaddr_type = direct_bdaddr_type;
  if(direct_bdaddr != NULL)
    memcpy(adv_cp.direct_bdaddr,direct_bdaddr,sizeof(adv_cp.direct_bdaddr));
  adv_cp.chan_map = chan_map;
  adv_cp.filter = filter;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADV_PARAMETERS);
  rq.cparam = &adv_cp;
  rq.clen = LE_SET_ADV_PARAMETERS_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

void hci_le_set_advertising_data_begin(uint8_t length, const uint8_t *data)
{
  struct hci_request rq;
  le_set_adv_data_cp adv_cp;

  memset(&adv_cp, 0, sizeof(adv_cp));
  adv_cp.length = length;
  memcpy(adv_cp.data, data, MIN(31,length));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADV_DATA);
  rq.cparam = &adv_cp;
  rq.clen = LE_SET_ADV_DATA_CP_SIZE;

  hci_send_req(&rq);
}

tBleStatus hci_le_set_advertising_data_end(void)
{
  struct hci_response rq;
  uint8_t status;

  rq.rparam = &status;
  rq.rlen = 1;

  hci_recv_resp(&rq);

  return status;
}

int hci_le_set_advertise_enable(uint8_t enable)
{
  struct hci_request_and_response rq;
  le_set_advertise_enable_cp adv_cp;
  uint8_t status;

  memset(&adv_cp, 0, sizeof(adv_cp));
  adv_cp.enable = enable?1:0;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADVERTISE_ENABLE);
  rq.cparam = &adv_cp;
  rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_le_set_scan_parameters(uint8_t type, uint16_t interval,
                               uint16_t window, uint8_t own_bdaddr_type,
                               uint8_t filter)
{
  struct hci_request_and_response rq;
  le_set_scan_parameters_cp scan_cp;
  uint8_t status;

  memset(&scan_cp, 0, sizeof(scan_cp));
  scan_cp.type = type;
  scan_cp.interval = interval;
  scan_cp.window = window;
  scan_cp.own_bdaddr_type = own_bdaddr_type;
  scan_cp.filter = filter;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_PARAMETERS);
  rq.cparam = &scan_cp;
  rq.clen = LE_SET_SCAN_PARAMETERS_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_le_set_scan_enable(uint8_t enable, uint8_t filter_dup)
{
  struct hci_request_and_response rq;
  le_set_scan_enable_cp scan_cp;
  uint8_t status;

  memset(&scan_cp, 0, sizeof(scan_cp));
  scan_cp.enable = enable?1:0;
  scan_cp.filter_dup = filter_dup;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE);
  rq.cparam = &scan_cp;
  rq.clen = LE_SET_SCAN_ENABLE_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_le_rand(uint8_t random_number[8])
{
  struct hci_request_and_response rq;
  le_rand_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_RAND);
  rq.cparam = NULL;
  rq.clen = 0;
  rq.rparam = &resp;
  rq.rlen = LE_RAND_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  memcpy(random_number, resp.random, 8);

  return 0;
}

void hci_le_set_scan_response_data_begin(uint8_t length, const uint8_t *data)
{
  struct hci_request rq;
  le_set_scan_response_data_cp scan_resp_cp;

  memset(&scan_resp_cp, 0, sizeof(scan_resp_cp));
  scan_resp_cp.length = length;
  memcpy(scan_resp_cp.data, data, MIN(31,length));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_RESPONSE_DATA);
  rq.cparam = &scan_resp_cp;
  rq.clen = LE_SET_SCAN_RESPONSE_DATA_CP_SIZE;

  hci_send_req(&rq);
}

tBleStatus hci_le_set_scan_response_data_end(void)
{
  struct hci_response rq;
  uint8_t status;

  rq.rparam = &status;
  rq.rlen = 1;

  hci_recv_resp(&rq);

  return status;
}

int hci_le_read_advertising_channel_tx_power(int8_t *tx_power_level)
{
  struct hci_request_and_response rq;
  le_read_adv_channel_tx_power_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_READ_ADV_CHANNEL_TX_POWER);
  rq.cparam = NULL;
  rq.clen = 0;
  rq.rparam = &resp;
  rq.rlen = LE_RAND_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  *tx_power_level = resp.level;

  return 0;
}

int hci_le_set_random_address(tBDAddr bdaddr)
{
  struct hci_request_and_response rq;
  le_set_random_address_cp set_rand_addr_cp;
  uint8_t status;

  memset(&set_rand_addr_cp, 0, sizeof(set_rand_addr_cp));
  memcpy(set_rand_addr_cp.bdaddr, bdaddr, sizeof(tBDAddr));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_RANDOM_ADDRESS);
  rq.cparam = &set_rand_addr_cp;
  rq.clen = LE_SET_RANDOM_ADDRESS_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_read_bd_addr(tBDAddr bdaddr)
{
  struct hci_request_and_response rq;
  read_bd_addr_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_INFO_PARAM, OCF_READ_BD_ADDR);
  rq.cparam = NULL;
  rq.clen = 0;
  rq.rparam = &resp;
  rq.rlen = READ_BD_ADDR_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }
  memcpy(bdaddr, resp.bdaddr, sizeof(tBDAddr));

  return 0;
}

int hci_le_create_connection(uint16_t interval, uint16_t window, uint8_t initiator_filter, uint8_t peer_bdaddr_type,
                             const tBDAddr peer_bdaddr, uint8_t own_bdaddr_type, uint16_t min_interval, uint16_t max_interval,
                             uint16_t latency, uint16_t supervision_timeout, uint16_t min_ce_length, uint16_t max_ce_length)
{
  struct hci_request_and_response rq;
  le_create_connection_cp create_cp;
  uint8_t status;

  memset(&create_cp, 0, sizeof(create_cp));
  create_cp.interval = interval;
  create_cp.window =  window;
  create_cp.initiator_filter = initiator_filter;
  create_cp.peer_bdaddr_type = peer_bdaddr_type;
  memcpy(create_cp.peer_bdaddr, peer_bdaddr, sizeof(tBDAddr));
  create_cp.own_bdaddr_type = own_bdaddr_type;
  create_cp.min_interval=min_interval;
  create_cp.max_interval=max_interval;
  create_cp.latency = latency;
  create_cp.supervision_timeout=supervision_timeout;
  create_cp.min_ce_length=min_ce_length;
  create_cp.max_ce_length=max_ce_length;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_CREATE_CONN);
  rq.cparam = &create_cp;
  rq.clen = LE_CREATE_CONN_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_le_create_connection_cancel(void)
{
  struct hci_request_and_response rq;
  uint8_t status;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_CREATE_CONN_CANCEL);
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_le_encrypt(uint8_t key[16], uint8_t plaintextData[16], uint8_t encryptedData[16])
{
  struct hci_request_and_response rq;
  le_encrypt_cp params;
  le_encrypt_rp resp;

  memset(&resp, 0, sizeof(resp));

  memcpy(params.key, key, 16);
  memcpy(params.plaintext, plaintextData, 16);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_ENCRYPT);
  rq.cparam = &params;
  rq.clen = LE_ENCRYPT_CP_SIZE;
  rq.rparam = &resp;
  rq.rlen = LE_ENCRYPT_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  memcpy(encryptedData, resp.encdata, 16);

  return 0;
}

int hci_le_ltk_request_reply(uint8_t key[16])
{
  struct hci_request_and_response rq;
  le_ltk_reply_cp params;
  le_ltk_reply_rp resp;

  memset(&resp, 0, sizeof(resp));

  params.handle = 1;
  memcpy(params.key, key, 16);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_LTK_REPLY);
  rq.cparam = &params;
  rq.clen = LE_LTK_REPLY_CP_SIZE;
  rq.rparam = &resp;
  rq.rlen = LE_LTK_REPLY_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  return resp.status;
}

int hci_le_ltk_request_neg_reply(void)
{
  struct hci_request_and_response rq;
  le_ltk_neg_reply_cp params;
  le_ltk_neg_reply_rp resp;

  memset(&resp, 0, sizeof(resp));

  params.handle = 1;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_LTK_NEG_REPLY);
  rq.cparam = &params;
  rq.clen = LE_LTK_NEG_REPLY_CP_SIZE;
  rq.rparam = &resp;
  rq.rlen = LE_LTK_NEG_REPLY_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  return resp.status;
}

int hci_le_read_white_list_size(uint8_t *size)
{
  struct hci_request_and_response rq;
  le_read_white_list_size_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_READ_WHITE_LIST_SIZE);
  rq.rparam = &resp;
  rq.rlen = LE_READ_WHITE_LIST_SIZE_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  *size = resp.size;

  return 0;
}

int hci_le_clear_white_list(void)
{
  struct hci_request_and_response rq;
  uint8_t status;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_CLEAR_WHITE_LIST);
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_le_add_device_to_white_list(uint8_t bdaddr_type, tBDAddr bdaddr)
{
  struct hci_request_and_response rq;
  le_add_device_to_white_list_cp params;
  uint8_t status;

  params.bdaddr_type = bdaddr_type;
  memcpy(params.bdaddr, bdaddr, 6);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_ADD_DEVICE_TO_WHITE_LIST);
  rq.cparam = &params;
  rq.clen = LE_ADD_DEVICE_TO_WHITE_LIST_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_le_remove_device_from_white_list(uint8_t bdaddr_type, tBDAddr bdaddr)
{
  struct hci_request_and_response rq;
  le_remove_device_from_white_list_cp params;
  uint8_t status;

  params.bdaddr_type = bdaddr_type;
  memcpy(params.bdaddr, bdaddr, 6);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_REMOVE_DEVICE_FROM_WHITE_LIST);
  rq.cparam = &params;
  rq.clen = LE_REMOVE_DEVICE_FROM_WHITE_LIST_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_read_transmit_power_level(uint16_t *conn_handle, uint8_t type, int8_t * tx_level)
{
  struct hci_request_and_response rq;
  read_transmit_power_level_cp params;
  read_transmit_power_level_rp resp;

  memset(&resp, 0, sizeof(resp));

  params.handle = *conn_handle;
  params.type = type;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_HOST_CTL, OCF_READ_TRANSMIT_POWER_LEVEL);
  rq.cparam = &params;
  rq.clen = READ_TRANSMIT_POWER_LEVEL_CP_SIZE;
  rq.rparam = &resp;
  rq.rlen = READ_TRANSMIT_POWER_LEVEL_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  *conn_handle = resp.handle;
  *tx_level = resp.level;

  return 0;
}

int hci_read_rssi(uint16_t *conn_handle, int8_t * rssi)
{
  struct hci_request_and_response rq;
  read_rssi_cp params;
  read_rssi_rp resp;

  memset(&resp, 0, sizeof(resp));

  params.handle = *conn_handle;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_STATUS_PARAM, OCF_READ_RSSI);
  rq.cparam = &params;
  rq.clen = READ_RSSI_CP_SIZE;
  rq.rparam = &resp;
  rq.rlen = READ_RSSI_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  *conn_handle = resp.handle;
  *rssi = resp.rssi;

  return 0;
}

int hci_le_read_local_supported_features(uint8_t *features)
{
  struct hci_request_and_response rq;
  le_read_local_supported_features_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_READ_LOCAL_SUPPORTED_FEATURES);
  rq.rparam = &resp;
  rq.rlen = LE_READ_LOCAL_SUPPORTED_FEATURES_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  memcpy(features, resp.features, sizeof(resp.features));

  return 0;
}

int hci_le_read_channel_map(uint16_t conn_handle, uint8_t ch_map[5])
{
  struct hci_request_and_response rq;
  le_read_channel_map_cp params;
  le_read_channel_map_rp resp;

  memset(&resp, 0, sizeof(resp));

  params.handle = conn_handle;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_READ_CHANNEL_MAP);
  rq.cparam = &params;
  rq.clen = LE_READ_CHANNEL_MAP_CP_SIZE;
  rq.rparam = &resp;
  rq.rlen = LE_READ_CHANNEL_MAP_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  memcpy(ch_map, resp.map, 5);

  return 0;
}

int hci_le_read_supported_states(uint8_t states[8])
{
  struct hci_request_and_response rq;
  le_read_supported_states_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_READ_SUPPORTED_STATES);
  rq.rparam = &resp;
  rq.rlen = LE_READ_SUPPORTED_STATES_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  memcpy(states, resp.states, 8);

  return 0;
}

int hci_le_receiver_test(uint8_t frequency)
{
  struct hci_request_and_response rq;
  le_receiver_test_cp params;
  uint8_t status;

  params.frequency = frequency;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_RECEIVER_TEST);
  rq.cparam = &params;
  rq.clen = LE_RECEIVER_TEST_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_le_transmitter_test(uint8_t frequency, uint8_t length, uint8_t payload)
{
  struct hci_request_and_response rq;
  le_transmitter_test_cp params;
  uint8_t status;

  params.frequency = frequency;
  params.length = length;
  params.payload = payload;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_TRANSMITTER_TEST);
  rq.cparam = &params;
  rq.clen = LE_TRANSMITTER_TEST_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

int hci_le_test_end(uint16_t *num_pkts)
{
  struct hci_request_and_response rq;
  le_test_end_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_LE_CTL, OCF_LE_TEST_END);
  rq.rparam = &resp;
  rq.rlen = LE_TEST_END_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  *num_pkts = resp.num_pkts;

  return 0;
}
