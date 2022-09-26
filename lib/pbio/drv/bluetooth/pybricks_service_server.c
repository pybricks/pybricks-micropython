// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

/*
 * Based on nordic_spp_service_server.c from BTStack
 *
 * Copyright (C) 2018 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK

#define BTSTACK_FILE__ "pybricks_service_server.c"

#include <stdint.h>
#include <string.h>

#include <pbsys/app.h>
#include <pbio/math.h>
#include <pbio/protocol.h>

#include "btstack_defines.h"
#include "ble/att_db.h"
#include "ble/att_server.h"
#include "btstack_util.h"
#include "bluetooth_gatt.h"
#include "btstack_debug.h"

#include "pybricks_service_server.h"

static att_service_handler_t pybricks_service;
static pybricks_characteristic_write_callback_t client_callback;
static pybricks_characteristic_configuration_callback_t client_configuration_callback;

static uint16_t pybricks_command_event_value_handle;
static uint16_t pybricks_command_event_client_configuration_handle;
static uint16_t pybricks_command_event_client_configuration_value;
static uint16_t pybricks_hub_capabilities_value_handle;

// TODO: need a way to reset pybricks_command_event_client_configuration_value on disconnect

static uint16_t pybricks_service_read_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(con_handle);
    UNUSED(offset);

    if (attribute_handle == pybricks_command_event_client_configuration_handle) {
        if (buffer) {
            little_endian_store_16(buffer, 0, pybricks_command_event_client_configuration_value);
        }
        return 2;
    }

    if (attribute_handle == pybricks_hub_capabilities_value_handle) {
        if (buffer && buffer_size >= PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE) {
            pbio_pybricks_hub_capabilities(buffer, pbio_math_min(att_server_get_mtu(con_handle) - 3, 512),
                PBSYS_APP_HUB_FEATURE_FLAGS, PBSYS_APP_USER_PROGRAM_SIZE);
        }
        return PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE;
    }

    return 0;
}

static int pybricks_service_write_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);
    UNUSED(offset);

    if (attribute_handle == pybricks_command_event_value_handle) {
        client_callback(con_handle, &buffer[0], buffer_size);
    }
    else if (attribute_handle == pybricks_command_event_client_configuration_handle) {
        pybricks_command_event_client_configuration_value = little_endian_read_16(buffer, 0);
        client_configuration_callback(con_handle, pybricks_command_event_client_configuration_value);
    }

    return 0;
}

void pybricks_service_server_init(
    pybricks_characteristic_write_callback_t write_callback,
    pybricks_characteristic_configuration_callback_t configuration_callback) {

    btstack_assert(write_callback);
    btstack_assert(configuration_callback);

    client_callback = write_callback;
    client_configuration_callback = configuration_callback;

    // get service handle range
    uint16_t start_handle = 0;
    uint16_t end_handle = 0xffff;
    int service_found = gatt_server_get_get_handle_range_for_service_with_uuid128(pbio_pybricks_service_uuid, &start_handle, &end_handle);
    btstack_assert(service_found != 0);
    UNUSED(service_found);

    // get characteristic value and descriptor handles
    pybricks_command_event_value_handle = gatt_server_get_value_handle_for_characteristic_with_uuid128(start_handle, end_handle, pbio_pybricks_command_event_char_uuid);
    pybricks_command_event_client_configuration_handle = gatt_server_get_client_configuration_handle_for_characteristic_with_uuid128(start_handle, end_handle, pbio_pybricks_command_event_char_uuid);
    pybricks_hub_capabilities_value_handle = gatt_server_get_value_handle_for_characteristic_with_uuid128(start_handle, end_handle, pbio_pybricks_hub_capabilities_char_uuid);

    log_info("pybricks_command_event_value_handle 0x%02x", pybricks_command_event_value_handle);
    log_info("pybricks_command_event_client_configuration_handle 0x%02x", pybricks_command_event_client_configuration_handle);

    // register service with ATT Server
    pybricks_service.start_handle = start_handle;
    pybricks_service.end_handle = end_handle;
    pybricks_service.read_callback = &pybricks_service_read_callback;
    pybricks_service.write_callback = &pybricks_service_write_callback;
    att_server_register_service_handler(&pybricks_service);
}

/**
 * @brief Queue send request. When called, one packet can be send via pybricks_service_send below
 * @param request
 * @param con_handle
 */
void pybricks_service_server_request_can_send_now(btstack_context_callback_registration_t *request, hci_con_handle_t con_handle) {
    att_server_request_to_send_notification(request, con_handle);
}

/**
 * @brief Send data
 * @param con_handle
 * @param data
 * @param size
 */
int pybricks_service_server_send(hci_con_handle_t con_handle, const uint8_t *data, uint16_t size) {
    return att_server_notify(con_handle, pybricks_command_event_value_handle, data, size);
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK
