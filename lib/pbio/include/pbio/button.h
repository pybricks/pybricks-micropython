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
 * \addtogroup Button Brick and Remote Control Buttons
 *
 * Provides functions to test when a button is pressed.
 * @{
 */

#ifndef _PBIO_BUTTON_H_
#define _PBIO_BUTTON_H_

#include <pbio/error.h>
#include <pbio/port.h>

/**
 * Button flags.
 */
typedef enum _pbio_button_flags_t {
    PBIO_BUTTON_LEFT_DOWN  = 1 << 1, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Lefthand '-' button
                                      *   | EV3 brick            | *invalid*
                                      *   | EV3 IR remote        | Lefthand down button (red bar)
                                      *   | NXT brick            | *invalid*
                                      */
    PBIO_BUTTON_DOWN       = 1 << 2, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | *invalid*
                                      *   | EV3 brick            | Down button
                                      *   | EV3 IR remote        | *invalid*
                                      *   | NXT brick            | Dark gray button (reserved for system - not useable in user programs)
                                      */
    PBIO_BUTTON_RIGHT_DOWN = 1 << 3, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Righthand '-' button
                                      *   | EV3 brick            | *invalid*
                                      *   | EV3 IR remote        | Righthand down button (blue bar)
                                      *   | NXT brick            | *invalid*
                                      */
    PBIO_BUTTON_LEFT       = 1 << 4, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Lefthand red button
                                      *   | EV3 brick            | Left button
                                      *   | EV3 IR remote        | *invalid*
                                      *   | NXT brick            | Left arrow button
                                      */
    PBIO_BUTTON_CENTER     = 1 << 5, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | Green button
                                      *   | Powered UP remote    | Green button
                                      *   | EV3 brick            | Center button
                                      *   | EV3 IR remote        | *invalid*
                                      *   | NXT brick            | Orange button
                                      */
    PBIO_BUTTON_RIGHT      = 1 << 6, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Righthand red button
                                      *   | EV3 brick            | Right button
                                      *   | EV3 IR remote        | *invalid*
                                      *   | NXT brick            | Right arrow button
                                      */
    PBIO_BUTTON_LEFT_UP    = 1 << 7, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Lefthand '+' button
                                      *   | EV3 brick            | Back button (reserved for system - not useable in user programs)
                                      *   | EV3 IR remote        | Lefthand up button (red bar)
                                      *   | NXT brick            | *invalid*
                                      */
    PBIO_BUTTON_UP         = 1 << 8, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | *invalid*
                                      *   | EV3 brick            | Up button
                                      *   | EV3 IR remote        | Beacon
                                      *   | NXT brick            | *invalid*
                                      */
    PBIO_BUTTON_RIGHT_UP   = 1 << 9, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Righthand '+' button
                                      *   | EV3 brick            | *invalid*
                                      *   | EV3 IR remote        | Righthand up button (blue bar)
                                      *   | NXT brick            | *invalid*
                                      */

} pbio_button_flags_t;

/** @cond */
// calling pbdrv function directly for efficiency
#define pbio_button_is_pressed pbdrv_button_is_pressed

// include for pbdrv_button_is_pressed needs to be called after pbio_button_flags_t is defined
#include <pbdrv/button.h>
/** @endcond */

/**
 * Get bitmask indicating currently pressed buttons.
 * @param [in] port         The port to read
 * @param [out] pressed     Bitmask indicating which buttons are pressed
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_NO_DEV if port is valid but a device with buttons is not connected
 *                          ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_button_is_pressed(pbio_port_t port, pbio_button_flags_t *pressed);

#endif // _PBIO_BUTTON_H_

/** @}*/
