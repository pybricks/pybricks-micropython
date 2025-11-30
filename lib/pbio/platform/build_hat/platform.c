// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/gpio.h>

#include "hardware/gpio.h"

const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin = {
    .pin = 17,
};

int main(void) {
    extern void pbsys_main(void);
    pbsys_main();
}
