

#ifndef _PBIO_ERROR_H_
#define _PBIO_ERROR_H_

/**
 * \addtogroup Error Error handling
 * @{
 */

/**
 * Error code.
 */
typedef enum {
    PBIO_SUCCESS,               /**< No error */
    PBIO_ERROR_FAILED,          /**< Unspecified error */
    PBIO_ERROR_INVALID_ARG,     /**< Invalid argument */
    PBIO_ERROR_INVALID_PORT,    /**< Invalid port identifier */
    PBIO_ERROR_IO,              /**< General I/O error */
    PBIO_ERROR_NO_DEV,          /**< Device is not connected */
    PBIO_ERROR_NOT_IMPLEMENTED, /**< This feature is not yet implemented on this device */
    PBIO_ERROR_NOT_SUPPORTED    /**< This feature is not supported on this device */
} pbio_error_t;

/** @}*/

#endif // _PBIO_ERROR_H_
