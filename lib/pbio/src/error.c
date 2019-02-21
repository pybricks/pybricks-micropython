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
