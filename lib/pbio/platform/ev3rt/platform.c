// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include "ev3api.h"
#include "app.h"

#include <pbsys/main.h>

/**
 * Any device specific initialization that isn't already done by EV3RT can
 * be done here. This follows the pattern of the embedded hubs where there is
 * a bit more to do.
 */
static void SystemInit(void) {
    // Show the Pybricks logo on the screen so we know that something is
    // running. This should be replaced by an appropriate driver in pbio and
    // called from the system hmi interface in pbsys.
    memfile_t memfile;
    image_t image;
    if (ev3_memfile_load("/pybricks.bmp", &memfile) == E_OK &&
        ev3_image_load(&memfile, &image) == E_OK) {
        ev3_lcd_draw_image(&image, 0, 0);
    }
}

/**
 * This is the main user task launched by EV3RT. It initializes the system
 * and then runs to the main pbsys function. This is similar to how these two
 * subsequent calls are normally made from startup.s on the embedded hubs.
 */
void main_task(intptr_t unused) {
    SystemInit();
    extern int main(int argc, char **argv);
    main(0, NULL);
}
