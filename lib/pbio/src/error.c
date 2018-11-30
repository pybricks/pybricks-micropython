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

#include <stddef.h>

#include <pbio/error.h>

/**
 * Gets a string descirbing an error.
 * @param [in]  err     The error code
 * @return              A string describing the error or *NULL*
 */
const char *pbio_error_str(pbio_error_t err) {
    switch (err) {
    case PBIO_SUCCESS:
        break;
    case PBIO_ERROR_FAILED:
        return "Unknown error";
    case PBIO_ERROR_INVALID_ARG:
        return "Invalid argument";
    case PBIO_ERROR_INVALID_PORT:
        return "Invalid port";
    case PBIO_ERROR_IO:
        return "I/O error";
    case PBIO_ERROR_NO_DEV:
        return "Device not connected";
    case PBIO_ERROR_NOT_IMPLEMENTED:
        return "Not implemented";
    case PBIO_ERROR_NOT_SUPPORTED:
        return "Not supported";
    case PBIO_ERROR_AGAIN:
        return "Try again later";
    case PBIO_ERROR_INVALID_OP:
        return "Invalid operation";
    }

    return NULL;
}

/** @}*/
