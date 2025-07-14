// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include "py/mpconfig.h"
#include "py/mperrno.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"

#include <pybricks/util_pb/pb_error.h>

int pb_errcode_from_pbio_error(pbio_error_t error) {
    switch (error) {
        case PBIO_SUCCESS:
            return 0;
        case PBIO_ERROR_FAILED:
            return MP_EFAULT;
        case PBIO_ERROR_INVALID_ARG:
            return MP_EINVAL;
        case PBIO_ERROR_NOT_IMPLEMENTED:
            return MP_ENOENT;
        case PBIO_ERROR_IO:
            return MP_EIO;
        case PBIO_ERROR_BUSY:
            return MP_EBUSY;
        case PBIO_ERROR_NO_DEV:
            return MP_ENODEV;
        case PBIO_ERROR_NOT_SUPPORTED:
            return MP_EOPNOTSUPP;
        case PBIO_ERROR_AGAIN:
            return MP_EAGAIN;
        case PBIO_ERROR_INVALID_OP:
            return MP_EPERM;
        case PBIO_ERROR_TIMEDOUT:
            return MP_ETIMEDOUT;
        case PBIO_ERROR_CANCELED:
            return MP_ECANCELED;
    }
    // This should never happen, but if it does, return a generic error.
    return MP_EFAULT;
}

/**
 * Raise an exception if @p error is not ::PBIO_SUCCESS. Most errors translate
 * to an OSError with the appropriate error code. There are a few special
 * cases that use another built-in python exception when it is more appropriate.
 */
void pb_assert(pbio_error_t error) {
    #if PYBRICKS_OPT_TERSE_ERR
    // using EINVAL to mean that the argument to this function was invalid.
    // since we raise ValueError for PBIO_ERROR_INVALID_ARG, there isn't a
    // possible conflict
    int os_err = MP_EINVAL;

    switch (error) {
        case PBIO_SUCCESS:
            return;
        case PBIO_ERROR_FAILED:
            mp_raise_msg(&mp_type_RuntimeError, NULL);
            __builtin_unreachable();
        case PBIO_ERROR_INVALID_ARG:
            mp_raise_ValueError(NULL);
            __builtin_unreachable();
        case PBIO_ERROR_NOT_IMPLEMENTED:
            mp_raise_NotImplementedError(NULL);
            __builtin_unreachable();
        case PBIO_ERROR_IO:
            os_err = MP_EIO;
            break;
        case PBIO_ERROR_BUSY:
            os_err = MP_EBUSY;
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
    __builtin_unreachable();
    #else // PYBRICKS_OPT_TERSE_ERR
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
        "The requested operation is not supported on this device:\n"
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
            mp_raise_msg(&mp_type_RuntimeError, (mp_rom_error_text_t)pbio_error_str(error));
            __builtin_unreachable();
        case PBIO_ERROR_INVALID_ARG:
            mp_raise_ValueError((mp_rom_error_text_t)pbio_error_str(error));
            __builtin_unreachable();
        case PBIO_ERROR_NOT_IMPLEMENTED:
            mp_raise_NotImplementedError((mp_rom_error_text_t)pbio_error_str(error));
            __builtin_unreachable();
        case PBIO_ERROR_IO:
            args[0] = MP_OBJ_NEW_SMALL_INT(MP_EIO);
            args[1] = MP_OBJ_FROM_PTR(&msg_io_obj);
            break;
        case PBIO_ERROR_BUSY:
            args[0] = MP_OBJ_NEW_SMALL_INT(MP_EBUSY);
            args[1] = MP_OBJ_NEW_QSTR(qstr_from_str(pbio_error_str(error)));
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
    __builtin_unreachable();
    #endif // PYBRICKS_OPT_TERSE_ERR
}
