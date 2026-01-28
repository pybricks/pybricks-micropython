// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef _PBIO_MAIN_H_
#define _PBIO_MAIN_H_

#include <stdbool.h>

#include "pbio/config.h"

#include <pbio/error.h>

void pbio_init(void);
void pbio_deinit(void);
pbio_error_t pbio_main_start_application_resources(void);
pbio_error_t pbio_main_stop_application_resources(void);
void pbio_main_soft_stop(void);

#endif // _PBIO_MAIN_H_
