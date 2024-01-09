// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2024 The Pybricks Authors

#ifndef _PBIO_LWP3_H_
#define _PBIO_LWP3_H_

#include <stdint.h>

#include "lego_lwp3.h"

extern const uint8_t pbio_lwp3_hub_service_uuid[];
extern const uint8_t pbio_lwp3_hub_char_uuid[];

bool pbio_lwp3_advertisement_matches(uint8_t event_type, const uint8_t *data, lwp3_hub_kind_t hub_kind);

#endif // _PBIO_LWP3_H_
