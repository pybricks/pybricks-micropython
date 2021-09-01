// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

/*
 * Based on pybricks_service_server.c from BTStack
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
#ifndef PYBRICKS_SERVICE_SERVER_H
#define PYBRICKS_SERVICE_SERVER_H

#include <stdint.h>

#include "bluetooth.h"
#include "btstack_defines.h"

typedef void (*pybricks_characteristic_write_callback_t)(hci_con_handle_t con_handle, const uint8_t *data, uint16_t size);
typedef void (*pybricks_characteristic_configuration_callback_t)(hci_con_handle_t con_handle, uint16_t value);

/**
 * @brief Init Pybricks Service Server with ATT DB
 * @param write_callback called when a connected device writes to any Pybricks service characteristic
 * @param configuration_callback called when a connected device writes to any Pybricks service control characteristic configuration attribute
 */
void pybricks_service_server_init(
    pybricks_characteristic_write_callback_t write_callback,
    pybricks_characteristic_configuration_callback_t configuration_callback);

/**
 * @brief Queue send request. When called, one packet can be send via pybricks_service_send below
 * @param request
 * @param con_handle
 */
void pybricks_service_server_request_can_send_now(btstack_context_callback_registration_t *request, hci_con_handle_t con_handle);

/**
 * @brief Send data
 * @param con_handle
 * @param data
 * @param size
 */
int pybricks_service_server_send(hci_con_handle_t con_handle, const uint8_t *data, uint16_t size);

#endif // PYBRICKS_SERVICE_SERVER_H
