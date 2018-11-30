/*
 * Copyright (c) 2018 Laurens Valk
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

#include <stdio.h>

#include <pbdrv/light.h>
#include <pbio/port.h>
#include <pbio/error.h>

#define NLEDS 4

static FILE *f_brightness[NLEDS];

void _pbdrv_light_init(void) {

    const char * const trigger_paths[NLEDS] = {
        "/sys/class/leds/led0:red:brick-status/trigger",
        "/sys/class/leds/led1:red:brick-status/trigger",
        "/sys/class/leds/led0:green:brick-status/trigger",
        "/sys/class/leds/led1:green:brick-status/trigger"
    };

    const char * const brightness_paths[NLEDS] = {
        "/sys/class/leds/led0:red:brick-status/brightness",
        "/sys/class/leds/led1:red:brick-status/brightness",
        "/sys/class/leds/led0:green:brick-status/brightness",
        "/sys/class/leds/led1:green:brick-status/brightness"
    };

    for (int led = 0; led < NLEDS; led++) {
        FILE* f_trigger = fopen(trigger_paths[led], "w");
        if (!f_trigger) {
            continue;
        }
        fprintf(f_trigger, "none");
        fclose(f_trigger);
        f_brightness[led] = fopen(brightness_paths[led], "w");
    }
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_light_deinit(void) {
    for (int led = 0; led < NLEDS; led++) {
        if (f_brightness[led]) {
            fclose(f_brightness[led]);
        }
    }
}
#endif

pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }
    for (int led = 0; led < NLEDS; led++) {
        if (f_brightness[led]) {
            int ret;

            ret = fseek(f_brightness[led], 0, SEEK_SET);
            if (ret == -1) {
                return PBIO_ERROR_IO;
            }
            ret = fprintf(f_brightness[led], "%d", led < 2 ? r : g);
            if (ret < 0) {
                return PBIO_ERROR_IO;
            }
            ret = fflush(f_brightness[led]);
            if (ret == EOF) {
                return PBIO_ERROR_IO;
            }
        }
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_light_color_t color,
                                           uint8_t *r, uint8_t *g, uint8_t *b) {

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    switch (color) {
    case PBIO_LIGHT_COLOR_NONE:
        *r = 0;
        *g = 0;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_RED:
        *r = 255;
        *g = 0;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_ORANGE:
        *r = 255;
        *g = 255;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_YELLOW:
        *r = 30;
        *g = 255;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_GREEN:
        *r = 0;
        *g = 255;
        *b = 0;
        break;
    default:
        return PBIO_ERROR_INVALID_ARG;
    }

    return PBIO_SUCCESS;
}
