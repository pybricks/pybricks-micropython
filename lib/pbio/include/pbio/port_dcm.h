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

PT_THREAD(pbio_port_dcm_thread(struct pt *pt, struct etimer *etimer, pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins));

/**
 * Gets device connection manager state.
 *
 * @param [in]  index       The index of the DC motor.
 * @return                  The dcmotor instance.
 */
pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index);

/**
 * Asserts or gets the device id of a device connected to the port.
 *
 * @param [in]  lump_dev    The connection manager instance.
 * @param [out] type_id     The device id.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_NO_DEV if the port does not have a device attached or is not of the expected type range.
 */
pbio_error_t pbio_port_dcm_assert_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t *type_id);

/**
 * Gets the analog value of the device connected to the port.
 */
uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins);

#else // PBIO_CONFIG_PORT_DCM

static inline pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index) {
    return NULL;
}

static inline pbio_error_t pbio_port_dcm_assert_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t *type_id) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins) {
    return 0;
}

static inline PT_THREAD(pbio_port_dcm_thread(struct pt *pt, struct etimer *etimer, pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins)) {
    PT_BEGIN(pt);
    PT_END(pt);
}

#endif // PBIO_CONFIG_PORT_DCM

#endif // _PBIO_PORT_DCM_H_
