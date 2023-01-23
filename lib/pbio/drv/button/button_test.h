// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_BUTTON_TEST_H_
#define _INTERNAL_PBDRV_BUTTON_TEST_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_TEST

#include <pbio/button.h>

// this can be used by tests that consume the button driver
void pbio_test_button_set_pressed(pbio_button_flags_t flags);

#endif // PBDRV_CONFIG_BUTTON_TEST

#endif // _INTERNAL_PBDRV_BUTTON_TEST_H_
