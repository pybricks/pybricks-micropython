/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David Lechner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * \addtogroup Error Error handling
 * @{
 */


#ifndef _PBIO_ERROR_H_
#define _PBIO_ERROR_H_

/**
 * Error code.
 */
typedef enum {
    PBIO_SUCCESS,               /**< No error */
    PBIO_ERROR_FAILED,          /**< Unspecified error (used when no other error code fits) */
    PBIO_ERROR_INVALID_ARG,     /**< Invalid argument (other than port) */
    PBIO_ERROR_INVALID_PORT,    /**< Invalid port identifier (special case of ::PBIO_ERROR_INVALID_ARG) */
    PBIO_ERROR_IO,              /**< General I/O error */
    PBIO_ERROR_NO_DEV,          /**< Device is not connected */
    PBIO_ERROR_NOT_IMPLEMENTED, /**< Feature is not yet implemented */
    PBIO_ERROR_NOT_SUPPORTED,   /**< Feature is not supported on this device */
    PBIO_ERROR_AGAIN,           /**< Function should be called again later */
    PBIO_ERROR_INVALID_OP,      /**< Operation is not permitted in the current state */
} pbio_error_t;

const char *pbio_error_str(pbio_error_t err);

#endif // _PBIO_ERROR_H_

/** @}*/
