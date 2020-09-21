// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_LIGHT_GRID_H_
#define _PBSYS_SYS_LIGHT_GRID_H_

#include <contiki.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_HUB_LIGHT_GRID
void pbsys_hub_light_grid_init();
void pbsys_hub_light_grid_handle_event(process_event_t event, process_data_t data);
#else
#define pbsys_hub_light_grid_init()
#define pbsys_hub_light_grid_handle_event(event, data)
#endif

#endif // _PBSYS_SYS_LIGHT_GRID_H_
