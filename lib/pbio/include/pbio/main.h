// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef _PBIO_MAIN_H_
#define _PBIO_MAIN_H_

#include "pbio/config.h"

/**
 * Event loop hook function.
 */
typedef void (*pbio_event_hook_t)(void);

void pbio_init(void);
void pbio_stop_all(void);
int pbio_do_one_event(void);
void pbio_set_event_hook(pbio_event_hook_t hook);

#endif // _PBIO_MAIN_H_
