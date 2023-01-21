/** @file avr.h
 *  @brief Coprocessor interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS_AVR_H__
#define __NXOS_BASE_DRIVERS_AVR_H__

#include "base/types.h"

/** @addtogroup driver */
/*@{*/

/** @defgroup avr Coprocessor interface
 *
 * The coprocessor in the NXT is in charge of analog sensor and motor
 * output control, but also serves a number of secondary
 * functions. The functions related to sensors and motors are hidden
 * away under the higher level specialized API, but the secondary
 * utility functions are available from this module.
 */
/*@{*/

/** The type of button press that has been detected. */
typedef enum {
  BUTTON_NONE = 0, /**< No button pressed. */
  BUTTON_OK, /**< OK (central orange button) pressed. */
  BUTTON_CANCEL, /**< Cancel pressed. */
  BUTTON_LEFT, /**< Left arrow pressed. */
  BUTTON_RIGHT, /**< Right arrow pressed. */
} nx_avr_button_t;

/** Return the state of the brick's buttons.
 *
 * @return A <tt>nx_avr_button_t</tt> value.
 *
 * @note Given hardware limitations on the NXT, only one button may be
 * depressed at once. Thus only one button is reported as being
 * pressed at once.
 */
nx_avr_button_t nx_avr_get_button(void);

/** Return the measured battery voltage in millivolts.
 *
 * @return The measured voltage, in millivolts.
 */
U32 nx_avr_get_battery_voltage(void);

/** Detect the kind of power supply connected to the NXT.
 *
 * @return TRUE if the power supply is AA batteries, FALSE if it is a
 * Lego power pack (possibly with AC supply).
 */
bool nx_avr_battery_is_aa(void);

/** Populate @a major, @a minor with the version of the AVR firmware.
 *
 * @param major The major version of the AVR firmware.
 * @param minor The minor version of the AVR firmware.
 */
void nx_avr_get_version(U8 *major, U8 *minor);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_AVR_H__ */
