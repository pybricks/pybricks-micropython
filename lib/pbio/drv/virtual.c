// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Common functions used by virtual (CPython) drivers.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_VIRTUAL

#include <stdbool.h>
#include <stdio.h>

#include <Python.h>

#include <pbio/error.h>
#include "virtual.h"

#define CREATE_PLATFORM_OBJECT \
    "import importlib, os\n" \
    "platform_module_name = os.environ.get('PBIO_VIRTUAL_PLATFORM_MODULE', 'pbio_virtual.platform.default')\n" \
    "platform_module = importlib.import_module(platform_module_name)\n" \
    "platform = platform_module.Platform()\n"

static PyThreadState *thread_state;
static pbdrv_virtual_cpython_exception_handler_t cpython_exception_handler;

/**
 * Starts the CPython runtime and instantiates the virtual `platform` object.
 *
 * NOTE: This must be called before `pbdrv_init()`!
 *
 * When this function returns, the CPython runtime is intialized and the CPython
 * GIL is released.
 *
 * @param [in]  handler A callback to be called on an unhandled CPython exception or NULL.
 * @returns             ::PBIO_SUCCESS if
 */
pbio_error_t pbdrv_virtual_platform_start(pbdrv_virtual_cpython_exception_handler_t handler) {
    cpython_exception_handler = handler;

    // embedding Python to provide virtual hardware implementation
    Py_Initialize();

    if (PyRun_SimpleString(CREATE_PLATFORM_OBJECT) < 0) {
        return PBIO_ERROR_FAILED;
    }

    // release the GIL to allow pbio to run without blocking CPython
    thread_state = PyEval_SaveThread();

    return PBIO_SUCCESS;
}

/**
 * Stops the CPython runtime.
 *
 * NOTE: No other pbio functions may be called after calling this function!
 *
 * The CPython GIL must not be held when calling this function.
 *
 * @returns ::PBIO_SUCCESS if the runtime was stopped successfully, otherwise
 *          ::PBIO_ERROR_FAILED.
 */
pbio_error_t pbdrv_virtual_platform_stop(void) {
    PyEval_RestoreThread(thread_state);

    if (Py_FinalizeEx() < 0) {
        return PBIO_ERROR_FAILED;
    }

    return PBIO_SUCCESS;
}

/**
 * This checks for pending CPython exceptions and converts them to the
 * corresponding PBIO error code.
 *
 * NOTE: This function must be called with the GIL held!
 *
 * Exceptions with a `pbio_error` attribute that returns an int will be caught
 * and the value of `pbio_error` will be used as the return value of this
 * function.
 *
 * If a ::pbdrv_virtual_cpython_exception_handler_t was registered, it will
 * be called for any exception that does not meed the criteria above.
 *
 * If the handler returns false or there is no registered handler, the CPython
 * `sys.unraisablehook()` will be called, which by default prints the unhanded
 * exception to stderr.
 *
 * For any unhanded CPython exception without a `pbio_error` attribute, this
 * function will return ::PBIO_ERROR_FAILED.
 *
 * If there is no pending error, ::PBIO_SUCCESS is returned.
 *
 * @return  The error code.
 */
static pbio_error_t pbdrv_virtual_check_cpython_exception(void) {
    PyObject *type, *value, *traceback;

    // returns new references
    PyErr_Fetch(&type, &value, &traceback);

    // if type is NULL, it means that there is no pending error
    if (!type) {
        return PBIO_SUCCESS;
    }

    pbio_error_t err = PBIO_ERROR_FAILED;
    bool handled = false;

    // If the exception instance object has an attribute pbio_error with an
    // integer value, then use that for the err return value.

    // new reference
    PyObject *pbio_error = value ? PyObject_GetAttrString(value, "pbio_error") : NULL;

    if (pbio_error) {
        unsigned long value = PyLong_AsUnsignedLong(pbio_error);

        if (value != (unsigned long)-1 || !PyErr_Occurred()) {
            err = value;
            handled = true;
        }

        Py_DECREF(pbio_error);
    } else if (cpython_exception_handler) {
        handled = cpython_exception_handler(type, value, traceback);
    }

    // steals references
    PyErr_Restore(type, value, traceback);

    if (handled) {
        PyErr_Clear();
    } else {
        PyErr_WriteUnraisable(PyUnicode_FromString(
            "Virtual hub CPython exception: if this error is expected, catch it and raise PbioError instead."));
    }

    return err;
}

/**
 * Gets the value of `platform` in the `__main__` module.
 *
 * NOTE: The GIL must be held when calling this function!
 *
 * @return A new reference to `platform` or `NULL` on error.
 */
static PyObject *pbdrv_virtual_get_platform(void) {
    // new ref
    PyObject *main = PyImport_ImportModule("__main__");
    if (!main) {
        return NULL;
    }

    // new ref
    PyObject *result = PyObject_GetAttrString(main, "platform");

    Py_DECREF(main);

    return result;
}

