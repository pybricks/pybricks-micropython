// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Internal common bluetooth functions.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_H_
#define _INTERNAL_PBDRV_BLUETOOTH_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH

#include <pbio/os.h>

#include <pbdrv/bluetooth.h>

#include <stdbool.h>
#include <stdint.h>

#include <lwrb/lwrb.h>

#define PBDRV_BLUETOOTH_STATUS_UPDATE_INTERVAL (500)
#define PBDRV_BLUETOOTH_MAX_CHAR_SIZE 20
#define PBDRV_BLUETOOTH_MAX_ADV_SIZE 31

void pbdrv_bluetooth_init_hci(void);
void pbdrv_bluetooth_controller_reset_hard(void);
pbio_error_t pbdrv_bluetooth_controller_reset(pbio_os_state_t *state, pbio_os_timer_t *timer);
pbio_error_t pbdrv_bluetooth_controller_initialize(pbio_os_state_t *state, pbio_os_timer_t *timer);

pbio_error_t pbdrv_bluetooth_start_broadcasting_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_disconnect_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_start_advertising_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_stop_advertising_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_start_observing_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_stop_observing_func(pbio_os_state_t *state, void *context);

pbio_error_t pbdrv_bluetooth_send_pybricks_value_notification(pbio_os_state_t *state, const uint8_t *data, uint16_t size);

extern pbdrv_bluetooth_receive_handler_t pbdrv_bluetooth_receive_handler;

extern pbdrv_bluetooth_peripheral_t peripheral_singleton;

extern uint16_t pbdrv_bluetooth_char_write_handle;
extern uint8_t pbdrv_bluetooth_char_write_data[PBDRV_BLUETOOTH_MAX_CHAR_SIZE];
extern size_t pbdrv_bluetooth_char_write_size;

extern uint8_t pbdrv_bluetooth_broadcast_data[PBDRV_BLUETOOTH_MAX_ADV_SIZE];
extern uint8_t pbdrv_bluetooth_broadcast_data_size;

extern bool pbdrv_bluetooth_is_broadcasting;
extern bool pbdrv_bluetooth_is_observing;
extern pbdrv_bluetooth_start_observing_callback_t pbdrv_bluetooth_observe_callback;

pbio_error_t pbdrv_bluetooth_process_thread(pbio_os_state_t *state, void *context);

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _INTERNAL_PBDRV_BLUETOOTH_H_
