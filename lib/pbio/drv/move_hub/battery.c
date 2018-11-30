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

#include <pbdrv/adc.h>

#include <pbio/error.h>
#include <pbio/port.h>

#define PBDRV_BATTERY_VOLTAGE_CH 11
#define PBDRV_BATTERY_CURRENT_CH 10

void _pbdrv_battery_init(void) { }

void _pbdrv_battery_poll(uint32_t now) { }

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_battery_deinit(void) { }
#endif

pbio_error_t pbdrv_battery_get_voltage_now(pbio_port_t port, uint16_t *value) {
    uint16_t raw;
    pbio_error_t err;

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    err = pbdrv_adc_get_ch(PBDRV_BATTERY_VOLTAGE_CH, &raw);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // REVISIT: do we want to take into account shunt resistor voltage drop
    // like on EV3? Probably only makes a difference of ~10mV at the most.
    *value = raw * 9600 / 3893;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_current_now(pbio_port_t port, uint16_t *value) {
    uint16_t raw;
    pbio_error_t err;

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // this is measuring the voltage across a 0.05 ohm shunt resistor probably
    // via an op amp with unknown gain.
    err = pbdrv_adc_get_ch(PBDRV_BATTERY_CURRENT_CH, &raw);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // FIXME: these values come from LEGO firmware, but seem to be 2x current
    *value = raw * 2444 / 4095;

    return PBIO_SUCCESS;
}
