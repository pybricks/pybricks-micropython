// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

/**
 * @addtogroup SystemHost System: Host
 * @{
 */

#ifndef _PBSYS_HOST_H_
#define _PBSYS_HOST_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/error.h>
#include <pbio/os.h>
#include <pbsys/config.h>

/**
 * Callback function to handle stdin events.
 * @param [in]  c   the character received
 * @return          *true* if the character was handled and should not be placed
 *                  in the stdin buffer, otherwise *false*.
 */
typedef bool (*pbsys_host_stdin_event_callback_t)(uint8_t c);

typedef enum {
    PBSYS_HOST_TRANSPORT_TYPE_BLUETOOTH,
    PBSYS_HOST_TRANSPORT_TYPE_USB,
} pbsys_host_transport_type_t;

#if PBSYS_CONFIG_HOST

void pbsys_host_debug_print(const char *data, size_t len);
bool pbsys_host_get_event_buf(pbsys_host_transport_type_t transport, uint8_t **buf, uint32_t **len);
void pbsys_host_init(void);
bool pbsys_host_is_connected(void);
void pbsys_host_schedule_status_update(const uint8_t *buf);
uint32_t pbsys_host_stdin_get_free(void);
void pbsys_host_stdin_write(const uint8_t *data, uint32_t size);
void pbsys_host_stdin_set_callback(pbsys_host_stdin_event_callback_t callback);
void pbsys_host_stdin_flush(void);
uint32_t pbsys_host_stdin_get_available(void);
pbio_error_t pbsys_host_stdin_read(uint8_t *data, uint32_t *size);
pbio_error_t pbsys_host_stdout_write(const uint8_t *data, uint32_t *size);
bool pbsys_host_tx_is_idle(void);
pbio_error_t pbsys_host_send_app_data(pbio_os_state_t *state, const uint8_t *data, size_t size);
void pbsys_host_app_data_clear_pending(void);
void pbsys_host_connection_changed(void);

#else // PBSYS_CONFIG_HOST

#endif // PBSYS_CONFIG_HOST

#endif // _PBSYS_HOST_H_

/** @} */
