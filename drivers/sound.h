/** @file sound.h
 *  @brief Sound system interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS_SOUND_H__
#define __NXOS_BASE_DRIVERS_SOUND_H__

#include "base/types.h"

/** @addtogroup driver */
/*@{*/

/** @defgroup sound Sound
 *
 * The sound driver provides a very simple API to drive the I2S sound
 * chip. All the driving is done asynchronously.
 *
 * @warning Currently the sound driver is fairly limited: only one
 * beep at a time may be emitted. Avoid emitting a second beep while
 * one is still being emitted. Furthermore, avoid beeps that are under
 * 100ms in duration, since there is a bug that will crash the sound
 * driver.
 */
/*@{*/

/** Emit a beep at @a freq Hz for @a ms milliseconds.
 *
 * @param freq The frequency of the beep in Hz.
 * @param ms The duration of the beep.
 *
 * @note This function is asynchronous. It will start the beeping and
 * return immediately.
 */
void nx_sound_freq_async(U32 freq, U32 ms);

/** Synchronously emit a beep at @a freq Hz for @a ms milliseconds.
 *
 * Same as nx_sound_freq_async(), but will block until the beep ends.
 *
 * @param freq The frequency of the beep in Hz.
 * @param ms The duration of the beep.
 */
void nx_sound_freq(U32 freq, U32 ms);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_SOUND_H__ */