/**
 * Calls a method on the `platform` object.
 *
 * Refer to https://docs.python.org/3/c-api/arg.html#c.Py_BuildValue for formatting.
 *
 * Note that for single args, the format string needs to include `()` so that
 * it returns a tuple.
 *
 * @param [in]  name    The name of the method.
 * @param [in]  fmt     The format of each argument.
 * @param [in]  ...     The values for @p fmt or NULL if there are no args.
 * @returns             ::PBIO_SUCCESS if the call was successful or an error
 *                      if there was a CPython exception.
 */
pbio_error_t pbdrv_virtual_platform_call_method(const char *name, const char *fmt, ...) {
    PyGILState_STATE state = PyGILState_Ensure();

    PyObject *platform = pbdrv_virtual_get_platform();

    if (!platform) {
        goto err;
    }

    // There is no va_list version of PyObject_CallMethod, so we have to get
    // the method and use Py_VaBuildValue() instead.

    // new reference
    PyObject *method = PyObject_GetAttrString(platform, name);

    if (!method) {
        goto err_unref_platform;
    }

    va_list va;
    va_start(va, fmt);
    // new reference
    PyObject *args = (!fmt || !*fmt) ? PyTuple_New(0) : Py_VaBuildValue(fmt, va);
    va_end(va);

    if (!args) {
        goto err_unref_method;
    }

    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError, "args must be a tuple");
        goto err_unref_args;
    }

    PyObject *ret = PyObject_Call(method, args, NULL);

    if (!ret) {
        goto err_unref_args;
    }

    // return value is ignored
    Py_DECREF(ret);

err_unref_args:
    Py_DECREF(args);
err_unref_method:
    Py_DECREF(method);
err_unref_platform:
    Py_DECREF(platform);

err:;
    pbio_error_t err = pbdrv_virtual_check_cpython_exception();

    PyGILState_Release(state);

    return err;
}

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
 * Gets the value of `platform.<property>` from the `__main__` module as a signed
 * long.
 *
 * @param [in]  property    The name of the property.
 * @return                  The value or `(long)-1` on error.
 */
unsigned long pbdrv_virtual_get_signed_long(const char *property) {
    long result = (long)-1;

    PyGILState_STATE state = PyGILState_Ensure();

    char buf[50];
    snprintf(buf, sizeof(buf), "_ = platform.%s\n", property);

    int ret = PyRun_SimpleString(buf);
    if (ret != 0) {
        goto out;
    }

    // new ref
    PyObject *result_obj = pbdrv_virtual_get_result();
    if (!result_obj) {
        goto out;
    }

    result = PyLong_AsLong(result_obj);

    Py_DECREF(result_obj);

out:
    PyGILState_Release(state);

    return result;
}

/**
 * Gets the value of `platform.<property>[<index>]` from the `__main__` module as a
 * signed long.
 *
 * @param [in]  property    The name of the property.
 * @param [in]  index       The index in the value returned by the property.
 * @return                  The value or `(long)-1` on error.
 */
unsigned long pbdrv_virtual_get_indexed_signed_long(const char *property, uint8_t index) {
    long result = (long)-1;

    PyGILState_STATE state = PyGILState_Ensure();

    char buf[50];
    snprintf(buf, sizeof(buf), "_ = platform.%s[%d]\n", property, index);

    int ret = PyRun_SimpleString(buf);
    if (ret != 0) {
        goto out;
    }

    // new ref
    PyObject *result_obj = pbdrv_virtual_get_result();
    if (!result_obj) {
        goto out;
    }

    result = PyLong_AsLong(result_obj);

    Py_DECREF(result_obj);

out:
    PyGILState_Release(state);

    return result;
}

/**
 * Gets the value of `platform.<property>` from the `__main__` module as an unsigned
 * long.
 *
 * @param [in]  property    The name of the property.
 * @return                  The value or `(unsigned long)-1` on error.
 */
