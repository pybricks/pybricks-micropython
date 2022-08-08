// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// System capability configuration options

#ifndef _PBSYS_CONFIG_H_
#define _PBSYS_CONFIG_H_

#include "pbsysconfig.h"

// When set to (1) PBSYS_CONFIG_STATUS_LIGHT indicates that a hub has a hub status light
#ifndef PBSYS_CONFIG_STATUS_LIGHT
#error "Must define PBSYS_CONFIG_STATUS_LIGHT in pbsysconfig.h"
#endif

// Downloaded programs are received in chunks up to this size.
#define PBSYS_CONFIG_DOWNLOAD_CHUNK_SIZE (100)

#endif // _PBSYS_CONFIG_H_
