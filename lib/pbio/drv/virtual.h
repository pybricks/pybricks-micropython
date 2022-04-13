// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Common functions used by virtual (Python) drivers.

#ifndef _INTERNAL_PBDRV_VIRTUAL_H_
#define _INTERNAL_PBDRV_VIRTUAL_H_

#include <stdbool.h>

#include <pbio/error.h>

typedef struct _object PyObject;

/**
 * User-defined callback to handle exceptions from CPython.
 * @param [in]  type Borrowed ref to the exception type.
 * @param [in]  value Borrowed ref to the exception value.
 * @param [in]  traceback Borrowed ref to the exception stack trace.
 * @returns     true if the exception was "handled" and the unhanded exception
 *              message should be supressed or false to print an unhandled
 *              exception message.
 */
typedef bool (*pbdrv_virtual_cpython_exception_handler_t)(PyObject *type, PyObject *value, PyObject *traceback);

// REVISIT: these are high-level APIs and might need to be moved to a different header file
pbio_error_t pbdrv_virtual_platform_start(pbdrv_virtual_cpython_exception_handler_t handler);
pbio_error_t pbdrv_virtual_platform_stop(void);
pbio_error_t pbdrv_virtual_poll_events(void);

pbio_error_t pbdrv_virtual_call_method(const char *component, int index, const char *method, const char *format, ...);
pbio_error_t pbdrv_virtual_get_u8(const char *component, int index,  const char *attribute, uint8_t *value);
pbio_error_t pbdrv_virtual_get_u16(const char *component, int index, const char *attribute, uint16_t *value);
pbio_error_t pbdrv_virtual_get_u32(const char *component, int index, const char *attribute, uint32_t *value);
pbio_error_t pbdrv_virtual_get_i32(const char *component, int index, const char *attribute, int32_t *value);
pbio_error_t pbdrv_virtual_get_ctype_pointer(const char *component, int index, const char *attribute, void **value);

#endif // _INTERNAL_PBDRV_VIRTUAL_H_
