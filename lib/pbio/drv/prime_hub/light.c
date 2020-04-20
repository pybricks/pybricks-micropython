// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/light.h>
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

// pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b) {
//     return PBIO_SUCCESS;
// }

// pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_light_color_t color,
//                                            uint8_t *r, uint8_t *g, uint8_t *b) {
//     if (port != PBIO_PORT_SELF) {
//         return PBIO_ERROR_INVALID_PORT;
//     }

//     switch (color) {
//     case PBIO_LIGHT_COLOR_NONE:
//         *r = 0;
//         *g = 0;
//         *b = 0;
//         break;
//     case PBIO_LIGHT_COLOR_WHITE:
//         *r = 255;
//         *g = 140;
//         *b = 60;
//         break;
//     case PBIO_LIGHT_COLOR_RED:
//         *r = 255;
//         *g = 0;
//         *b = 0;
//         break;
//     case PBIO_LIGHT_COLOR_ORANGE:
//         *r = 255;
//         *g = 25;
//         *b = 0;
//         break;
//     case PBIO_LIGHT_COLOR_YELLOW:
//         *r = 255;
//         *g = 70;
//         *b = 0;
//         break;
//     case PBIO_LIGHT_COLOR_GREEN:
//         *r = 0;
//         *g = 200;
//         *b = 0;
//         break;
//     case PBIO_LIGHT_COLOR_BLUE:
//         *r = 0;
//         *g = 0;
//         *b = 255;
//         break;
//     case PBIO_LIGHT_COLOR_PURPLE:
//         *r = 220;
//         *g = 0;
//         *b = 120;
//         break;
//     default:
//         return PBIO_ERROR_INVALID_ARG;
//     }

//     return PBIO_SUCCESS;
// }
