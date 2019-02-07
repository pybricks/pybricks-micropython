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

#include <pbio/error.h>
#include <pbio/port.h>

FILE *f_voltage;
FILE *f_current;

void _pbdrv_battery_init(void) {
    f_voltage = fopen("/sys/class/power_supply/lego-ev3-battery/voltage_now", "r");
    f_current = fopen("/sys/class/power_supply/lego-ev3-battery/current_now", "r");
}

void _pbdrv_battery_poll(uint32_t now) { }

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_battery_deinit(void) {
    if (f_voltage != NULL) {
        fclose(f_voltage);
    }
    if (f_current != NULL) {    
        fclose(f_current);
    }
}
#endif

pbio_error_t pbdrv_battery_get_voltage_now(pbio_port_t port, uint16_t *value) {
    int32_t microvolt;
    if (0 == fseek(f_voltage, 0, SEEK_SET) &&
        0 <= fscanf(f_voltage, "%d", &microvolt) &&
        0 == fflush(f_voltage)) {
        *value = (microvolt / 1000);
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;    
}

pbio_error_t pbdrv_battery_get_current_now(pbio_port_t port, uint16_t *value) {
    int32_t microamp;
    if (0 == fseek(f_current, 0, SEEK_SET) &&
        0 <= fscanf(f_current, "%d", &microamp) &&
        0 == fflush(f_current)) {
        *value = (microamp / 1000);
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO; 
}
