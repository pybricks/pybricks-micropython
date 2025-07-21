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
#include <pbsys/config.h>

/**
 * Callback function to handle stdin events.
 * @param [in]  c   the character received
 * @return          *true* if the character was handled and should not be placed
 *                  in the stdin buffer, otherwise *false*.
 */
typedef bool (*pbsys_host_stdin_event_callback_t)(uint8_t c);

#if PBSYS_CONFIG_HOST

void pbsys_host_init(void);
bool pbsys_host_is_connected(void);
uint32_t pbsys_host_stdin_get_free(void);
void pbsys_host_stdin_write(const uint8_t *data, uint32_t size);
void pbsys_host_stdin_set_callback(pbsys_host_stdin_event_callback_t callback);
void pbsys_host_stdin_flush(void);
uint32_t pbsys_host_stdin_get_available(void);
pbio_error_t pbsys_host_stdin_read(uint8_t *data, uint32_t *size);
pbio_error_t pbsys_host_stdout_write(const uint8_t *data, uint32_t *size);
bool pbsys_host_tx_is_idle(void);

#else // PBSYS_CONFIG_HOST

#define pbsys_host_init()
#define pbsys_host_is_connected() false
#define pbsys_host_stdin_get_free() 0
#define pbsys_host_stdin_write(data, size) { (void)(data); (void)(size); }
#define pbsys_host_stdin_set_callback(callback) { (void)(callback); }
#define pbsys_host_stdin_flush()
#define pbsys_host_stdin_get_available() 0
#define pbsys_host_stdin_read(data, size) PBIO_ERROR_NOT_SUPPORTED
#define pbsys_host_stdout_write(data, size) { (void)(data); (void)(size); PBIO_ERROR_NOT_SUPPORTED; }
#define pbsys_host_tx_is_idle() false

#endif // PBSYS_CONFIG_HOST

#endif // _PBSYS_HOST_H_

/** @} */
