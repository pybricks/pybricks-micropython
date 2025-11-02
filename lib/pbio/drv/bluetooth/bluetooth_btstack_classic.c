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

// Storage for last read-local-version result so it can be inspected later.
static volatile bool version_pending;
static volatile bool version_done;
typedef struct {
    uint8_t status;
    uint8_t hci_version;
    uint16_t hci_revision;
    uint8_t lmp_pal_version;
    uint16_t manufacturer;
    uint16_t lmp_pal_subversion;
} pbdrv_bluetooth_local_version_info_t;
static pbdrv_bluetooth_local_version_info_t *version_out;

static void bluetooth_classic_hello_hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }

    if (hci_event_packet_get_type(packet) != HCI_EVENT_COMMAND_COMPLETE) {
        return;
    }

    if (hci_event_command_complete_get_command_opcode(packet) != HCI_OPCODE_HCI_READ_LOCAL_VERSION_INFORMATION) {
        return;
    }

    const uint8_t *rp = hci_event_command_complete_get_return_parameters(packet);
    // return parameters layout (spec): status, hci_version, hci_revision (le, 2), lmp_pal_version,
    // manufacturer (le, 2), lmp_pal_subversion (le, 2)
    if (version_out) {
        version_out->status = rp[0];
        version_out->hci_version = rp[1];
        version_out->hci_revision = rp[2] | ((uint16_t)rp[3] << 8);
        version_out->lmp_pal_version = rp[4];
        version_out->manufacturer = rp[5] | ((uint16_t)rp[6] << 8);
        version_out->lmp_pal_subversion = rp[7] | ((uint16_t)rp[8] << 8);
    }

    version_done = true;
    // Wake PBIO poll loop so awaiting threads are resumed.
    pbio_os_request_poll();
}

static btstack_packet_callback_registration_t bluetooth_classic_hello_hci_handler_reg;

/**
 * PBIO async function that sends HCI Read Local Version Information and waits
 * for the command complete event. The parsed result is written to `out`.
 */
pbio_error_t pbdrv_bluetooth_read_local_version_information(pbio_os_state_t *state, pbdrv_bluetooth_local_version_info_t *out) {
    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_uart_debug_printf("Reading local version information...\n");

    // Ensure HCI is up first.
    PBIO_OS_AWAIT_UNTIL(state, hci_get_state() == HCI_STATE_WORKING);

    // Prepare to receive the command-complete event.
    version_done = false;
    version_out = out;
    bluetooth_classic_hello_hci_handler_reg.callback = &bluetooth_classic_hello_hci_event_handler;
    hci_add_event_handler(&bluetooth_classic_hello_hci_handler_reg);
    version_pending = true;

    pbdrv_uart_debug_printf("HCI is working, sending command...\n");

    // Send the HCI command.
    hci_send_cmd(&hci_read_local_version_information);

    pbdrv_uart_debug_printf("Command sent, awaiting response...\n");

    // Wait for the event handler to set hello_done.
    PBIO_OS_AWAIT_UNTIL(state, version_done == true);

    pbdrv_uart_debug_printf("Command finished.\n");

    // Clean up handler registration.
    hci_remove_event_handler(&bluetooth_classic_hello_hci_handler_reg);
    version_pending = false;
    version_out = NULL;

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
    hci_dump_init(&bluetooth_btstack_classic_hci_dump);
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

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_bluetooth_deinit(void) {
    shutting_down = true;
    pbio_os_request_poll();
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_CLASSIC
