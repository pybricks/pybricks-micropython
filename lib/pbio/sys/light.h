// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_LIGHT_H_
#define _PBSYS_SYS_LIGHT_H_

#include <contiki.h>

#include <pbio/color.h>
#include <pbsys/config.h>

#if PBSYS_CONFIG_STATUS_LIGHT
void pbsys_status_light_init(void);
void pbsys_status_light_handle_event(process_event_t event, process_data_t data);
void pbsys_status_light_poll(void);
#else
#define pbsys_status_light_init()
#define pbsys_status_light_handle_event(event, data)
#define pbsys_status_light_poll()
#endif

#if PBSYS_CONFIG_STATUS_LIGHT_BLUETOOTH
void pbsys_status_light_bluetooth_set_color(pbio_color_t color);
void pbsys_status_light_bluetooth_deinit(void);
#else
static inline void pbsys_status_light_bluetooth_set_color(pbio_color_t color) {
}
static inline void pbsys_status_light_bluetooth_deinit(void) {
}
#endif

#endif // _PBSYS_SYS_LIGHT_H_
