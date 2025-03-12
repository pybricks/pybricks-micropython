// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

/**
 * @addtogroup Port pbio/port: I/O port interface
 * @{
 */

#ifndef _PBIO_PORT_H_
#define _PBIO_PORT_H_

#include <stdbool.h>
#include <stdint.h>

#include <lego/device.h>

#include <pbdrv/config.h>

#include <pbio/error.h>

typedef struct _pbio_port_t pbio_port_t;

/**
 * I/O port identifier. The meaning and availability of a port is device-specific.
 */
typedef enum {
    #if PBDRV_CONFIG_HAS_PORT_A
    PBIO_PORT_ID_A = 'A', /**< I/O port labeled as "A" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_B
    PBIO_PORT_ID_B = 'B', /**< I/O port labeled as "B" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_C
    PBIO_PORT_ID_C = 'C', /**< I/O port labeled as "C" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_D
    PBIO_PORT_ID_D = 'D', /**< I/O port labeled as "D" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_E
    PBIO_PORT_ID_E = 'E', /**< I/O port labeled as "C" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_F
    PBIO_PORT_ID_F = 'F', /**< I/O port labeled as "F" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_1
    PBIO_PORT_ID_1 = '1', /**< I/O port labeled as "1" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_2
    PBIO_PORT_ID_2 = '2', /**< I/O port labeled as "2" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_3
    PBIO_PORT_ID_3 = '3', /**< I/O port labeled as "3" */
    #endif
    #if PBDRV_CONFIG_HAS_PORT_4
    PBIO_PORT_ID_4 = '4', /**< I/O port labeled as "4" */
    #endif
} pbio_port_id_t;


/**
 * Kind (category) of attached device, if any.
 */
typedef enum {
    /**
     * No (known) device is connected.
     */
    PBIO_PORT_DEVICE_KIND_NONE,
    /**
     * Known DC motor or powered device like a light.
     */
    PBIO_PORT_DEVICE_KIND_DC_DEVICE,
    /**
     * A sensor with the LUMP protocol.
     */
    PBIO_PORT_DEVICE_KIND_LUMP,
    /**
     * A motor with quadrature encoders without intermediate processing.
     */
    PBIO_PORT_DEVICE_KIND_QUADRATURE_MOTOR,
} port_port_device_kind_t;

/**
 * Power state across the P1P2 pins when full on/off is required. Used for some
 * sensors that cannot run on the voltage provided by P4 alone.
 *
 * On some platforms, one pin may supply a voltage while the other is ground.
 * On some platforms, the two wires are driven by a motor driver. The effect on
 * a powered sensor is the same.
 */
typedef enum {
    /* The power is off. Both pins are either floating or both are ground */
    PBIO_PORT_POWER_REQUIREMENTS_NONE,
    /* The battery voltage is applied across the pins, with p1 the positive side. */
    PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P1_POS,
    /* The battery voltage is applied across the pins, with p2 the positive side. */
    PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P2_POS,
} pbio_port_power_requirements_t;

/**
 * Known information about the attached device. This is reset at the
 * start of the port process and may be set when a device is detected.
 */
typedef struct {
    /**
     * Power requirements for the port.
     */
    pbio_port_power_requirements_t power_requirements;
    /**
     * The device kind connected to this port, if any.
     */
    port_port_device_kind_t kind;
    /**
     * Type identifier of the device attached to this port, if any was detected.
     */
    lego_device_type_id_t type_id;
} pbio_port_device_info_t;


#endif // _PBIO_PORT_H_

/** @} */
