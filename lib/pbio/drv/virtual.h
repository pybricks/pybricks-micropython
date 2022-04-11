// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Common functions used by virtual (Python) drivers.

#ifndef _INTERNAL_PBDRV_VIRTUAL_H_
#define _INTERNAL_PBDRV_VIRTUAL_H_

unsigned long pbdrv_virtual_get_signed_long(const char *property);
unsigned long pbdrv_virtual_get_indexed_signed_long(const char *property, uint8_t index);
unsigned long pbdrv_virtual_get_unsigned_long(const char *property);
unsigned long pbdrv_virtual_get_indexed_unsigned_long(const char *property, uint8_t index);

#endif // _INTERNAL_PBDRV_VIRTUAL_H_
