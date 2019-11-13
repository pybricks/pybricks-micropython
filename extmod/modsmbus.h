// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbio/error.h>

typedef struct _smbus_t smbus_t;

pbio_error_t smbus_get(smbus_t **bus, int bus_num);
