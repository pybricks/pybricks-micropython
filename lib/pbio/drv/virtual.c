// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Common functions used by virtual (Python) drivers.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_VIRTUAL

#include <stdio.h>

#include <Python.h>

/**
 * Gets the value of `_` in the `__main__` module.
 *
 * The GIL must be held when calling this function.
 *
 * @return A new reference to `_` or `NULL` on error.
 */
static PyObject *pbdrv_virtual_get_result(void) {
    // new ref
    PyObject *main = PyImport_ImportModule("__main__");
    if (!main) {
        return NULL;
    }

    // new ref
    PyObject *result = PyObject_GetAttrString(main, "_");

    Py_DECREF(main);

    return result;
}

/**
 * Gets the value of `hub.<property>` from the `__main__` module as an unsigned
 * long.
 *
 * @param [in]  property    The name of the property.
 * @return                  The value or `(unsigned long)-1` on error.
 */
unsigned long pbdrv_virtual_get_unsigned_long(const char *property) {
    unsigned long result = (unsigned long)-1;

    PyGILState_STATE state = PyGILState_Ensure();

    char buf[50];
    snprintf(buf, sizeof(buf), "_ = hub.%s\n", property);

    int ret = PyRun_SimpleString(buf);
    if (ret != 0) {
        goto out;
    }

    // new ref
    PyObject *result_obj = pbdrv_virtual_get_result();
    if (!result_obj) {
        goto out;
    }

    result = PyLong_AsUnsignedLong(result_obj);

    Py_DECREF(result_obj);

out:
    PyGILState_Release(state);

    return result;
}

#endif // PBDRV_CONFIG_VIRTUAL
