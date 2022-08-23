// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef _PBIO_MAIN_H_
#define _PBIO_MAIN_H_

#include <stdbool.h>

#include "pbio/config.h"

void pbio_init(void);
void pbio_stop_all(bool reset);
int pbio_do_one_event(void);

#endif // _PBIO_MAIN_H_
