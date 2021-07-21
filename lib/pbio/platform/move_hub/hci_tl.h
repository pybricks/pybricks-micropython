/******************** (C) COPYRIGHT 2016 STMicroelectronics ********************
* File Name          : hci_tl_template.h
* Author             : AMG RF FW team
* Version            : V1.1.0
* Date               : 18-July-2016
* Description        : Header file for framework required for handling HCI interface.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#ifndef _HCI_TL_TEMPLATE_H_
#define _HCI_TL_TEMPLATE_H_

#include <stdint.h>

#include "bluenrg_types.h"
#include "bluenrg_conf.h"

struct hci_request_and_response {
    uint32_t event;   /**< HCI Event */
    void *cparam;     /**< HCI Command from MCU to Host */
    void *rparam;     /**< Response from Host to MCU */
    uint16_t opcode;  /**< Opcode */
    uint8_t clen;     /**< Command Length */
    uint8_t rlen;     /**< Response Length */
};

struct hci_request {
    void *cparam;     /**< HCI Command from MCU to Host */
    uint16_t opcode;  /**< Opcode */
    uint8_t clen;     /**< Command Length */
};

struct hci_response {
    void *rparam;     /**< Response from Host to MCU */
    uint8_t rlen;     /**< Response Length */
};

void hci_send_req_recv_rsp(struct hci_request_and_response *r);

void hci_send_req(struct hci_request *r);
void hci_recv_resp(struct hci_response *r);

#endif /* _HCI_TL_TEMPLATE_H_ */
