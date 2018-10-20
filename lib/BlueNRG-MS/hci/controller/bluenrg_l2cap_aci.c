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

#include "bluenrg_types.h"
#include "bluenrg_def.h"
#include "hci_const.h"
#include "bluenrg_aci_const.h"
#include "bluenrg_l2cap_aci.h"
#include "bluenrg_hal_aci.h"
#include "bluenrg_gap.h"

#define MIN(a,b)            ((a) < (b) )? (a) : (b)
#define MAX(a,b)            ((a) > (b) )? (a) : (b)

tBleStatus aci_l2cap_connection_parameter_update_request(uint16_t conn_handle, uint16_t interval_min,
                                                         uint16_t interval_max, uint16_t slave_latency,
                                                         uint16_t timeout_multiplier)
{
  struct hci_request rq;
  uint8_t status;
  l2cap_conn_param_update_req_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.interval_min = htobs(interval_min);
  cp.interval_max = htobs(interval_max);
  cp.slave_latency = htobs(slave_latency);
  cp.timeout_multiplier = htobs(timeout_multiplier);

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_L2CAP_CONN_PARAM_UPDATE_REQ);
  rq.cparam = &cp;
  rq.clen = L2CAP_CONN_PARAM_UPDATE_REQ_CP_SIZE;
  rq.event = EVT_CMD_STATUS;
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req(&rq);

  return status;
}

tBleStatus aci_l2cap_connection_parameter_update_response(uint16_t conn_handle, uint16_t interval_min,
                                                         uint16_t interval_max, uint16_t slave_latency,
                                                         uint16_t timeout_multiplier, uint16_t min_ce_length, uint16_t max_ce_length,
                             uint8_t id, uint8_t accept)
{
  struct hci_request rq;
  uint8_t status;
  l2cap_conn_param_update_resp_cp cp;

  cp.conn_handle = htobs(conn_handle);
  cp.interval_min = htobs(interval_min);
  cp.interval_max = htobs(interval_max);
  cp.slave_latency = htobs(slave_latency);
  cp.timeout_multiplier = htobs(timeout_multiplier);
  cp.min_ce_length = htobs(min_ce_length);
  cp.max_ce_length = htobs(max_ce_length);
  cp.id = id;
  cp.accept = accept;

  memset(&rq, 0, sizeof(rq));
  rq.opcode = cmd_opcode_pack(OGF_VENDOR_CMD, OCF_L2CAP_CONN_PARAM_UPDATE_RESP);
  rq.cparam = &cp;
  rq.clen = sizeof(cp);
  rq.rparam = &status;
  rq.rlen = 1;

  hci_send_req(&rq);

  return status;
}
