// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <unistd.h>
#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

typedef struct _pbio_serial_t pbio_serial_t;

#if PBIO_CONFIG_SERIAL

pbio_error_t pbio_serial_get(pbio_serial_t **_ser, pbio_port_t port, int baudrate, int timeout);

pbio_error_t pbio_serial_write(pbio_serial_t *ser, const void *buf, size_t count);

pbio_error_t pbio_serial_in_waiting(pbio_serial_t *ser, size_t *waiting);

pbio_error_t pbio_serial_read(pbio_serial_t *ser, uint8_t *buf, size_t count);

pbio_error_t pbio_serial_clear(pbio_serial_t *ser);

#else // PBIO_CONFIG_SERIAL

static inline pbio_error_t pbio_serial_get(pbio_serial_t **_ser, pbio_port_t port, int baudrate, int timeout) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_serial_write(pbio_serial_t *ser, const void *buf, size_t count) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_serial_in_waiting(pbio_serial_t *ser, size_t *waiting) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_serial_read(pbio_serial_t *ser, uint8_t *buf, size_t count) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_serial_clear(pbio_serial_t *ser) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBIO_CONFIG_SERIAL
