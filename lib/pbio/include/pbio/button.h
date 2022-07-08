// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

/**
 * @addtogroup Button pbio: Brick and Remote Control Buttons
 *
 * Provides functions to test when a button is pressed.
 * @{
 */

#ifndef _PBIO_BUTTON_H_
#define _PBIO_BUTTON_H_

#include <pbio/error.h>
#include <pbio/port.h>

/** The number of buttons. */
#define PBIO_BUTTON_NUM_BUTTONS 9

/**
 * Button flags.
 */
typedef enum _pbio_button_flags_t {
    PBIO_BUTTON_LEFT_DOWN  = 1 << 1, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Lefthand '-' button
                                      *   | EV3 Brick            | *invalid*
                                      *   | EV3 IR remote        | Lefthand down button (red bar)
                                      *   | NXT brick            | *invalid*
                                      */
    PBIO_BUTTON_DOWN       = 1 << 2, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | *invalid*
                                      *   | EV3 Brick            | Down button
                                      *   | EV3 IR remote        | *invalid*
                                      *   | NXT brick            | Dark gray button (reserved for system - not useable in user programs)
                                      */
    PBIO_BUTTON_RIGHT_DOWN = 1 << 3, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Righthand '-' button
                                      *   | EV3 Brick            | *invalid*
                                      *   | EV3 IR remote        | Righthand down button (blue bar)
                                      *   | NXT brick            | *invalid*
                                      */
    PBIO_BUTTON_LEFT       = 1 << 4, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Lefthand red button
                                      *   | SPIKE Prime hub      | Left button
                                      *   | EV3 Brick            | Left button
                                      *   | EV3 IR remote        | *invalid*
                                      *   | NXT brick            | Left arrow button
                                      */
    PBIO_BUTTON_CENTER     = 1 << 5, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | Green button
                                      *   | Powered UP remote    | Green button
                                      *   | SPIKE Prime hub      | Center button
                                      *   | EV3 Brick            | Center button
                                      *   | EV3 IR remote        | *invalid*
                                      *   | NXT brick            | Orange button
                                      */
    PBIO_BUTTON_RIGHT      = 1 << 6, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Righthand red button
                                      *   | SPIKE Prime hub      | Right button
                                      *   | EV3 Brick            | Right button
                                      *   | EV3 IR remote        | *invalid*
                                      *   | NXT brick            | Right arrow button
                                      */
    PBIO_BUTTON_LEFT_UP    = 1 << 7, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Lefthand '+' button
                                      *   | EV3 Brick            | Back button (reserved for system - not useable in user programs)
                                      *   | EV3 IR remote        | Lefthand up button (red bar)
                                      *   | NXT brick            | *invalid*
                                      */
    PBIO_BUTTON_UP         = 1 << 8, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | *invalid*
                                      *   | EV3 Brick            | Up button
                                      *   | EV3 IR remote        | Beacon
                                      *   | NXT brick            | *invalid*
                                      */
    PBIO_BUTTON_RIGHT_UP   = 1 << 9, /**< | Device               | Button
                                      *   |----------------------|---------------
                                      *   | Powered UP hub       | *invalid*
                                      *   | Powered UP remote    | Righthand '+' button
                                      *   | SPIKE Prime hub      | Bluetooth button
                                      *   | EV3 Brick            | *invalid*
                                      *   | EV3 IR remote        | Righthand up button (blue bar)
                                      *   | NXT brick            | *invalid*
                                      */

} pbio_button_flags_t;

// Generic button state getter
typedef pbio_error_t (*pbio_button_is_pressed_func_t)(pbio_button_flags_t *pressed);

#ifdef DOXYGEN

/**
 * Get bitmask indicating currently pressed buttons.
 * @param [out] pressed     Bitmask indicating which buttons are pressed
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_NO_DEV if port is valid but a device with buttons is not connected
 *                          ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_button_is_pressed(pbio_button_flags_t *pressed);

#else // DOXYGEN

// calling pbdrv function directly for efficiency
#define pbio_button_is_pressed pbdrv_button_is_pressed

// include for pbdrv_button_is_pressed needs to be called after pbio_button_flags_t is defined
#include <pbdrv/button.h>

#endif // DOXYGEN

#endif // _PBIO_BUTTON_H_

/** @} */
