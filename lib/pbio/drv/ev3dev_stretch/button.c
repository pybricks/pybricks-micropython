/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Laurens Valk
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
