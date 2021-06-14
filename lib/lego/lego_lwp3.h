// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// LEGO Wireless Protocol v3 (LWP3)
// https://lego.github.io/lego-ble-wireless-protocol-docs/

#ifndef _LEGO_LWP3_H_
#define _LEGO_LWP3_H_

// https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/
#define LWP3_LEGO_COMPANY_ID 0x0397

#define LWP3_HUB_SYSTEM(id) (id << 5)

typedef enum {
    LWP3_HUB_SYSTEM_WEDO2       = LWP3_HUB_SYSTEM(0),
    LWP3_HUB_SYSTEM_DUPLO       = LWP3_HUB_SYSTEM(1),
    LWP3_HUB_SYSTEM_SYSTEM      = LWP3_HUB_SYSTEM(2),
    LWP3_HUB_SYSTEM_SYSTEM_     = LWP3_HUB_SYSTEM(3),
    LWP3_HUB_SYSTEM_TECHNIC     = LWP3_HUB_SYSTEM(4),
} lwp3_hub_system_t;

#define LWP3_HUB_KIND(system, device) (system | device)

typedef enum {
    LWP3_HUB_KIND_WEDO2             = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_WEDO2, 0),

    LWP3_HUB_KIND_DUPLO_TRAIN       = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_DUPLO, 0),

    LWP3_HUB_KIND_BOOST             = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_SYSTEM, 0),
    LWP3_HUB_KIND_SYSTEM_2IO        = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_SYSTEM, 1),
    LWP3_HUB_KIND_HANDSET           = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_SYSTEM, 2),
    LWP3_HUB_KIND_MARIO             = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_SYSTEM, 3),

    LWP3_HUB_KIND_TECHNIC_MEDIUM    = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_TECHNIC, 0),
    LWP3_HUB_KIND_TECHNIC_LARGE     = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_TECHNIC, 1),
} lwp3_hub_kind_t;

#endif // _LEGO_LWP3_H_
