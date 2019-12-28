// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_TACHO_H_
#define _PBIO_TACHO_H_

#include <stdint.h>

#include <fixmath.h>

#include <pbio/config.h>
#include <pbdrv/counter.h>
#include <pbio/hbridge.h>

typedef struct _pbio_tacho_t {
    pbio_direction_t direction;
    int32_t offset;
    fix16_t counts_per_degree;
    fix16_t counts_per_output_unit;
    pbdrv_counter_dev_t *counter;
} pbio_tacho_t;

#if PBIO_CONFIG_TACHO

pbio_error_t pbio_tacho_get(pbio_port_t port, pbio_tacho_t **tacho, pbio_direction_t direction, fix16_t counts_per_degree, fix16_t gear_ratio);

pbio_error_t pbio_tacho_get_count(pbio_tacho_t *tacho, int32_t *count);
pbio_error_t pbio_tacho_get_angle(pbio_tacho_t *tacho, int32_t *angle);
pbio_error_t pbio_tacho_reset_angle(pbio_tacho_t *tacho, int32_t reset_angle);
pbio_error_t pbio_tacho_reset_angle_to_abs(pbio_tacho_t *tacho);
pbio_error_t pbio_tacho_get_rate(pbio_tacho_t *tacho, int32_t *encoder_rate);
pbio_error_t pbio_tacho_get_angular_rate(pbio_tacho_t *tacho, int32_t *angular_rate);

#else

static inline pbio_error_t pbio_tacho_get(pbio_port_t port, pbio_tacho_t **tacho, pbio_direction_t direction, fix16_t counts_per_degree, fix16_t gear_ratio) { return PBIO_ERROR_NOT_SUPPORTED; }

static inline pbio_error_t pbio_tacho_get_count(pbio_tacho_t *tacho, int32_t *count) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline pbio_error_t pbio_tacho_get_angle(pbio_tacho_t *tacho, int32_t *angle) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline pbio_error_t pbio_tacho_reset_angle(pbio_tacho_t *tacho, int32_t reset_angle) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline pbio_error_t pbio_tacho_reset_angle_to_abs(pbio_tacho_t *tacho) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline pbio_error_t pbio_tacho_get_rate(pbio_tacho_t *tacho, int32_t *encoder_rate) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline pbio_error_t pbio_tacho_get_angular_rate(pbio_tacho_t *tacho, int32_t *angular_rate) { return PBIO_ERROR_NOT_SUPPORTED; }

#endif // PBIO_CONFIG_TACHO

#endif // _PBIO_TACHO_H_
