// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <pberror.h>

#include "py/mperrno.h"
#include "py/runtime.h"

/**
 * Raise an exception if *error* is not *PBIO_SUCCESS*. Most errors translate
 * to an OSError with the appropriate error code. There are a few special
 * cases that use another built-in python exception when it is more appropriate.
 */
void pb_assert(pbio_error_t error) {
    // using EINVAL to mean that the argument to this function was invalid.
    // since we raise ValueError for PBIO_ERROR_INVALID_ARG, there isn't a
    // possible conflict
    int os_err = MP_EINVAL;

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
        os_err = MP_EIO;
        break;
    case PBIO_ERROR_NO_DEV:
#if MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE
        mp_raise_msg(&mp_type_OSError, pbio_error_str(error));
#else
        mp_raise_msg(&mp_type_OSError, 
                "\n\n"
               "A sensor or motor is not connected to the specified port:\n"
               "--> Check the cables to each motor and sensor.\n"
               "--> Check the port settings in your script.\n"
               "--> Check the line in your script that matches\n"
               "    the line number given in the 'Traceback' above."
               "\n\n"
        );
#endif
        return;
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
#if MICROPY_PY_BUILTINS_TIMEOUTERROR
        mp_raise_msg(&mp_type_TimeoutError, NULL);
        return;
#else
        os_err = MP_ETIMEDOUT;
        break;
#endif
    case PBIO_ERROR_CANCELED:
        os_err = MP_ECANCELED;
        break;
    }

    mp_raise_OSError(os_err);
}

void pb_assert_type(mp_obj_t obj, const mp_obj_type_t *type) {
    if (!mp_obj_is_type(obj, type)) {
        #if MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE
        mp_raise_TypeError(NULL);
        #else
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
            "can't convert %s to %s", mp_obj_get_type_str(obj), qstr_str(type->name)));
        #endif
    }
}
