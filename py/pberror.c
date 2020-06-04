// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pberror.h>

#include "py/mpconfig.h"

#include "py/mperrno.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"

/**
 * Raise an exception if *error* is not *PBIO_SUCCESS*. Most errors translate
 * to an OSError with the appropriate error code. There are a few special
 * cases that use another built-in python exception when it is more appropriate.
 */
void pb_assert(pbio_error_t error) {
    #if MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE
    // using EINVAL to mean that the argument to this function was invalid.
    // since we raise ValueError for PBIO_ERROR_INVALID_ARG, there isn't a
    // possible conflict
    int os_err = MP_EINVAL;

    switch (error) {
        case PBIO_SUCCESS:
            return;
        case PBIO_ERROR_FAILED:
            mp_raise_msg(&mp_type_RuntimeError, NULL);
            return;
        case PBIO_ERROR_INVALID_ARG:
        case PBIO_ERROR_INVALID_PORT:
            mp_raise_ValueError(NULL);
            return;
        case PBIO_ERROR_NOT_IMPLEMENTED:
            mp_raise_NotImplementedError(NULL);
            return;
        case PBIO_ERROR_IO:
            os_err = MP_EIO;
            break;
        case PBIO_ERROR_NO_DEV:
            os_err = MP_ENODEV;
            break;
        case PBIO_ERROR_NOT_SUPPORTED:
            os_err = MP_EOPNOTSUPP;
            break;
        case PBIO_ERROR_AGAIN:
            os_err = MP_EAGAIN;
            break;
        case PBIO_ERROR_INVALID_OP:
            os_err = MP_EPERM;
            break;
        case PBIO_ERROR_TIMEDOUT:
            os_err = MP_ETIMEDOUT;
            break;
        case PBIO_ERROR_CANCELED:
            os_err = MP_ECANCELED;
            break;
    }

    mp_raise_OSError(os_err);
    #else // MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE
    static const MP_DEFINE_STR_OBJ(msg_io_obj, "\n\n"
        "Unexpected hardware input/output error with a motor or sensor:\n"
        "--> Try unplugging the sensor or motor and plug it back in again.\n"
        "--> To see which sensor or motor is causing the problem,\n"
        "    check the line in your script that matches\n"
        "    the line number given in the 'Traceback' above.\n"
        "--> Try rebooting the hub/brick if the problem persists.\n"
        "\n");
    static const MP_DEFINE_STR_OBJ(msg_no_dev_obj, "\n\n"
        "A sensor or motor is not connected to the specified port:\n"
        "--> Check the cables to each motor and sensor.\n"
        "--> Check the port settings in your script.\n"
        "--> Check the line in your script that matches\n"
        "    the line number given in the 'Traceback' above.\n"
        "\n");
    static const MP_DEFINE_STR_OBJ(msg_not_supported_obj, "\n\n"
        "The requested operation is not support on this device:\n"
        "--> Check the documentation for device compatibility.\n"
        "--> Check the line in your script that matches\n"
        "    the line number given in the 'Traceback' above.\n"
        "\n");
    static const MP_DEFINE_STR_OBJ(msg_invalid_op_obj, "\n\n"
        "The requested operation is not valid in the current state:\n"
        "--> Check the documentation for required conditions.\n"
        "--> Check the line in your script that matches\n"
        "    the line number given in the 'Traceback' above.\n"
        "\n");

    mp_obj_t args[2];

    switch (error) {
        case PBIO_SUCCESS:
            return;
        case PBIO_ERROR_FAILED:
            mp_raise_msg(&mp_type_RuntimeError, pbio_error_str(error));
            return;
        case PBIO_ERROR_INVALID_ARG:
        case PBIO_ERROR_INVALID_PORT:
            mp_raise_ValueError(pbio_error_str(error));
            return;
        case PBIO_ERROR_NOT_IMPLEMENTED:
            mp_raise_NotImplementedError(pbio_error_str(error));
            return;
        case PBIO_ERROR_IO:
            args[0] = MP_OBJ_NEW_SMALL_INT(MP_EIO);
            args[1] = MP_OBJ_FROM_PTR(&msg_io_obj);
            break;
        case PBIO_ERROR_NO_DEV:
            args[0] = MP_OBJ_NEW_SMALL_INT(MP_ENODEV);
            args[1] = MP_OBJ_FROM_PTR(&msg_no_dev_obj);
            break;
        case PBIO_ERROR_NOT_SUPPORTED:
            args[0] = MP_OBJ_NEW_SMALL_INT(MP_EOPNOTSUPP);
            args[1] = MP_OBJ_FROM_PTR(&msg_not_supported_obj);
            break;
        case PBIO_ERROR_AGAIN:
            args[0] = MP_OBJ_NEW_SMALL_INT(MP_EAGAIN);
            args[1] = MP_OBJ_NEW_QSTR(qstr_from_str(pbio_error_str(error)));
            break;
        case PBIO_ERROR_INVALID_OP:
            args[0] = MP_OBJ_NEW_SMALL_INT(MP_EPERM);
            args[1] = MP_OBJ_FROM_PTR(&msg_invalid_op_obj);
            break;
        case PBIO_ERROR_TIMEDOUT:
            args[0] = MP_OBJ_NEW_SMALL_INT(MP_ETIMEDOUT);
            args[1] = MP_OBJ_NEW_QSTR(qstr_from_str(pbio_error_str(error)));
            break;
        case PBIO_ERROR_CANCELED:
            args[0] = MP_OBJ_NEW_SMALL_INT(MP_ECANCELED);
            args[1] = MP_OBJ_NEW_QSTR(qstr_from_str(pbio_error_str(error)));
            break;
    }

    nlr_raise(mp_obj_new_exception_args(&mp_type_OSError, 2, args));
    #endif // MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE
}

void pb_assert_type(mp_obj_t obj, const mp_obj_type_t *type) {
    if (!mp_obj_is_type(obj, type)) {
        #if MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE
        mp_raise_TypeError(NULL);
        #else
        mp_raise_msg_varg(&mp_type_TypeError, "can't convert %s to %s",
            mp_obj_get_type_str(obj), qstr_str(type->name));
        #endif
    }
}
