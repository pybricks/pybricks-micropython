/******************** (C) COPYRIGHT 2013 STMicroelectronics ********************
* File Name          : bluenrg_hci.c
* Author             : AMS - HEA&RF BU
* Version            : V1.0.0
* Date               : 4-Oct-2013
* Description        : File with HCI commands for BlueNRG FW6.0 and above.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#include <string.h>

#include "bluenrg_def.h"
#include "bluenrg_types.h"
#include "hci_const.h"
#include "bluenrg_aci_const.h"
#include "bluenrg_updater_aci.h"

#define MIN(a,b)            ((a) < (b) )? (a) : (b)
#define MAX(a,b)            ((a) > (b) )? (a) : (b)

tBleStatus aci_updater_start(void)
{
  struct hci_request_and_response rq;
  uint8_t status = 0;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_UPDATER_START);
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq); // No command complete is sent.

  return status;
}

tBleStatus aci_updater_reboot(void)
{
  struct hci_request_and_response rq;
  uint8_t status = 0;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_UPDATER_REBOOT);
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq); // No command complete is sent.

  return status;
}

tBleStatus aci_get_updater_version(uint8_t *version)
{
  struct hci_request_and_response rq;
  get_updater_version_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GET_UPDATER_VERSION);
  rq.rparam = &resp;
  rq.rlen = GET_UPDATER_VERSION_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  *version = resp.version;

  return resp.status;
}

tBleStatus aci_get_updater_buffer_size(uint8_t *buffer_size)
{
  struct hci_request_and_response rq;
  get_updater_bufsize_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_GET_UPDATER_BUFSIZE);
  rq.rparam = &resp;
  rq.rlen = GET_UPDATER_BUFSIZE_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  *buffer_size = resp.buffer_size;

  return resp.status;
}

tBleStatus aci_erase_blue_flag(void)
{
  struct hci_request_and_response rq;
  uint8_t status;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_UPDATER_ERASE_BLUE_FLAG);
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_reset_blue_flag(void)
{
  struct hci_request_and_response rq;
  uint8_t status;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_UPDATER_RESET_BLUE_FLAG);
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_updater_erase_sector(uint32_t address)
{
  struct hci_request_and_response rq;
  updater_erase_sector_cp cp;
  uint8_t status;

  cp.address = htobl(address);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_UPDATER_ERASE_SECTOR);
  rq.cparam = &cp;
  rq.clen = UPDATER_ERASE_SECTOR_CP_SIZE;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_updater_program_data_block(uint32_t address,
                                   uint16_t len,
                                   const uint8_t *data)
{
  struct hci_request_and_response rq;
  uint8_t status;
  updater_prog_data_block_cp cp;

  if( len > sizeof(cp.data))
    return BLE_STATUS_INVALID_PARAMS;

  cp.address = htobl(address);
  cp.data_len = htobs(len);
  memcpy(cp.data, data, len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_UPDATER_PROG_DATA_BLOCK);
  rq.cparam = &cp;
  rq.clen = UPDATER_PROG_DATA_BLOCK_CP_SIZE+len;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req_recv_rsp(&rq);

  return status;
}

tBleStatus aci_updater_read_data_block(uint32_t address,
                                uint16_t data_len,
                                uint8_t *data)
{
  struct hci_request_and_response rq;
  updater_read_data_block_cp cp;
  uint8_t buffer[HCI_MAX_PAYLOAD_SIZE];

  if((data_len+1) > HCI_MAX_PAYLOAD_SIZE)
    return BLE_STATUS_INVALID_PARAMS;

  cp.address = htobl(address);
  cp.data_len = htobs(data_len);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_UPDATER_READ_DATA_BLOCK);
  rq.cparam = &cp;
  rq.clen = UPDATER_READ_DATA_BLOCK_CP_SIZE;
  rq.rparam = buffer;
  rq.rlen = data_len + 1;

  hci_send_req_recv_rsp(&rq);

  // First byte is status
  memcpy(data, buffer+1, data_len);

  return buffer[0];
}

tBleStatus aci_updater_calc_crc(uint32_t address,
                         uint8_t num_sectors,
                         uint32_t *crc)
{
  struct hci_request_and_response rq;
  updater_calc_crc_cp cp;
  updater_calc_crc_rp resp;

  memset(&resp, 0, sizeof(resp));

  cp.address = htobl(address);
  cp.num_sectors = num_sectors;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_UPDATER_CALC_CRC);
  rq.cparam = &cp;
  rq.clen = UPDATER_CALC_CRC_CP_SIZE;
  rq.rparam = &resp;
  rq.rlen = UPDATER_CALC_CRC_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  *crc = btohl(resp.crc);

  return resp.status;
}

tBleStatus aci_updater_hw_version(uint8_t *version)
{
  struct hci_request_and_response rq;
  updater_hw_version_rp resp;

  memset(&resp, 0, sizeof(resp));

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_UPDATER_HW_VERSION);
  rq.rparam = &resp;
  rq.rlen = UPDATER_HW_VERSION_RP_SIZE;

  hci_send_req_recv_rsp(&rq);

  *version = resp.version;

  return resp.status;
}




