// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_TACHO_H_
#define _PBIO_TACHO_H_

#include <stdint.h>

#include <fixmath.h>

#include <pbio/config.h>
#include <pbdrv/counter.h>
#include <pbio/dcmotor.h>

typedef struct _pbio_tacho_t pbio_tacho_t;

#if PBIO_CONFIG_TACHO

pbio_error_t pbio_tacho_get(pbio_port_t port, pbio_tacho_t **tacho, pbio_direction_t direction, fix16_t gear_ratio);

pbio_error_t pbio_tacho_get_count(pbio_tacho_t *tacho, int32_t *count);
pbio_error_t pbio_tacho_get_angle(pbio_tacho_t *tacho, int32_t *angle);
pbio_error_t pbio_tacho_reset_angle(pbio_tacho_t *tacho, int32_t reset_angle, bool reset_to_abs);
pbio_error_t pbio_tacho_get_rate(pbio_tacho_t *tacho, int32_t *encoder_rate);
pbio_error_t pbio_tacho_get_angular_rate(pbio_tacho_t *tacho, int32_t *angular_rate);

#else

static inline pbio_error_t pbio_tacho_get(pbio_port_t port, pbio_tacho_t **tacho, pbio_direction_t direction, fix16_t gear_ratio) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_tacho_get_count(pbio_tacho_t *tacho, int32_t *count) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_tacho_get_angle(pbio_tacho_t *tacho, int32_t *angle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_tacho_reset_angle(pbio_tacho_t *tacho, int32_t reset_angle, bool reset_to_abs) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_tacho_get_rate(pbio_tacho_t *tacho, int32_t *encoder_rate) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_tacho_get_angular_rate(pbio_tacho_t *tacho, int32_t *angular_rate) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBIO_CONFIG_TACHO

#endif // _PBIO_TACHO_H_
