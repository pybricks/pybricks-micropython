#include <pberror.h>

/**
 * Raise an exception if *error* is not *PBIO_SUCCESS*
 */
void pb_assert(pbio_error_t error) {
    switch (error) {
        case PBIO_SUCCESS:
            break;
        case PBIO_ERROR_FAILED:
            mp_raise_msg(&mp_type_RuntimeError, "Unknown error");
            break;
        case PBIO_ERROR_INVALID_ARG:
            mp_raise_ValueError("Invalid argument");
            break;
        case PBIO_ERROR_INVALID_PORT:
            mp_raise_ValueError("Invalid port");
            break;
        case PBIO_ERROR_IO:
            mp_raise_msg(&mp_type_OSError, "I/O error");
            break;
        case PBIO_ERROR_NO_DEV:
            mp_raise_msg(&mp_type_RuntimeError, "Device is not connected");
            break;
        case PBIO_ERROR_NOT_IMPLEMENTED:
            mp_raise_msg(&mp_type_RuntimeError, "This feature is not yet implemented on this device");
            break;
        case PBIO_ERROR_NOT_SUPPORTED:
            mp_raise_msg(&mp_type_RuntimeError, "This feature is not supported on this device");
            break;
        case PBIO_ERROR_AGAIN:
            mp_raise_msg(&mp_type_RuntimeError, "Try again later");
            break;
    }
}
