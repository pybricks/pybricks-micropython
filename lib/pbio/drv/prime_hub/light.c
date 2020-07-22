// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/light.h>
#include <pbio/color.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

// #include "stm32f413xx.h"

// // setup LED PWMs and pins
// void _pbdrv_light_init(void) {

// }

// #if PBIO_CONFIG_ENABLE_DEINIT
// // turn off the light
// void _pbdrv_light_deinit(void) {
// }
// #endif

// pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint16_t r, uint16_t g, uint16_t b) {
//     return PBIO_SUCCESS;
// }

// pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_color_t color,
//                                            pbdrv_light_raw_rgb_t *raw) {
//     if (port != PBIO_PORT_SELF) {
//         return PBIO_ERROR_INVALID_PORT;
//     }

//     pbio_color_rgb_t rgb;
//     pbio_color_to_rgb(color, &rgb);

//     // Adjust for chromacity
//     uint32_t r = rgb.r * 1000;
//     uint32_t g = rgb.g * 270;
//     uint32_t b = rgb.b * 200;

//     // Adjust for apparent brightness
//     // + 1 protects against division by zero
//     uint32_t Y = ((174 * r + 1590 * g + 327 * b) >> 16) + 1;
//     raw->r = r / Y;
//     raw->g = g / Y;
//     raw->b = b / Y;

//     return PBIO_SUCCESS;
// }
