/*
 * Copyright (c) 2018 David Lechner
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
