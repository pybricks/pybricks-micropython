// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Bluetooth driver using BlueKitchen BTStack with TI CC256x chip and STM32F4 MCU.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_STM32_CC264X

#include <ble/gatt-service/nordic_spp_service_server.h>
#include <btstack_chipset_cc256x.h>
#include <btstack.h>
#include <contiki.h>

#include <pbio/event.h>
#include <pbsys/status.h>

#include "../src/processes.h"
#include "bluetooth_btstack_control_gpio.h"
#include "bluetooth_btstack_run_loop_contiki.h"
#include "bluetooth_btstack_uart_block_stm32_hal.h"
#include "genhdr/pybricks_service.h"

// max data size for nRF UART characteristics
#define NRF_CHAR_SIZE 20

static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static btstack_context_callback_registration_t send_request;
static btstack_packet_callback_registration_t hci_event_callback_registration;
PROCESS(pbdrv_bluetooth_hci_process, "Bluetooth HCI");

// nRF UART tx is in progress
static bool uart_tx_busy;
// buffer to queue UART tx data
static uint8_t uart_tx_buf[NRF_CHAR_SIZE];
// bytes used in uart_tx_buf
static uint8_t uart_tx_buf_size;

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

static const hci_transport_config_uart_t config = {
    .type = HCI_TRANSPORT_CONFIG_UART,
    .baudrate_init = 115200,
    .baudrate_main = 115200,
    // FIXME: Fails to receive valid data after baud rate change if we have a different rate here.
    // ENABLE_CC256X_BAUDRATE_CHANGE_FLOWCONTROL_BUG_WORKAROUND doesn't seem to make a difference
    // .baudrate_main = 921600,
    .flowcontrol = 1,
    .device_name = NULL,
};

pbio_error_t pbdrv_bluetooth_tx(uint8_t c) {
    // make sure we have a Bluetooth connection
    if (con_handle == HCI_CON_HANDLE_INVALID) {
        return PBIO_ERROR_INVALID_OP;
    }

    // can't add the the buffer if tx is in progress or buffer is full
    if (uart_tx_busy || uart_tx_buf_size == NRF_CHAR_SIZE) {
        return PBIO_ERROR_AGAIN;
    }

    // append the char
    uart_tx_buf[uart_tx_buf_size] = c;
    uart_tx_buf_size++;

    // poke the process to start tx soon-ish. This way, we can accumulate up to
    // NRF_CHAR_SIZE bytes before actually transmitting
    // TODO: it probably would be better to poll here instead of using events
    // so that we don't fill up the event queue with 20 events
    process_post(&pbdrv_bluetooth_hci_process, PROCESS_EVENT_MSG, NULL);

    return PBIO_SUCCESS;
}

static void nordic_can_send(void *context) {
    nordic_spp_service_server_send(con_handle, uart_tx_buf, uart_tx_buf_size);
    uart_tx_busy = false;
    uart_tx_buf_size = 0;
}

static void uart_rx_char_modified(const uint8_t *data, uint8_t size) {
    for (int i = 0; i < size; i++) {
        // TODO: set .port = bluetooth port
        pbio_event_uart_rx_data_t rx = { .byte = data[i] };
        process_post_synch(&pbsys_process, PBIO_EVENT_UART_RX, &rx);
    }
}

static void nordic_data_received(hci_con_handle_t tx_con_handle, const uint8_t *data, uint16_t size) {
    if (size == 0 && con_handle == HCI_CON_HANDLE_INVALID) {
        con_handle = tx_con_handle;
    } else {
        uart_rx_char_modified(data, size);
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

    process_poll(&pbdrv_bluetooth_hci_process);
}

void pbdrv_bluetooth_init() {
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

    // setup advertisements
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;
    bd_addr_t null_addr = { 0 };
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(adv_data), (uint8_t *)adv_data);
    gap_scan_response_set_data(sizeof(scan_resp_data), (uint8_t *)scan_resp_data);

    process_start(&pbdrv_bluetooth_hci_process, NULL);
}

PROCESS_THREAD(pbdrv_bluetooth_hci_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    for (;;) {
        // make sure the Bluetooth chip is in reset long enough to actually reset
        etimer_set(&timer, clock_from_msec(10000));
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

        // take Bluetooth chip out of reset
        hci_power_control(HCI_POWER_ON);

        // TODO: we should have a timeout and stop scanning eventually
        pbsys_status_set(PBSYS_STATUS_BLE_ADVERTISING);
        gap_advertisements_enable(true);
        PROCESS_WAIT_UNTIL(con_handle != HCI_CON_HANDLE_INVALID);
        gap_advertisements_enable(false);
        pbsys_status_clear(PBSYS_STATUS_BLE_ADVERTISING);

        etimer_set(&timer, clock_from_msec(500));

        // con_handle is set to HCI_CON_HANDLE_INVALID upon disconnection
        while (con_handle != HCI_CON_HANDLE_INVALID) {
            PROCESS_WAIT_EVENT();
            if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
                etimer_reset(&timer);
                // just occasionally checking to see if we are still connected
                continue;
            }
            if (ev == PROCESS_EVENT_MSG && uart_tx_buf_size) {
                uart_tx_busy = true;
                send_request.callback = &nordic_can_send;
                nordic_spp_service_server_request_can_send_now(&send_request, con_handle);
                PROCESS_WAIT_WHILE(uart_tx_busy);
            }

        }

        // reset Bluetooth chip
        hci_power_control(HCI_POWER_OFF);
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_STM32_CC264X
