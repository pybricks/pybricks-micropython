// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_LEGODEV_SPEC_H_
#define _INTERNAL_PBDRV_LEGODEV_SPEC_H_

#include <stdint.h>

#include <pbdrv/legodev.h>

uint32_t pbdrv_legodev_spec_stale_data_delay(pbdrv_legodev_type_id_t id, uint8_t mode);

uint32_t pbdrv_legodev_spec_data_set_delay(pbdrv_legodev_type_id_t id, uint8_t mode);

uint8_t pbdrv_legodev_spec_default_mode(pbdrv_legodev_type_id_t id);

pbdrv_legodev_capability_flags_t pbdrv_legodev_spec_basic_flags(pbdrv_legodev_type_id_t id);

#endif // _INTERNAL_PBDRV_LEGODEV_SPEC_H_
