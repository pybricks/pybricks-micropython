// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_LINUX_EV3

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <linux/input.h>

#include <pbio/button.h>
#include <pbio/config.h>
#include <pbio/error.h>

static uint8_t f_btn = -1; // Button file descriptor

void _pbdrv_button_init(void) {
    f_btn = open("/dev/input/by-path/platform-gpio_keys-event", O_RDONLY);
    if (f_btn == -1) {
        perror("Failed to init buttons");
    }
}

#if PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_button_deinit(void) {
    close(f_btn);
    f_btn = -1;
}
#endif

static bool check(uint8_t *buffer, uint8_t key) {
    return buffer[key >> 3] & (1 << (key % 8));
}

pbio_error_t pbdrv_button_is_pressed(pbio_button_flags_t *pressed) {
    uint8_t buffer[(KEY_CNT + 7) / 8];

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

#endif // PBDRV_CONFIG_BUTTON_LINUX_EV3
