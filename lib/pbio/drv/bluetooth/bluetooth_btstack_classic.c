// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// This file defines the functions required to implement Pybricks' Bluetooth
// classic functionality using BTStack.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_CLASSIC

#include <pbdrv/bluetooth.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include <btstack.h>

#include <pbdrv/bluetooth.h>

static int32_t pending_inquiry_response_count;
static int32_t pending_inquiry_response_limit;
static void *pending_inquiry_result_handler_context;
static pbdrv_bluetooth_inquiry_result_handler_t pending_inquiry_result_handler;

static void handle_hci_event_packet(uint8_t *packet, uint16_t size);

void pbdrv_bluetooth_classic_handle_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            handle_hci_event_packet(packet, size);
    }
}

static void handle_hci_event_packet(uint8_t *packet, uint16_t size) {
    switch (hci_event_packet_get_type(packet)) {
        case GAP_EVENT_INQUIRY_RESULT: {
            if (!pending_inquiry_result_handler) {
                return;
            }
            pbdrv_bluetooth_inquiry_result_t result;
            gap_event_inquiry_result_get_bd_addr(packet, result.bdaddr);
            if (gap_event_inquiry_result_get_rssi_available(packet)) {
                result.rssi = gap_event_inquiry_result_get_rssi(packet);
            }
            if (gap_event_inquiry_result_get_name_available(packet)) {
                const uint8_t *name = gap_event_inquiry_result_get_name(packet);
                const size_t name_len = gap_event_inquiry_result_get_name_len(packet);
                snprintf(result.name, sizeof(result.name), "%.*s", (int)name_len, name);
            }
            result.class_of_device = gap_event_inquiry_result_get_class_of_device(packet);
            pending_inquiry_result_handler(pending_inquiry_result_handler_context, &result);
            if (pending_inquiry_response_limit > 0) {
                pending_inquiry_response_count++;
                if (pending_inquiry_response_count >= pending_inquiry_response_limit) {
                    gap_inquiry_stop();
                }
            }
            break;
        }
        case GAP_EVENT_INQUIRY_COMPLETE: {
            if (pending_inquiry_result_handler) {
                pending_inquiry_result_handler = NULL;
                pending_inquiry_result_handler_context = NULL;
                pbio_os_request_poll();
            }
            break;
        }
        default:
            break;
    }
}

void pbdrv_bluetooth_classic_init() {
    static btstack_packet_callback_registration_t hci_event_handler_registration;
    hci_event_handler_registration.callback = pbdrv_bluetooth_classic_handle_packet;
    hci_add_event_handler(&hci_event_handler_registration);
}

pbio_error_t pbdrv_bluetooth_inquiry_scan(
    pbio_os_state_t *state,
    int32_t max_responses,
    int32_t timeout,
    void *context,
    pbdrv_bluetooth_inquiry_result_handler_t result_handler) {
    PBIO_OS_ASYNC_BEGIN(state);
    if (pending_inquiry_result_handler) {
        return PBIO_ERROR_BUSY;
    }
    PBIO_OS_AWAIT_UNTIL(state, hci_get_state() == HCI_STATE_WORKING);
    pending_inquiry_response_count = 0;
    pending_inquiry_response_limit = max_responses;
    pending_inquiry_result_handler = result_handler;
    pending_inquiry_result_handler_context = context;
    gap_inquiry_start(timeout);

    PBIO_OS_AWAIT_UNTIL(state, !pending_inquiry_result_handler);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_CLASSIC
