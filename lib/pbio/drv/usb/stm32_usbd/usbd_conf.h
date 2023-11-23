// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _USBD_CONF_H_
#define _USBD_CONF_H_

#include "stm32f4xx_hal.h"

#define USBD_MAX_NUM_INTERFACES               1
#define USBD_MAX_NUM_CONFIGURATION            1
#define USBD_MAX_STR_DESC_SIZ                 0x100
#define USBD_SELF_POWERED                     1
#define USBD_CLASS_BOS_ENABLED                1

#endif /* _USBD_CONF_H_ */
