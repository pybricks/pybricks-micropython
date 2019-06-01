// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PBIO_MAIN_H_
#define _PBIO_MAIN_H_

#include "pbio/config.h"

void pbio_init(void);
int pbio_do_one_event(void);

#if PBIO_CONFIG_ENABLE_DEINIT
void pbio_deinit(void);
#else
static inline void pbio_deinit(void) { }
#endif

#endif // _PBIO_MAIN_H_
