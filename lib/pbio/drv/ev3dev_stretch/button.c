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

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>

#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/port.h>

static uint8_t f_btn = -1; // Button file descriptor

void _pbdrv_button_init(void) {
    f_btn = open("/dev/input/by-path/platform-gpio_keys-event", O_RDONLY);
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_button_deinit(void) {
    close(f_btn);
    f_btn = -1;
 }
#endif

static bool check(uint8_t *buffer, uint8_t key) {
    return buffer[key>>3] & (1 << (key % 8));
}

pbio_error_t pbdrv_button_is_pressed(pbio_port_t port, pbio_button_flags_t *pressed) {
    uint8_t buffer[(KEY_CNT + 7) / 8];

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    if (ioctl(f_btn, EVIOCGKEY(sizeof(buffer)), &buffer) == -1) {
        return PBIO_ERROR_IO;
    }

    *pressed = 0;
    if (check(buffer, KEY_UP)) {
        *pressed |= PBIO_BUTTON_UP;
    }
    if (check(buffer, KEY_ENTER)) {
        *pressed |= PBIO_BUTTON_CENTER;
    }
    if (check(buffer, KEY_DOWN)) {
        *pressed |= PBIO_BUTTON_DOWN;
    }
    if (check(buffer, KEY_RIGHT)) {
        *pressed |= PBIO_BUTTON_RIGHT;
    }
    if (check(buffer, KEY_LEFT)) {
        *pressed |= PBIO_BUTTON_LEFT;
    }
    if (check(buffer, KEY_BACKSPACE)) {
        *pressed |= PBIO_BUTTON_LEFT_UP;
    }

    return PBIO_SUCCESS;
}
