// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_MESSAGING_H
#define PYBRICKS_INCLUDED_PYBRICKS_MESSAGING_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_MESSAGING

#include "py/obj.h"

#if PYBRICKS_PY_MESSAGING_BLE_RADIO
extern const mp_obj_type_t pb_type_ble_radio;
#endif

#if PYBRICKS_PY_MESSAGING_BLE_RADIO_OLD
mp_obj_t pb_type_BLE_new(mp_obj_t broadcast_channel_in, mp_obj_t observe_channels_in);
#endif

#if PYBRICKS_PY_MESSAGING_APP_DATA
extern const mp_obj_type_t pb_type_app_data;
#endif

#endif // PYBRICKS_PY_MESSAGING

#endif // PYBRICKS_INCLUDED_PYBRICKS_MESSAGING_H
