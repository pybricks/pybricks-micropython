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
 * \addtogroup LightDriver Light I/O driver
 * @{
 */

#ifndef _PBDRV_LIGHT_H_
#define _PBDRV_LIGHT_H_

#include <stdint.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/light.h>
#include <pbio/port.h>

#if PBDRV_CONFIG_LIGHT

/** @cond INTERNAL */

/**
 * Initializes the low level light blinking engine. This should be called only
 * once and must be called before using any other light functions.
 */
void _pbdrv_light_init(void);

/**
 * Releases the low level light driver. No light functions can be called after
 * calling this function.
 */
#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_light_deinit(void);
#else
static inline void _pbdrv_light_deinit(void) { }
#endif

/** @endcond */

/**
 * Sets the color of the light. The RGB values are "raw" values, meaning that
 * setting all values to 255 may not result in a white color because of
 * different maximum intensities of the component LEDs. Use
 * ::pbdrv_light_get_rgb_for_color() to get device-specific RGB values for
 * predefined colors.
 *
 * The light may not be capabile of displaying all colors or may not have
 * adjustable brightness. If a light is only white, the color values
 * will be averaged to give the final intensity. If the light only has one or
 * two of the possible three colors, the other color values will be ignored.
 * If the light is not capabile of adjusting the intensity, values < 128 will
 * be considered "off" and >= 128 will be considered as "on".
 * @param [in]  port    The light port
 * @param [in]  r       The red component
 * @param [in]  g       The green component
 * @param [in]  b       The blue component
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b);

/**
 * Gets the "raw" RGB values for a predefined color. These returned values
 * should be passed to ::pbdrv_light_set_rgb() to produce that color.
 * @param [in]  port        The light port
 * @param [in]  color       The color to look up
 * @param [out] r           The red component
 * @param [out] g           The green component
 * @param [out] b           The blue component
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_INVALID_ARG if the color value is not valid
 *                          ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 */
pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_light_color_t color,
                                           uint8_t *r, uint8_t *g, uint8_t *b);

#else

static inline void _pbdrv_light_init(void) { }
static inline void _pbdrv_light_deinit(void) { }
static inline pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t
pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_light_color_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif

#endif // _PBDRV_LIGHT_H_

/** @}*/
