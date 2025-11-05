// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

// Bluetooth driver using BlueKitchen BTStack for Classic Bluetooth.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_CLASSIC

#include "bluetooth_btstack_classic.h"

#include <math.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include <btstack.h>
#include <btstack_chipset_cc256x.h>
#include <hci_dump.h>
#include <hci_transport_h4.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/clock.h>

#include <pbio/error.h>
#include <pbio/os.h>

#include "bluetooth_init_cc2560x.h"
#include "../uart/uart_debug_first_port.h"


static void reset(void) {
}

static void log_packet(uint8_t packet_type, uint8_t in, uint8_t *packet, uint16_t len) {
    pbdrv_uart_debug_printf("HCI %s packet type: %02x, len: %u\n", in ? "in" : "out", packet_type, len);
}

static void log_message(int log_level, const char *format, va_list argptr) {
    pbdrv_uart_debug_vprintf(format, argptr);
    pbdrv_uart_debug_printf("\n");
}

const hci_dump_t bluetooth_btstack_classic_hci_dump = {
    .reset = reset,
    .log_packet = log_packet,
    .log_message = log_message,
};

static const pbdrv_bluetooth_btstack_platform_data_t *pdata = &pbdrv_bluetooth_btstack_platform_data;

// note on baud rate: with a 48MHz clock, 3000000 baud is the highest we can
// go with LL_USART_OVERSAMPLING_16. With LL_USART_OVERSAMPLING_8 we could go
// to 4000000, which is the max rating of the CC2564C.
static const hci_transport_config_uart_t hci_transport_config = {
    .type = HCI_TRANSPORT_CONFIG_UART,
    .baudrate_init = 115200,
    .baudrate_main = 115200,
    .flowcontrol = 1,
    .device_name = NULL,
};

typedef struct {
    uint8_t status;
    uint8_t hci_version;
    uint16_t hci_revision;
    uint8_t lmp_pal_version;
    uint16_t manufacturer;
    uint16_t lmp_pal_subversion;
} pbdrv_bluetooth_local_version_info_t;

// Pending request datastructures. There is a common pattern to how we handle
// all of the operations in this file.
//
// * The caller *sets* the pending request variable or handler.
// * The packet handler *fills* the pending request variable or calls the handler.
// * The packet handler *clears* the pending request variable or handler to indicate operation completion.
static pbdrv_bluetooth_local_version_info_t *pending_local_version_info_request;
static int32_t pending_inquiry_response_count;
static int32_t pending_inquiry_response_limit;
static void *pending_inquiry_result_handler_context;
static pbdrv_bluetooth_inquiry_result_handler_t pending_inquiry_result_handler;

static void pbdrv_bluetooth_btstack_classic_handle_hci_event_packet(uint8_t *packet, uint16_t size);

static void pbdrv_bluetooth_btstack_classic_handle_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            pbdrv_bluetooth_btstack_classic_handle_hci_event_packet(packet, size);
    }
}

static void pbdrv_bluetooth_btstack_classic_handle_hci_event_packet(uint8_t *packet, uint16_t size) {
    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_COMMAND_COMPLETE: {
            const uint8_t *rp = hci_event_command_complete_get_return_parameters(packet);
            switch (hci_event_command_complete_get_command_opcode(packet)) {
                case HCI_OPCODE_HCI_READ_LOCAL_VERSION_INFORMATION: {
                    pbdrv_bluetooth_local_version_info_t *ret = pending_local_version_info_request;
                    if (!ret) {
                        return;
                    }
                    ret->status = rp[0];
                    ret->hci_version = rp[1];
                    ret->hci_revision = rp[2] | ((uint16_t)rp[3] << 8);
                    ret->lmp_pal_version = rp[4];
                    ret->manufacturer = rp[5] | ((uint16_t)rp[6] << 8);
                    ret->lmp_pal_subversion = rp[7] | ((uint16_t)rp[8] << 8);
                    pending_local_version_info_request = NULL;
                    pbio_os_request_poll();
                    return;
                }
                default:
                    break;
            }
            break;
        }
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
                strncpy(result.name, (const char *)name, name_len);
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

bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
    // TODO: implement classic connection check
    return false;
}

