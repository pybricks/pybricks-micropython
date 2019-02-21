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
 * \addtogroup IOPortDriver I/O Port I/O driver
 * @{
 */

#ifndef _PBDRV_IOPORT_H_
#define _PBDRV_IOPORT_H_

#include "pbdrv/config.h"
#include "pbio/error.h"
#include "pbio/iodev.h"
#include "pbio/port.h"
#include "sys/process.h"

#if PBDRV_CONFIG_IOPORT

pbio_error_t pbdrv_ioport_get_iodev(pbio_port_t port, pbio_iodev_t **iodev);

/** @cond INTERNAL */

PROCESS_NAME(pbdrv_ioport_process);

/** @endcond */

#else // PBDRV_CONFIG_IOPORT

static inline pbio_error_t pbdrv_ioport_get_iodev(pbio_port_t port, pbio_iodev_t **iodev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_IOPORT

#endif // _PBDRV_IOPORT_H_

/** @}*/
