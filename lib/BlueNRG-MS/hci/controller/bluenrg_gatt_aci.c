/******************** (C) COPYRIGHT 2014 STMicroelectronics ********************
* File Name          : bluenrg_gatt_aci.c
* Author             : AMS - AAS
* Version            : V1.0.0
* Date               : 26-Jun-2014
* Description        : File with GATT commands for BlueNRG FW6.3.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#include <assert.h>
#include <string.h>

#include "bluenrg_types.h"
#include "bluenrg_def.h"
#include "hci_const.h"
#include "bluenrg_aci_const.h"
#include "bluenrg_gatt_aci.h"
#include "bluenrg_gatt_server.h"
#include "bluenrg_gap.h"

#define MIN(a,b)            ((a) < (b) )? (a) : (b)
#define MAX(a,b)            ((a) > (b) )? (a) : (b)


void aci_gatt_init_begin(void)
{
  struct hci_request rq;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_INIT);

  hci_send_req(&rq);
}

tBleStatus aci_gatt_init_end(void)
{
  struct hci_response rq;
  uint8_t status;

  rq.rparam = &status;
  rq.rlen = 1;

  hci_recv_resp(&rq);

  return status;
}

void aci_gatt_add_serv_begin(uint8_t service_uuid_type, const uint8_t* service_uuid, uint8_t service_type, uint8_t max_attr_records)
{
  struct hci_request rq;
  uint8_t buffer[19];
  uint8_t uuid_len;
  uint8_t indx = 0;

  buffer[indx] = service_uuid_type;
  indx++;

  if(service_uuid_type == UUID_TYPE_16){
    uuid_len = 2;
  }
  else {
    uuid_len = 16;
  }
  memcpy(buffer + indx, service_uuid, uuid_len);
  indx +=  uuid_len;

  buffer[indx] = service_type;
  indx++;

  buffer[indx] = max_attr_records;
  indx++;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_ADD_SERV);
  rq.cparam = (void *)buffer;
  rq.clen = indx;

  hci_send_req(&rq);
}

tBleStatus aci_gatt_add_serv_end(uint16_t *serviceHandle)
{
  struct hci_response rq;
  gatt_add_serv_rp resp;

  rq.rparam = &resp;
  rq.rlen = GATT_ADD_SERV_RP_SIZE;

  hci_recv_resp(&rq);

  if (resp.status) {
    return resp.status;
  }

  *serviceHandle = btohs(resp.handle);

  return 0;
}

tBleStatus aci_gatt_include_service(uint16_t service_handle, uint16_t included_start_handle,
                                    uint16_t included_end_handle, uint8_t included_uuid_type,
                                    const uint8_t* included_uuid, uint16_t *included_handle)
{
  struct hci_request_and_response rq;
  gatt_include_serv_rp resp;
  uint8_t buffer[23];
  uint8_t uuid_len;
  uint8_t indx = 0;

  service_handle = htobs(service_handle);
  memcpy(buffer, &service_handle, 2);
  indx += 2;

  included_start_handle = htobs(included_start_handle);
  memcpy(buffer+indx, &included_start_handle, 2);
  indx += 2;

  included_end_handle = htobs(included_end_handle);
  memcpy(buffer+indx, &included_end_handle, 2);
  indx += 2;

  if(included_uuid_type == UUID_TYPE_16){
    uuid_len = 2;
  } else {
    uuid_len = 16;
  }

  buffer[indx] = included_uuid_type;
  indx++;

  memcpy(buffer + indx, included_uuid, uuid_len);
  indx += uuid_len;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_INCLUDE_SERV);
  rq.cparam = (void *)buffer;
  rq.clen = indx;
  rq.rparam = &resp;
  rq.rlen = GATT_INCLUDE_SERV_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  *included_handle = btohs(resp.handle);

  return 0;
}

void aci_gatt_add_char_begin(uint16_t serviceHandle,
                                   uint8_t charUuidType,
                                   const uint8_t* charUuid,
                                   uint8_t charValueLen,
                                   uint8_t charProperties,
                                   uint8_t secPermissions,
                                   uint8_t gattEvtMask,
                                   uint8_t encryKeySize,
                                   uint8_t isVariable)
{
  struct hci_request rq;
  uint8_t buffer[25];
  uint8_t uuid_len;
  uint8_t indx = 0;

  serviceHandle = htobs(serviceHandle);
  memcpy(buffer + indx, &serviceHandle, 2);
  indx += 2;

  buffer[indx] = charUuidType;
  indx++;

  if(charUuidType == UUID_TYPE_16){
    uuid_len = 2;
  }
  else {
    uuid_len = 16;
  }
  memcpy(buffer + indx, charUuid, uuid_len);
  indx +=  uuid_len;

  buffer[indx] = charValueLen;
  indx++;

  buffer[indx] = charProperties;
  indx++;

  buffer[indx] = secPermissions;
  indx++;

  buffer[indx] = gattEvtMask;
  indx++;

  buffer[indx] = encryKeySize;
  indx++;

  buffer[indx] = isVariable;
  indx++;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_ADD_CHAR);
  rq.cparam = (void *)buffer;
  rq.clen = indx;

  hci_send_req(&rq);
}

tBleStatus aci_gatt_add_char_end(uint16_t* charHandle)
{
  struct hci_response rq;
  gatt_add_serv_rp resp;

  rq.rparam = &resp;
  rq.rlen = GATT_ADD_CHAR_RP_SIZE;

  hci_recv_resp(&rq);

  if (resp.status) {
    return resp.status;
  }

  *charHandle = btohs(resp.handle);

  return 0;
}

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
                                  uint16_t* descHandle)
{
  struct hci_request_and_response rq;
  gatt_add_char_desc_rp resp;
  uint8_t buffer[HCI_MAX_PAYLOAD_SIZE];
  uint8_t uuid_len;
  uint8_t indx = 0;

  serviceHandle = htobs(serviceHandle);
  memcpy(buffer + indx, &serviceHandle, 2);
  indx += 2;

  charHandle = htobs(charHandle);
  memcpy(buffer + indx, &charHandle, 2);
  indx += 2;

  buffer[indx] = descUuidType;
  indx++;

  if(descUuidType == UUID_TYPE_16){
    uuid_len = 2;
  }
  else {
    uuid_len = 16;
  }
  memcpy(buffer + indx, uuid, uuid_len);
  indx +=  uuid_len;

  buffer[indx] = descValueMaxLen;
  indx++;

  buffer[indx] = descValueLen;
  indx++;

  if ((descValueLen+indx+5) > HCI_MAX_PAYLOAD_SIZE)
    return BLE_STATUS_INVALID_PARAMS;

  memcpy(buffer + indx, descValue, descValueLen);
  indx += descValueLen;

  buffer[indx] = secPermissions;
  indx++;

  buffer[indx] = accPermissions;
  indx++;

  buffer[indx] = gattEvtMask;
  indx++;

  buffer[indx] = encryKeySize;
  indx++;

  buffer[indx] = isVariable;
  indx++;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_ADD_CHAR_DESC);
  rq.cparam = (void *)buffer;
  rq.clen = indx;
  rq.rparam = &resp;
  rq.rlen = GATT_ADD_CHAR_DESC_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  if (resp.status) {
    return resp.status;
  }

  *descHandle = btohs(resp.handle);

  return 0;
}

void aci_gatt_update_char_value_begin(uint16_t servHandle,
                                            uint16_t charHandle,
                                            uint8_t charValOffset,
                                            uint8_t charValueLen,
                                            const void *charValue)
{
  struct hci_request rq;
  uint8_t buffer[HCI_MAX_PAYLOAD_SIZE];
  uint8_t indx = 0;

  assert(charValueLen + 6 <= HCI_MAX_PAYLOAD_SIZE);

  servHandle = htobs(servHandle);
  memcpy(buffer + indx, &servHandle, 2);
  indx += 2;

  charHandle = htobs(charHandle);
  memcpy(buffer + indx, &charHandle, 2);
  indx += 2;

  buffer[indx] = charValOffset;
  indx++;

  buffer[indx] = charValueLen;
  indx++;

  memcpy(buffer + indx, charValue, charValueLen);
  indx +=  charValueLen;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_UPD_CHAR_VAL);
  rq.cparam = (void *)buffer;
  rq.clen = indx;

  hci_send_req(&rq);
}

tBleStatus aci_gatt_update_char_value_end(void)
{
  struct hci_response rq;
  uint8_t status;

  rq.rparam = &status;
  rq.rlen = 1;

  hci_recv_resp(&rq);

  return status;
}

tBleStatus aci_gatt_del_char(uint16_t servHandle, uint16_t charHandle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_del_char_cp cp;

  cp.service_handle = htobs(servHandle);
  cp.char_handle = htobs(charHandle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_DEL_CHAR);
  rq.cparam = &cp;
  rq.clen = GATT_DEL_CHAR_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_del_service(uint16_t servHandle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_del_serv_cp cp;

  cp.service_handle = htobs(servHandle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_DEL_SERV);
  rq.cparam = &cp;
  rq.clen = GATT_DEL_SERV_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_del_include_service(uint16_t servHandle, uint16_t includeServHandle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_del_inc_serv_cp cp;

  cp.service_handle = htobs(servHandle);
  cp.inc_serv_handle = htobs(includeServHandle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_DEL_INC_SERV);
  rq.cparam = &cp;
  rq.clen = GATT_DEL_INC_SERV_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_set_event_mask(uint32_t event_mask)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_set_evt_mask_cp cp;

  cp.evt_mask = htobs(event_mask);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_SET_EVT_MASK);
  rq.cparam = &cp;
  rq.clen = GATT_SET_EVT_MASK_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_exchange_configuration(uint16_t conn_handle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_exchange_config_cp cp;

  cp.conn_handle = htobs(conn_handle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_EXCHANGE_CONFIG);
  rq.cparam = &cp;
  rq.clen = GATT_EXCHANGE_CONFIG_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_att_find_information_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  att_find_info_req_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.start_handle = htobs(start_handle);
  cp.end_handle = htobs(end_handle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_ATT_FIND_INFO_REQ);
  rq.cparam = &cp;
  rq.clen = ATT_FIND_INFO_REQ_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_att_find_by_type_value_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
                                          uint8_t* uuid, uint8_t attr_val_len, uint8_t* attr_val)
{
  struct hci_request_and_response rq;
  uint8_t status;
  att_find_by_type_value_req_cp cp;

  if(attr_val_len > sizeof(cp.attr_val))
    return BLE_STATUS_INVALID_PARAMS;

  cp.conn_handle = htobs(conn_handle);
  cp.start_handle = htobs(start_handle);
  cp.end_handle = htobs(end_handle);
  memcpy(cp.uuid, uuid, 2);
  cp.attr_val_len = attr_val_len;
  memcpy(cp.attr_val, attr_val, attr_val_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_ATT_FIND_BY_TYPE_VALUE_REQ);
  rq.cparam = &cp;
  rq.clen = ATT_FIND_BY_TYPE_VALUE_REQ_CP_SIZE + attr_val_len;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_att_read_by_type_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
                                    uint8_t  uuid_type, uint8_t* uuid)
{
  struct hci_request_and_response rq;
  uint8_t status;
  att_read_by_type_req_cp cp;
  uint8_t uuid_len;

  if(uuid_type == UUID_TYPE_16){
    uuid_len = 2;
  }
  else{
    uuid_len = 16;
  }

  cp.conn_handle = htobs(conn_handle);
  cp.start_handle = htobs(start_handle);
  cp.end_handle = htobs(end_handle);
  cp.uuid_type = uuid_type;
  memcpy(cp.uuid, uuid, uuid_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_ATT_READ_BY_TYPE_REQ);
  rq.cparam = &cp;
  rq.clen = ATT_READ_BY_TYPE_REQ_CP_SIZE + uuid_len;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_att_read_by_group_type_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
                                    uint8_t  uuid_type, uint8_t* uuid)
{
  struct hci_request_and_response rq;
  uint8_t status;
  att_read_by_group_type_req_cp cp;
  uint8_t uuid_len;

  if(uuid_type == UUID_TYPE_16){
    uuid_len = 2;
  }
  else{
    uuid_len = 16;
  }

  cp.conn_handle = htobs(conn_handle);
  cp.start_handle = htobs(start_handle);
  cp.end_handle = htobs(end_handle);
  cp.uuid_type = uuid_type;
  memcpy(cp.uuid, uuid, uuid_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_ATT_READ_BY_GROUP_TYPE_REQ);
  rq.cparam = &cp;
  rq.clen = ATT_READ_BY_GROUP_TYPE_REQ_CP_SIZE + uuid_len;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_att_prepare_write_req(uint16_t conn_handle, uint16_t attr_handle, uint16_t value_offset,
                                    uint8_t  attr_val_len, uint8_t* attr_val)
{
  struct hci_request_and_response rq;
  uint8_t status;
  att_prepare_write_req_cp cp;

  if(attr_val_len > sizeof(cp.attr_val))
    return BLE_STATUS_INVALID_PARAMS;

  cp.conn_handle = htobs(conn_handle);
  cp.attr_handle = htobs(attr_handle);
  cp.value_offset = htobs(value_offset);
  cp.attr_val_len = attr_val_len;
  memcpy(cp.attr_val, attr_val, attr_val_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_ATT_PREPARE_WRITE_REQ);
  rq.cparam = &cp;
  rq.clen = ATT_PREPARE_WRITE_REQ_CP_SIZE + attr_val_len;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_att_execute_write_req(uint16_t conn_handle, uint8_t execute)
{
  struct hci_request_and_response rq;
  uint8_t status;
  att_execute_write_req_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.execute = execute;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_ATT_EXECUTE_WRITE_REQ);
  rq.cparam = &cp;
  rq.clen = ATT_EXECUTE_WRITE_REQ_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_disc_all_prim_services(uint16_t conn_handle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_disc_all_prim_services_cp cp;

  cp.conn_handle = htobs(conn_handle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_DISC_ALL_PRIM_SERVICES);
  rq.cparam = &cp;
  rq.clen = GATT_DISC_ALL_PRIM_SERVICES_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_disc_prim_service_by_uuid(uint16_t conn_handle, uint8_t uuid_type, uint8_t* uuid)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_disc_prim_service_by_uuid_cp cp;
  uint8_t uuid_len;

  if(uuid_type == UUID_TYPE_16){
    uuid_len = 2;
  }
  else{
    uuid_len = 16;
  }

  cp.conn_handle = htobs(conn_handle);
  cp.uuid_type = uuid_type;
  memcpy(cp.uuid, uuid, uuid_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_DISC_PRIM_SERVICE_BY_UUID);
  rq.cparam = &cp;
  rq.clen = GATT_DISC_PRIM_SERVICE_BY_UUID_CP_SIZE + uuid_len;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_find_included_services(uint16_t conn_handle, uint16_t start_service_handle,
                                           uint16_t end_service_handle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_find_included_services_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.start_handle = htobs(start_service_handle);
  cp.end_handle = htobs(end_service_handle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_FIND_INCLUDED_SERVICES);
  rq.cparam = &cp;
  rq.clen = GATT_FIND_INCLUDED_SERVICES_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_disc_all_charac_of_serv(uint16_t conn_handle, uint16_t start_attr_handle,
                                            uint16_t end_attr_handle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_disc_all_charac_of_serv_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.start_attr_handle = htobs(start_attr_handle);
  cp.end_attr_handle = htobs(end_attr_handle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_DISC_ALL_CHARAC_OF_SERV);
  rq.cparam = &cp;
  rq.clen = GATT_DISC_ALL_CHARAC_OF_SERV_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

void aci_gatt_disc_charac_by_uuid_begin(
    uint16_t conn_handle, uint16_t start_handle,
    uint16_t end_handle, uint8_t charUuidType,
    const uint8_t* charUuid)
{
  struct hci_request rq;

  uint8_t buffer[23];
  uint8_t uuid_len;
  uint8_t indx = 0;

  conn_handle = htobs(conn_handle);
  memcpy(buffer + indx, &conn_handle, 2);
  indx += 2;

  start_handle = htobs(start_handle);
  memcpy(buffer + indx, &start_handle, 2);
  indx += 2;

  end_handle = htobs(end_handle);
  memcpy(buffer + indx, &end_handle, 2);
  indx += 2;

  buffer[indx] = charUuidType;
  indx++;

  if(charUuidType == 0x01){
    uuid_len = 2;
  }
  else {
    uuid_len = 16;
  }
  memcpy(buffer + indx, charUuid, uuid_len);
  indx +=  uuid_len;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_DISC_CHARAC_BY_UUID);
  rq.cparam = (void *)buffer;
  rq.clen = indx;
  rq.event = EVT_CMD_STATUS;

  hci_send_req(&rq);
}

tBleStatus aci_gatt_disc_charac_by_uuid_end(void)
{
  struct hci_response rq;
  uint8_t status;

  rq.rparam = &status;
  rq.rlen = 1;

  hci_recv_resp(&rq);

  return status;
}

tBleStatus aci_gatt_disc_all_charac_descriptors(uint16_t conn_handle, uint16_t char_val_handle,
                                                uint16_t char_end_handle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_disc_all_charac_descriptors_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.char_val_handle = htobs(char_val_handle);
  cp.char_end_handle = htobs(char_end_handle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_DISC_ALL_CHARAC_DESCRIPTORS);
  rq.cparam = &cp;
  rq.clen = GATT_DISC_ALL_CHARAC_DESCRIPTORS_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_read_charac_val(uint16_t conn_handle, uint16_t attr_handle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_read_charac_val_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.attr_handle = htobs(attr_handle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_READ_CHARAC_VAL);
  rq.cparam = &cp;
  rq.clen = GATT_READ_CHARAC_VAL_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_read_using_charac_uuid(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
                                    uint8_t  uuid_type, uint8_t* uuid)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_read_using_charac_uuid_cp cp;
  uint8_t uuid_len;

  if(uuid_type == UUID_TYPE_16){
    uuid_len = 2;
  }
  else{
    uuid_len = 16;
  }

  cp.conn_handle = htobs(conn_handle);
  cp.start_handle = htobs(start_handle);
  cp.end_handle = htobs(end_handle);
  cp.uuid_type = uuid_type;
  memcpy(cp.uuid, uuid, uuid_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_READ_USING_CHARAC_UUID);
  rq.cparam = &cp;
  rq.clen = GATT_READ_USING_CHARAC_UUID_CP_SIZE + uuid_len;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_read_long_charac_val(uint16_t conn_handle, uint16_t attr_handle,
                                         uint16_t val_offset)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_read_long_charac_val_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.attr_handle = htobs(attr_handle);
  cp.val_offset = htobs(val_offset);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_READ_LONG_CHARAC_VAL);
  rq.cparam = &cp;
  rq.clen = GATT_READ_LONG_CHARAC_VAL_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_read_multiple_charac_val(uint16_t conn_handle, uint8_t num_handles,
                                             uint8_t* set_of_handles)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_read_multiple_charac_val_cp cp;

  if(num_handles*2 > sizeof(cp.set_of_handles))
    return BLE_STATUS_INVALID_PARAMS;

  cp.conn_handle = htobs(conn_handle);
  cp.num_handles = htobs(num_handles);
  memcpy(cp.set_of_handles, set_of_handles, 2*num_handles);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_READ_MULTIPLE_CHARAC_VAL);
  rq.cparam = &cp;
  rq.clen = GATT_READ_MULTIPLE_CHARAC_VAL_CP_SIZE + 2*num_handles;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

void aci_gatt_write_charac_value_begin(uint16_t conn_handle,
                                             uint16_t attr_handle,
                                             uint8_t value_len,
                                             uint8_t *attr_value)
{
  struct hci_request rq;
  uint8_t buffer[HCI_MAX_PAYLOAD_SIZE];
  uint8_t indx = 0;

  assert(value_len + 5 <= HCI_MAX_PAYLOAD_SIZE);

  conn_handle = htobs(conn_handle);
  memcpy(buffer + indx, &conn_handle, 2);
  indx += 2;

  attr_handle = htobs(attr_handle);
  memcpy(buffer + indx, &attr_handle, 2);
  indx += 2;

  buffer[indx] = value_len;
  indx++;

  memcpy(buffer + indx, attr_value, value_len);
  indx +=  value_len;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_WRITE_CHAR_VALUE);
  rq.cparam = (void *)buffer;
  rq.clen = indx;
  rq.event = EVT_CMD_STATUS;

  hci_send_req(&rq);
}

tBleStatus aci_gatt_write_charac_value_end(void)
{
  struct hci_response rq;
  uint8_t status;

  rq.rparam = &status;
  rq.rlen = 1;

  hci_recv_resp(&rq);

  return status;
}

tBleStatus aci_gatt_write_long_charac_val(uint16_t conn_handle, uint16_t attr_handle,
                                          uint16_t val_offset, uint8_t val_len, const uint8_t* attr_val)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_write_long_charac_val_cp cp;

  if(val_len > sizeof(cp.attr_val))
    return BLE_STATUS_INVALID_PARAMS;

  cp.conn_handle = htobs(conn_handle);
  cp.attr_handle = htobs(attr_handle);
  cp.val_offset = htobs(val_offset);
  cp.val_len = val_len;
  memcpy(cp.attr_val, attr_val, val_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_WRITE_LONG_CHARAC_VAL);
  rq.cparam = &cp;
  rq.clen = GATT_WRITE_LONG_CHARAC_VAL_CP_SIZE + val_len;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_write_charac_reliable(uint16_t conn_handle, uint16_t attr_handle,
                                          uint16_t val_offset, uint8_t val_len, uint8_t* attr_val)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_write_charac_reliable_cp cp;

  if(val_len > sizeof(cp.attr_val))
    return BLE_STATUS_INVALID_PARAMS;

  cp.conn_handle = htobs(conn_handle);
  cp.attr_handle = htobs(attr_handle);
  cp.val_offset = htobs(val_offset);
  cp.val_len = val_len;
  memcpy(cp.attr_val, attr_val, val_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_WRITE_CHARAC_RELIABLE);
  rq.cparam = &cp;
  rq.clen = GATT_WRITE_CHARAC_RELIABLE_CP_SIZE + val_len;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_write_long_charac_desc(uint16_t conn_handle, uint16_t attr_handle,
                                          uint16_t val_offset, uint8_t val_len, uint8_t* attr_val)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_write_charac_reliable_cp cp;

  if(val_len > sizeof(cp.attr_val))
    return BLE_STATUS_INVALID_PARAMS;

  cp.conn_handle = htobs(conn_handle);
  cp.attr_handle = htobs(attr_handle);
  cp.val_offset = htobs(val_offset);
  cp.val_len = val_len;
  memcpy(cp.attr_val, attr_val, val_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_WRITE_LONG_CHARAC_DESC);
  rq.cparam = &cp;
  rq.clen = GATT_WRITE_LONG_CHARAC_DESC_CP_SIZE + val_len;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_read_long_charac_desc(uint16_t conn_handle, uint16_t attr_handle,
                                          uint16_t val_offset)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_read_long_charac_desc_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.attr_handle = htobs(attr_handle);
  cp.val_offset = htobs(val_offset);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_READ_LONG_CHARAC_DESC);
  rq.cparam = &cp;
  rq.clen = GATT_READ_LONG_CHARAC_DESC_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_write_charac_descriptor(uint16_t conn_handle, uint16_t attr_handle,
                                           uint8_t value_len, uint8_t *attr_value)
{
  struct hci_request_and_response rq;
  uint8_t status;
  uint8_t buffer[HCI_MAX_PAYLOAD_SIZE];
  uint8_t indx = 0;

  if ((value_len+5) > HCI_MAX_PAYLOAD_SIZE)
    return BLE_STATUS_INVALID_PARAMS;

  conn_handle = htobs(conn_handle);
  memcpy(buffer + indx, &conn_handle, 2);
  indx += 2;

  attr_handle = htobs(attr_handle);
  memcpy(buffer + indx, &attr_handle, 2);
  indx += 2;

  buffer[indx] = value_len;
  indx++;

  memcpy(buffer + indx, attr_value, value_len);
  indx +=  value_len;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_WRITE_CHAR_DESCRIPTOR);
  rq.cparam = (void *)buffer;
  rq.clen = indx;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_read_charac_desc(uint16_t conn_handle, uint16_t attr_handle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_read_long_charac_desc_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.attr_handle = htobs(attr_handle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_READ_CHAR_DESCRIPTOR);
  rq.cparam = &cp;
  rq.clen = GATT_READ_CHAR_DESCRIPTOR_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

void aci_gatt_write_without_response_begin(
    uint16_t conn_handle, uint16_t attr_handle,
    uint8_t val_len, const uint8_t* attr_val)
{
  struct hci_request rq;
  gatt_write_without_resp_cp cp;

  assert(val_len <= sizeof(cp.attr_val));

  cp.conn_handle = htobs(conn_handle);
  cp.attr_handle = htobs(attr_handle);
  cp.val_len = val_len;
  memcpy(cp.attr_val, attr_val, val_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_WRITE_WITHOUT_RESPONSE);
  rq.cparam = &cp;
  rq.clen = GATT_WRITE_WITHOUT_RESPONSE_CP_SIZE + val_len;

  hci_send_req(&rq);
}

tBleStatus aci_gatt_write_without_response_end(void)
{
  struct hci_response rq;
  uint8_t status;

  rq.rparam = &status;
  rq.rlen = 1;

  hci_recv_resp(&rq);

  return status;
}

tBleStatus aci_gatt_signed_write_without_resp(uint16_t conn_handle, uint16_t attr_handle,
                                              uint8_t val_len, uint8_t* attr_val)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_signed_write_without_resp_cp cp;

  if(val_len > sizeof(cp.attr_val))
    return BLE_STATUS_INVALID_PARAMS;

  cp.conn_handle = htobs(conn_handle);
  cp.attr_handle = htobs(attr_handle);
  cp.val_len = val_len;
  memcpy(cp.attr_val, attr_val, val_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_SIGNED_WRITE_WITHOUT_RESPONSE);
  rq.cparam = &cp;
  rq.clen = GATT_SIGNED_WRITE_WITHOUT_RESPONSE_CP_SIZE + val_len;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_confirm_indication(uint16_t conn_handle)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_confirm_indication_cp cp;

  cp.conn_handle = htobs(conn_handle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_CONFIRM_INDICATION);
  rq.cparam = &cp;
  rq.clen = GATT_CONFIRM_INDICATION_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_write_response(uint16_t conn_handle,
                                   uint16_t attr_handle,
                                   uint8_t write_status,
                                   uint8_t err_code,
                                   uint8_t att_val_len,
                                   uint8_t *att_val)
{
  struct hci_request_and_response rq;
  uint8_t status;
  uint8_t buffer[HCI_MAX_PAYLOAD_SIZE];
  uint8_t indx = 0;

  if ((att_val_len+7) > HCI_MAX_PAYLOAD_SIZE)
    return BLE_STATUS_INVALID_PARAMS;

  conn_handle = htobs(conn_handle);
  memcpy(buffer + indx, &conn_handle, 2);
  indx += 2;

  attr_handle = htobs(attr_handle);
  memcpy(buffer + indx, &attr_handle, 2);
  indx += 2;

  buffer[indx] = write_status;
  indx += 1;

  buffer[indx] = err_code;
  indx += 1;

  buffer[indx] = att_val_len;
  indx += 1;

  memcpy(buffer + indx, att_val, att_val_len);
  indx += att_val_len;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_WRITE_RESPONSE);
  rq.cparam = (void *)buffer;
  rq.clen = indx;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  if (status) {
    return status;
  }

  return 0;
}

tBleStatus aci_gatt_allow_read(uint16_t conn_handle)
{
    struct hci_request_and_response rq;
    gatt_allow_read_cp cp;
    uint8_t status;

    cp.conn_handle = htobs(conn_handle);

    memset(&rq, 0, sizeof(rq));
    rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_ALLOW_READ);
    rq.cparam = &cp;
    rq.clen = GATT_ALLOW_READ_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;

    hci_send_req_recv_rsp(&rq);

    return status;
}

tBleStatus aci_gatt_set_security_permission(uint16_t service_handle, uint16_t attr_handle,
                                            uint8_t security_permission)
{
    struct hci_request_and_response rq;
    gatt_set_security_permission_cp cp;
    uint8_t status;

    cp.service_handle = htobs(service_handle);
    cp.attr_handle = htobs(attr_handle);
    cp.security_permission = security_permission;

    memset(&rq, 0, sizeof(rq));
    rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_SET_SECURITY_PERMISSION);
    rq.cparam = &cp;
    rq.clen = GATT_GATT_SET_SECURITY_PERMISSION_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;

    hci_send_req_recv_rsp(&rq);

    return status;
}

tBleStatus aci_gatt_set_desc_value(uint16_t servHandle,
                                   uint16_t charHandle,
                                   uint16_t charDescHandle,
                                   uint16_t charDescValOffset,
                                   uint8_t charDescValueLen,
                                   const void *charDescValue)
{
  struct hci_request_and_response rq;
  uint8_t status;
  uint8_t buffer[HCI_MAX_PAYLOAD_SIZE];
  uint8_t indx = 0;

  if ((charDescValueLen+9) > HCI_MAX_PAYLOAD_SIZE)
    return BLE_STATUS_INVALID_PARAMS;

  servHandle = htobs(servHandle);
  memcpy(buffer + indx, &servHandle, 2);
  indx += 2;

  charHandle = htobs(charHandle);
  memcpy(buffer + indx, &charHandle, 2);
  indx += 2;

  charDescHandle = htobs(charDescHandle);
  memcpy(buffer + indx, &charDescHandle, 2);
  indx += 2;

  memcpy(buffer + indx, &charDescValOffset, 2);
  indx += 2;

  buffer[indx] = charDescValueLen;
  indx++;

  memcpy(buffer + indx, charDescValue, charDescValueLen);
  indx +=  charDescValueLen;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_SET_DESC_VAL);
  rq.cparam = (void *)buffer;
  rq.clen = indx;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_gatt_read_handle_value(uint16_t attr_handle, uint16_t data_len, uint16_t *data_len_out_p, uint8_t *data)
{
  struct hci_request_and_response rq;
  gatt_read_handle_val_cp cp;
  gatt_read_handle_val_rp rp;

  if(data_len > sizeof(rp.value))
    return BLE_STATUS_INVALID_PARAMS;

  cp.attr_handle = htobs(attr_handle);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_READ_HANDLE_VALUE);
  rq.cparam = &cp;
  rq.clen = sizeof(cp);
  rq.rparam = &rp;
  rq.rlen = sizeof(rp);

  hci_send_req_recv_rsp(&rq);

  if(rp.status)
    return rp.status;

  *data_len_out_p = btohs(rp.value_len);

  memcpy(data, rp.value, MIN(data_len, *data_len_out_p));

  return 0;
}

tBleStatus aci_gatt_read_handle_value_offset(uint16_t attr_handle, uint8_t offset, uint16_t data_len, uint16_t *data_len_out_p, uint8_t *data)
{
  struct hci_request_and_response rq;
  gatt_read_handle_val_offset_cp cp;
  gatt_read_handle_val_offset_rp rp;

  if(data_len > sizeof(rp.value))
    return BLE_STATUS_INVALID_PARAMS;

  cp.attr_handle = htobs(attr_handle);
  cp.offset = offset;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_READ_HANDLE_VALUE_OFFSET);
  rq.cparam = &cp;
  rq.clen = sizeof(cp);
  rq.rparam = &rp;
  rq.rlen = sizeof(rp);

  hci_send_req_recv_rsp(&rq);

  if(rp.status)
    return rp.status;

  *data_len_out_p = rp.value_len;

  memcpy(data, rp.value, MIN(data_len, *data_len_out_p));

  return 0;
}

tBleStatus aci_gatt_update_char_value_ext(uint16_t service_handle, uint16_t char_handle,
                                                  uint8_t update_type, uint16_t char_length,
                                                  uint16_t value_offset, uint8_t value_length,
                                                  const uint8_t* value)
{
  struct hci_request_and_response rq;
  uint8_t status;
  gatt_upd_char_val_ext_cp cp;

  if(value_length > sizeof(cp.value))
    return BLE_STATUS_INVALID_PARAMS;

  cp.service_handle = htobs(service_handle);
  cp.char_handle = htobs(char_handle);
  cp.update_type = update_type;
  cp.char_length = htobs(char_length);
  cp.value_offset = htobs(value_offset);
  cp.value_length = value_length;
  memcpy(cp.value, value, value_length);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GATT_UPD_CHAR_VAL_EXT);
  rq.cparam = &cp;
  rq.clen = GATT_UPD_CHAR_VAL_EXT_CP_SIZE + value_length;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}


