// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Pybricks platform data for BlueKitchen BTStack.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_H_
#define _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_H_

#include <btstack.h>
#include <btstack_chipset.h>
#include <btstack_control.h>
#include <btstack_uart_block.h>

#include <pbio/error.h>

/**
 * Optional platform initialization before BTstack takes over.
 *
 * Unlike some BTstack init code, this is guaranteed to be called only once.
 *
 * @return Any error code to skip Bluetooth or ::PBIO_SUCCESS to proceed.
 */
pbio_error_t pbdrv_bluetooth_btstack_platform_init(void);

/**
 * Optional platform poll handler, called on every process iteration.
 */
void pbdrv_bluetooth_btstack_platform_poll(void);

/**
 * Optional platform packet handler, called on HCI packets before common handler.
 *
 * @param [in]  packet_type  The HCI packet type.
 * @param [in]  channel      The HCI channel.
 * @param [in]  packet       Pointer to the raw packet data.
 * @param [in]  size         Size of the packet data.
 */
void pbdrv_bluetooth_btstack_platform_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

/** Chipset info */
typedef struct {
    /** Version */
    const uint16_t lmp_version;
    /** Initialization script */
    const uint8_t *init_script;
    /** Initialization script size */
    const uint32_t init_script_size;
} pbdrv_bluetooth_btstack_chipset_info_t;

/**
 * Local HCI version info returned by the chipset.
 */
typedef struct {
    /** HCI version. */
    uint8_t hci_version;
    /** HCI revision. */
    uint16_t hci_revision;
    /** LMP/PAL version. */
    uint8_t lmp_pal_version;
    /** Manufacturer. */
    uint16_t manufacturer;
    /** LMP/PAL subversion. */
    uint16_t lmp_pal_subversion;
} pbdrv_bluetooth_btstack_local_version_info_t;

/**
 * Hook called when BTstack reads the local version information.
 *
 * This is called _after_ hci_set_chipset but _before_ the init script is sent
 * over the wire, so this can be used to dynamically select the init script.
 *
 * @param device_info The device information read from the Bluetooth chip.
 */
void pbdrv_bluetooth_btstack_set_chipset(pbdrv_bluetooth_btstack_local_version_info_t *device_info);

typedef struct {
    const hci_transport_t *(*transport_instance)(void);
    const void *(*transport_config)(void);
    const btstack_chipset_t *(*chipset_instance)(void);
    const btstack_control_t *(*control_instance)(void);
    const uint8_t *er_key;
    const uint8_t *ir_key;
} pbdrv_bluetooth_btstack_platform_data_t;

// defined in platform.c
extern const pbdrv_bluetooth_btstack_platform_data_t pbdrv_bluetooth_btstack_platform_data;

#endif // _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_H_
