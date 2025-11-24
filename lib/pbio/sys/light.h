// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_LIGHT_H_
#define _PBSYS_SYS_LIGHT_H_

#include <pbio/color.h>
#include <pbsys/config.h>

#if PBSYS_CONFIG_STATUS_LIGHT
void pbsys_status_light_init(void);
void pbsys_status_light_handle_status_change(void);
void pbsys_status_light_poll(void);
#else
#define pbsys_status_light_init()
#define pbsys_status_light_handle_status_change()
#define pbsys_status_light_poll()
#endif

#endif // _PBSYS_SYS_LIGHT_H_