/**
 * btstack's hci_power_control() synchronously emits an event that would cause
 * it to re-enter the event loop. This would not be safe to call from within
 * the event loop. This wrapper ensures it is called at most once.
 */
static pbio_error_t bluetooth_btstack_classic_handle_power_control(pbio_os_state_t *state, HCI_POWER_MODE power_mode, HCI_STATE end_state) {

    bool do_it_this_time = false;

    PBIO_OS_ASYNC_BEGIN(state);

    do_it_this_time = true;
    PBIO_OS_ASYNC_SET_CHECKPOINT(state);

    // The first time we get here, do_it_this_time = true, so we call
    // hci_power_control. When it re-enters at the checkpoint above, it will
    // be false, so move on.
    if (do_it_this_time) {
        hci_power_control(power_mode);
    }

    // Wait for the power state to take effect.
    PBIO_OS_AWAIT_UNTIL(state, hci_get_state() == end_state);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

const char *pbdrv_bluetooth_get_hub_name(void) {
    // REVISIT: this should be linked to the init script as it can be updated in software
    return "Pybricks Hub";
}

// Reads the local version information from the Bluetooth controller.
pbio_error_t pbdrv_bluetooth_read_local_version_information(pbio_os_state_t *state, pbdrv_bluetooth_local_version_info_t *out) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_AWAIT_UNTIL(state, hci_get_state() == HCI_STATE_WORKING);
    pending_local_version_info_request = out;
    hci_send_cmd(&hci_read_local_version_information);
    PBIO_OS_AWAIT_UNTIL(state, !pending_local_version_info_request);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
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

static void bluetooth_btstack_classic_run_loop_init(void) {
    // Not used. Bluetooth process is started like a regular pbdrv process.
}

static btstack_linked_list_t data_sources;

static void bluetooth_btstack_classic_run_loop_add_data_source(btstack_data_source_t *ds) {
    btstack_linked_list_add(&data_sources, &ds->item);
}

static bool bluetooth_btstack_classic_run_loop_remove_data_source(btstack_data_source_t *ds) {
    return btstack_linked_list_remove(&data_sources, &ds->item);
}

static void bluetooth_btstack_classic_run_loop_enable_data_source_callbacks(btstack_data_source_t *ds, uint16_t callback_types) {
    ds->flags |= callback_types;
}

static void bluetooth_btstack_classic_run_loop_disable_data_source_callbacks(btstack_data_source_t *ds, uint16_t callback_types) {
    ds->flags &= ~callback_types;
}

static btstack_linked_list_t timers;

static void bluetooth_btstack_classic_run_loop_set_timer(btstack_timer_source_t *ts, uint32_t timeout_in_ms) {
    ts->timeout = pbdrv_clock_get_ms() + timeout_in_ms;
}

static void bluetooth_btstack_classic_run_loop_add_timer(btstack_timer_source_t *ts) {
    btstack_linked_item_t *it;
    for (it = (void *)&timers; it->next; it = it->next) {
        // don't add timer that's already in there
        btstack_timer_source_t *next = (void *)it->next;
        if (next == ts) {
            // timer was already in the list!
            // assert(0);
            return;
        }
        // exit if new timeout before list timeout
        int32_t delta = btstack_time_delta(ts->timeout, next->timeout);
        if (delta < 0) {
            break;
        }
    }

    ts->item.next = it->next;
    it->next = &ts->item;
}

static bool bluetooth_btstack_classic_run_loop_remove_timer(btstack_timer_source_t *ts) {
    if (btstack_linked_list_remove(&timers, &ts->item)) {
        return true;
    }
    return false;
}

static void bluetooth_btstack_classic_run_loop_execute(void) {
    // not used
}

static void bluetooth_btstack_classic_run_loop_dump_timer(void) {
    // not used
}

static const btstack_run_loop_t bluetooth_btstack_classic_run_loop = {
    .init = bluetooth_btstack_classic_run_loop_init,
    .add_data_source = bluetooth_btstack_classic_run_loop_add_data_source,
    .remove_data_source = bluetooth_btstack_classic_run_loop_remove_data_source,
    .enable_data_source_callbacks = bluetooth_btstack_classic_run_loop_enable_data_source_callbacks,
    .disable_data_source_callbacks = bluetooth_btstack_classic_run_loop_disable_data_source_callbacks,
    .set_timer = bluetooth_btstack_classic_run_loop_set_timer,
    .add_timer = bluetooth_btstack_classic_run_loop_add_timer,
    .remove_timer = bluetooth_btstack_classic_run_loop_remove_timer,
    .execute = bluetooth_btstack_classic_run_loop_execute,
    .dump_timer = bluetooth_btstack_classic_run_loop_dump_timer,
    .get_time_ms = pbdrv_clock_get_ms,
};

static bool do_poll_handler;

void pbdrv_bluetooth_btstack_classic_run_loop_trigger(void) {
    do_poll_handler = true;
    pbio_os_request_poll();
}

static pbio_os_process_t pbdrv_bluetooth_hci_process;

static pbio_os_state_t bluetooth_thread_state;
static pbio_os_state_t bluetooth_thread_err;
pbio_error_t pbdrv_bluetooth_process_thread(pbio_os_state_t *state, void *context);

/**
 * This process is slightly unusual in that it does not have a state. It is
 * essentially just a poll handler.
 */
static pbio_error_t pbdrv_bluetooth_hci_process_thread(pbio_os_state_t *state, void *context) {

    if (do_poll_handler) {
        do_poll_handler = false;

        btstack_data_source_t *ds, *next;
        for (ds = (void *)data_sources; ds != NULL; ds = next) {
            // cache pointer to next data_source to allow data source to remove itself
            next = (void *)ds->item.next;
            if (ds->flags & DATA_SOURCE_CALLBACK_POLL) {
                ds->process(ds, DATA_SOURCE_CALLBACK_POLL);
            }
        }
    }

    static pbio_os_timer_t btstack_timer = {
        .duration = 1,
    };

    if (pbio_os_timer_is_expired(&btstack_timer)) {
        pbio_os_timer_extend(&btstack_timer);

        // process all BTStack timers in list that have expired
        while (timers) {
            btstack_timer_source_t *ts = (void *)timers;
            int32_t delta = btstack_time_delta(ts->timeout, pbdrv_clock_get_ms());
            if (delta > 0) {
                // we have reached unexpired timers
                break;
            }
            bluetooth_btstack_classic_run_loop_remove_timer(ts);
            ts->process(ts);
        }
    }

    // No events to propagate in this classic-only implementation for now.
    if (bluetooth_thread_err == PBIO_ERROR_AGAIN) {
        bluetooth_thread_err = pbdrv_bluetooth_process_thread(&bluetooth_thread_state, NULL);
    }

    return PBIO_ERROR_AGAIN;
}


void pbdrv_bluetooth_init(void) {
    pbdrv_uart_debug_printf("Starting btstack classic ...\n");

    // Here we only initialize the HCI transport, btstack memory, and the run
    // loop, and kick off the hci process thread. The rest of initialization
    // moves forward in pbdrv_bluetooth_process_thread. Note that the bluetooth
    // thread is driven by the HCI thread.
    // hci_dump_init(&bluetooth_btstack_classic_hci_dump);
    btstack_memory_init();
    btstack_run_loop_init(&bluetooth_btstack_classic_run_loop);
    hci_init(hci_transport_h4_instance_for_uart(pdata->uart_block_instance()), &hci_transport_config);
    hci_set_control(pdata->control_instance());
    bluetooth_thread_err = PBIO_ERROR_AGAIN;
    pbio_os_process_start(&pbdrv_bluetooth_hci_process, pbdrv_bluetooth_hci_process_thread, NULL);
}

const char *pbdrv_bluetooth_get_fw_version(void) {
    // REVISIT: this should be linked to the init script as it can be updated in software
    // init script version
    return "v1.4";
}

static bool shutting_down;
static btstack_packet_callback_registration_t pbdrv_bluetooth_btstack_packet_handler_reg = {
    .item = { NULL },
    .callback = pbdrv_bluetooth_btstack_classic_handle_packet,
};

static pbio_error_t pbdrv_bluetooth_controller_reset(pbio_os_state_t *state, pbio_os_timer_t *timer) {
    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    // TODO: Disconnect classic connection if connected.

    // Wait for power off.
    PBIO_OS_AWAIT(state, &sub, bluetooth_btstack_classic_handle_power_control(&sub, HCI_POWER_OFF, HCI_STATE_OFF));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_error_t pbdrv_bluetooth_controller_initialize(pbio_os_state_t *state, pbio_os_timer_t *timer) {
    static pbio_os_state_t sub;

    // The first time we init the classic chip, we don't know what LMP
    // subversion the chip is (could be cc2560 or cc2560a). We must read the
    // subversion from the chip itself, then select the appropriate init script.
    static uint16_t lmp_subversion = 0;

    PBIO_OS_ASYNC_BEGIN(state);

    hci_add_event_handler(&pbdrv_bluetooth_btstack_packet_handler_reg);
    pbdrv_uart_debug_printf("Starting btstack classic HCI...\n");

    if (lmp_subversion == 0) {
        static pbdrv_bluetooth_local_version_info_t version_info;
        // We must power-on the bluetooth chip before
        pbdrv_uart_debug_printf("Powering on the chip to read version info...\n");
        PBIO_OS_AWAIT(state, &sub, bluetooth_btstack_classic_handle_power_control(&sub, HCI_POWER_ON, HCI_STATE_INITIALIZING));
        pbdrv_uart_debug_printf("Chip is on, reading version info...\n");
        PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_read_local_version_information(&sub, &version_info));
        lmp_subversion = version_info.lmp_pal_subversion;
        pbdrv_uart_debug_printf("Detected LMP Subversion: 0x%04x\n", lmp_subversion);
        // Power down the chip -- we'll power it up again later with the correct init script.
        PBIO_OS_AWAIT(state, &sub, bluetooth_btstack_classic_handle_power_control(&sub, HCI_POWER_OFF, HCI_STATE_OFF));
        pbdrv_uart_debug_printf("Power is off\n", lmp_subversion);
    }

    pbdrv_bluetooth_init_script_t init_script;
    pbio_error_t err = pbdrv_bluetooth_get_init_script(lmp_subversion, &init_script);
    if (err != PBIO_SUCCESS) {
        pbdrv_uart_debug_printf("Unsupported LMP Subversion: 0x%04" PRIx16 "\n", lmp_subversion);
        return err;
    }
    btstack_chipset_cc256x_set_init_script((uint8_t *)init_script.script, init_script.script_size);
    hci_set_chipset(pdata->chipset_instance());

    // TODO: add packet handler for classic events
    // hci_add_event_handler(...)

    l2cap_init();

    hci_set_inquiry_mode(0x01);  // Enable RSSI responses for inquiry scans.

    // TODO: add classic-specific initializations (SDP, RFCOMM, etc.)
    bluetooth_thread_err = PBIO_ERROR_AGAIN;
    pbdrv_uart_debug_printf("HCI process started.\n");

    // Wait for power on.
    PBIO_OS_AWAIT(state, &sub, bluetooth_btstack_classic_handle_power_control(&sub, HCI_POWER_ON, HCI_STATE_WORKING));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}


pbio_error_t pbdrv_bluetooth_process_thread(pbio_os_state_t *state, void *context) {
    static pbio_os_state_t sub;
    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_uart_debug_printf("Reset BT controller\n");

    // Reset and initialize the controller.
    PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_controller_reset(&sub, &timer));

    pbdrv_uart_debug_printf("Initialize BT controller\n");
    PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_controller_initialize(&sub, &timer));

    pbdrv_uart_debug_printf("BT controller initialized. Awaiting shutdown.\n");

    // Service scheduled tasks as long as Bluetooth is enabled.
    while (!shutting_down) {
        // TODO: Add SPP connection/data handling.
        PBIO_OS_AWAIT_MS(state, &timer, 100);
    }

    // Power down the chip.
    PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_controller_reset(&sub, &timer));

    hci_remove_event_handler(&pbdrv_bluetooth_btstack_packet_handler_reg);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_bluetooth_deinit(void) {
    shutting_down = true;
    pbio_os_request_poll();
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_CLASSIC
