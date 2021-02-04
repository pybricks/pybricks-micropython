// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

// Bluetooth driver using BlueKitchen BTStack with TI CC256x chip and STM32F4 MCU.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_STM32_CC264X

#include <ble/gatt-service/nordic_spp_service_server.h>
#include <btstack_chipset_cc256x.h>
#include <btstack.h>

#include <pbdrv/bluetooth.h>

#include "bluetooth_btstack_control_gpio.h"
#include "bluetooth_btstack_run_loop_contiki.h"
#include "bluetooth_btstack_uart_block_stm32_hal.h"
#include "genhdr/pybricks_service.h"

typedef struct {
    const uint8_t *data;
    uint8_t size;
    pbdrv_bluetooth_uart_tx_done_t done;
} uart_tx_context_t;

static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static btstack_packet_callback_registration_t hci_event_callback_registration;
static pbdrv_bluetooth_on_event_t bluetooth_on_event;
static pbdrv_bluetooth_uart_on_rx_t uart_on_rx;

const uint8_t adv_data[] = {
    // Flags general discoverable, BR/EDR not supported
    2, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // UUID ...
    17, BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS,
    0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89, 0xda, 0x46, 0x80, 0x82, 0x01, 0x00, 0xf5, 0xc5,
};

_Static_assert(sizeof(adv_data) <= 31, "31 octect max");

const uint8_t scan_resp_data[] = {
    // Name
    13, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'P', 'y', 'b', 'r', 'i', 'c', 'k', 's', ' ', 'H', 'u', 'b',
};

_Static_assert(sizeof(scan_resp_data) <= 31, "31 octect max");

// note on baud rate: with a 48MHz clock, 3000000 baud is the highest we can
// go with LL_USART_OVERSAMPLING_16. With LL_USART_OVERSAMPLING_8 we could go
// to 4000000, which is the max rating of the CC2564C.

static const hci_transport_config_uart_t config = {
    .type = HCI_TRANSPORT_CONFIG_UART,
    .baudrate_init = 115200,
    .baudrate_main = 3000000,
    .flowcontrol = 1,
    .device_name = NULL,
};

static void nordic_can_send(void *context) {
    uart_tx_context_t *uart_tx = context;

    nordic_spp_service_server_send(con_handle, uart_tx->data, uart_tx->size);
    uart_tx->done();
}

static void nordic_data_received(hci_con_handle_t tx_con_handle, const uint8_t *data, uint16_t size) {
    if (size == 0 && con_handle == HCI_CON_HANDLE_INVALID) {
        con_handle = tx_con_handle;
    } else {
        if (uart_on_rx) {
            uart_on_rx(data, size);
        }
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            con_handle = HCI_CON_HANDLE_INVALID;
            break;
        default:
            break;
    }
    if (bluetooth_on_event) {
        bluetooth_on_event();
    }
}

void pbdrv_bluetooth_init(void) {
    btstack_memory_init();
    btstack_run_loop_init(pbdrv_bluetooth_btstack_run_loop_contiki_get_instance());

    hci_init(hci_transport_h4_instance(pbdrv_bluetooth_btstack_uart_block_stm32_hal_instance()), &config);
    hci_set_chipset(btstack_chipset_cc256x_instance());
    hci_set_control(pbdrv_bluetooth_btstack_control_gpio_instance());

    // REVISIT: do we need to call btstack_chipset_cc256x_set_power() or btstack_chipset_cc256x_set_power_vector()?

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    l2cap_init();

    // setup LE device DB
    le_device_db_init();

    // setup SM: Display only
    sm_init();

    // setup ATT server
    att_server_init(profile_data, NULL, NULL);

    // setup Nordic SPP service
    nordic_spp_service_server_init(&nordic_data_received);
}

void pbdrv_bluetooth_power_on(bool on) {
    hci_power_control(on ? HCI_POWER_ON : HCI_POWER_OFF);
}

bool pbdrv_bluetooth_is_ready(void) {
    return hci_get_state() != HCI_STATE_OFF;
}

static void init_advertising_data(void) {
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;
    bd_addr_t null_addr = { 0 };
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(adv_data), (uint8_t *)adv_data);
    gap_scan_response_set_data(sizeof(scan_resp_data), (uint8_t *)scan_resp_data);
}

void pbdrv_bluetooth_start_advertising(void) {
    init_advertising_data();
    gap_advertisements_enable(true);
}

bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
    if ((connection & PBDRV_BLUETOOTH_CONNECTION_UART) && con_handle != HCI_CON_HANDLE_INVALID) {
        return true;
    }
    return false;
}

void pbdrv_bluetooth_set_on_event(pbdrv_bluetooth_on_event_t on_event) {
    bluetooth_on_event = on_event;
}

void pbdrv_bluetooth_uart_tx(const uint8_t *data, uint8_t size, pbdrv_bluetooth_uart_tx_done_t done) {
    static btstack_context_callback_registration_t send_request;
    static uart_tx_context_t uart_tx;

    uart_tx.data = data;
    uart_tx.size = size;
    uart_tx.done = done;
    send_request.callback = &nordic_can_send;
    send_request.context = &uart_tx;
    nordic_spp_service_server_request_can_send_now(&send_request, con_handle);
}

void pbdrv_bluetooth_uart_set_on_rx(pbdrv_bluetooth_uart_on_rx_t on_rx) {
    uart_on_rx = on_rx;
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_STM32_CC264X