unsigned long pbdrv_virtual_get_unsigned_long(const char *property) {
    unsigned long result = (unsigned long)-1;

    PyGILState_STATE state = PyGILState_Ensure();

    char buf[50];
    snprintf(buf, sizeof(buf), "_ = platform.%s\n", property);

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

/**
 * Gets the value of `platform.<property>[<index>]` from the `__main__` module as an
 * unsigned long.
 *
 * @param [in]  property    The name of the property.
 * @param [in]  index       The index in the value returned by the property.
 * @return                  The value or `(unsigned long)-1` on error.
 */
unsigned long pbdrv_virtual_get_indexed_unsigned_long(const char *property, uint8_t index) {
    unsigned long result = (unsigned long)-1;

    PyGILState_STATE state = PyGILState_Ensure();

    char buf[50];
    snprintf(buf, sizeof(buf), "_ = platform.%s[%d]\n", property, index);

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

/**
 * Calls `platform.on_event_poll()`.
 *
 * This should be called whenever the runtime is "idle".
 *
 * @returns ::PBIO_SUCCESS if there were no unhandled CPython exception or
 *          ::PBIO_ERROR_FAILED if there was an unhandled exception.
 */
pbio_error_t pbdrv_virtual_poll_events(void) {
    return pbdrv_virtual_platform_call_method("on_event_poll", NULL);
}

/**
 * Gets the value of `platform.<component>.<attribute>` or
 * `platform.<component>[<index>].<attribute>` from the `__main__`
 * module as an unsigned long value.
 *
 * @param [in]  component   The name of the component.
 * @param [in]  index       The index on component or -1 to not use an index.
 * @param [in]  attribute   The name of the attribute.
 * @return                  ::PBIO_SUCCESS or an error from a caught CPython exception.
 */
static pbio_error_t pbdrv_virtual_platform_get_unsigned_long(const char *component, int index, const char *attribute, unsigned long *value) {
    PyGILState_STATE state = PyGILState_Ensure();

    // new ref
    PyObject *platform = pbdrv_virtual_get_platform();

    if (!platform) {
        goto err;
    }

    // new ref
    PyObject *component_obj = PyObject_GetAttrString(platform, component);

    if (!component_obj) {
        goto err_unref_platform;
    }

    // if there is an index replace component_obj with the object at component_obj[index]
    if (index >= 0) {
        // new ref
        PyObject *index_obj = PyLong_FromLong(index);

        if (!index_obj) {
            goto err_unref_component;
        }

        // new ref
        PyObject *sub_component_obj = PyObject_GetItem(component_obj, index_obj);

        Py_DECREF(index_obj);

        if (!sub_component_obj) {
            goto err_unref_component;
        }

        // discard original component and steal the ref from sub_component_obj
        Py_DECREF(component_obj);
        component_obj = sub_component_obj;
    }

    // new ref
    PyObject *attr_obj = PyObject_GetAttrString(component_obj, attribute);

    if (!attr_obj) {
        goto err_unref_component;
    }

    *value = PyLong_AsUnsignedLong(attr_obj);

    Py_DECREF(attr_obj);
err_unref_component:
    Py_DECREF(component_obj);
err_unref_platform:
    Py_DECREF(platform);
err:;
    pbio_error_t err = pbdrv_virtual_check_cpython_exception();

    PyGILState_Release(state);

    return err;
}

/**
 * Gets the value of `platform.<component>.<attribute>` or
 * `platform.<component>[<index>].<attribute>` from the `__main__`
 * module as an unsigned 8-bit integer.
 *
 * @param [in]  component   The name of the component.
 * @param [in]  index       The index on component or -1 to not use an index.
 * @param [in]  attribute   The name of the attribute.
 * @param [out] value       The value read from CPython.
 * @return                  ::PBIO_SUCCESS or an error from a caught CPython exception.
 */
pbio_error_t pbdrv_virtual_get_u8(const char *component, int index, const char *attribute, uint8_t *value) {
    unsigned long long_value;
    pbio_error_t err = pbdrv_virtual_platform_get_unsigned_long(component, index, attribute, &long_value);
    *value = long_value;
    return err;
}

/**
 * Gets the value of `platform.<component>.<attribute>` or
 * `platform.<component>[<index>].<attribute>` from the `__main__`
 * module as an unsigned 16-bit integer.
 *
 * @param [in]  component   The name of the component.
 * @param [in]  index       The index on component or -1 to not use an index.
 * @param [in]  attribute   The name of the attribute.
 * @param [out] value       The value read from CPython.
 * @return                  ::PBIO_SUCCESS or an error from a caught CPython exception.
 */
pbio_error_t pbdrv_virtual_get_u16(const char *component, int index, const char *attribute, uint16_t *value) {
    unsigned long long_value;
    pbio_error_t err = pbdrv_virtual_platform_get_unsigned_long(component, index, attribute, &long_value);
    *value = long_value;
    return err;
}

/**
 * Gets the value of `platform.<component>.<attribute>` or
 * `platform.<component>[<index>].<attribute>` from the `__main__`
 * module as an unsigned 32-bit integer.
 *
 * @param [in]  component   The name of the component.
 * @param [in]  index       The index on component or -1 to not use an index.
 * @param [in]  attribute   The name of the attribute.
 * @param [out] value       The value read from CPython.
 * @return                  ::PBIO_SUCCESS or an error from a caught CPython exception.
 */
pbio_error_t pbdrv_virtual_get_u32(const char *component, int index, const char *attribute, uint32_t *value) {
    unsigned long long_value;
    pbio_error_t err = pbdrv_virtual_platform_get_unsigned_long(component, index, attribute, &long_value);
    *value = long_value;
    return err;
}
#endif // PBDRV_CONFIG_VIRTUAL
