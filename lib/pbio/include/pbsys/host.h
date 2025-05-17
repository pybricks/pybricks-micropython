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
void pbsys_host_rx_set_callback(pbsys_host_stdin_event_callback_t callback);
void pbsys_host_rx_flush(void);
uint32_t pbsys_host_rx_get_available(void);
uint32_t pbsys_host_rx_get_free(void);
void pbsys_host_rx_write(const uint8_t *data, uint32_t size);
pbio_error_t pbsys_host_rx(uint8_t *data, uint32_t *size);
pbio_error_t pbsys_host_tx(const uint8_t *data, uint32_t size);
bool pbsys_host_tx_is_idle(void);

#else // PBSYS_CONFIG_HOST

#define pbsys_host_init()
#define pbsys_host_rx_set_callback(callback)
#define pbsys_host_rx_flush()
#define pbsys_host_rx_get_available() 0
#define pbsys_host_rx_get_free() 0
#define pbsys_host_rx_write(data, size)

static inline pbio_error_t pbsys_host_rx(uint8_t *data, uint32_t *size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbsys_host_tx(const uint8_t *data, uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline bool pbsys_host_tx_is_idle(void) {
    return false;
}

#endif // PBSYS_CONFIG_HOST

#endif // _PBSYS_HOST_H_

/** @} */
