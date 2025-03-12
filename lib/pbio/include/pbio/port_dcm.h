// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#ifndef _PBIO_PORT_DCM_H_
#define _PBIO_PORT_DCM_H_

#include <contiki.h>
#include <pbio/config.h>
#include <pbio/port.h>
#include <pbdrv/ioport.h>

typedef struct _pbio_port_dcm_t pbio_port_dcm_t;

#if PBIO_CONFIG_PORT_DCM

PT_THREAD(pbio_port_dcm_thread(struct pt *pt, struct etimer *etimer, pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins, pbio_port_device_info_t *device_info));

pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index);

#else // PBIO_CONFIG_PORT_DCM

static inline pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index) {
    return NULL;
}

static inline PT_THREAD(pbio_port_dcm_thread(struct pt *pt, struct etimer *etimer, pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins, pbio_port_device_info_t *device_info)) {
    PT_BEGIN(pt);
    PT_END(pt);
}

#endif // PBIO_CONFIG_PORT_DCM

#endif // _PBIO_PORT_DCM_H_
