// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_HARDWARE_EV3

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/gpio.h>
#include <pbdrv/hardware.h>

#include <pbio/util.h>

#include "../bluetooth/bluetooth_btstack_ev3.h"
#include "../gpio/gpio_ev3.h"

#include <tiam1808/hw/hw_syscfg0_AM1808.h>

#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

/**
 * Hardware ID pins, least significant bit first.
 */
static const pbdrv_gpio_t hardware_id_pins[] = {
    PBDRV_GPIO_EV3_PIN(8, 11, 8, 3, 5), // HWID0
    PBDRV_GPIO_EV3_PIN(8, 15, 12, 3, 4), // HWID1
    PBDRV_GPIO_EV3_PIN(8, 23, 20, 3, 2), // HWID2
    PBDRV_GPIO_EV3_PIN(9, 23, 20, 4, 10), // HWID3
};

/**
 * Hardware version and its bitwise complement, read from the boot EEPROM at
 * 0x3f00 during early boot. Defined in platform.c.
 */
extern uint8_t pbdrv_ev3_eeprom_hardware_version[2];

/**
 * Bluetooth MAC address, read from the boot EEPROM at 0x3f06 during early
 * boot, or at 0x3f00 on boards without the hardware version field. Defined in
 * platform.c.
 */
extern uint8_t pbdrv_ev3_bluetooth_mac_address[6];

const char *pbdrv_hardware_get_version(void) {

    uint8_t hardware_id = 0;
    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(hardware_id_pins); i++) {
        hardware_id |= pbdrv_gpio_input(&hardware_id_pins[i]) << i;
    }

    // The EEPROM stores the complement alongside the version, so boards that
    // predate the version field can be told apart from boards that have it.
    uint8_t eeprom_version = pbdrv_ev3_eeprom_hardware_version[0];
    bool eeprom_version_valid = eeprom_version == (uint8_t)(pbdrv_ev3_eeprom_hardware_version[1] ^ 0xff);
    if (!eeprom_version_valid) {
        // Boards without the version field are V0.30.
        eeprom_version = 0x03;
    }

    DEBUG_PRINT("EV3 hardware: HWID 0x%01x, EEPROM version 0x%02x (raw 0x%02x 0x%02x, %s)\n",
        hardware_id, eeprom_version,
        pbdrv_ev3_eeprom_hardware_version[0], pbdrv_ev3_eeprom_hardware_version[1],
        eeprom_version_valid ? "valid" : "absent, assuming V0.30");

    DEBUG_PRINT("EV3 hardware: Bluetooth address %02X:%02X:%02X:%02X:%02X:%02X\n",
        pbdrv_ev3_bluetooth_mac_address[0], pbdrv_ev3_bluetooth_mac_address[1],
        pbdrv_ev3_bluetooth_mac_address[2], pbdrv_ev3_bluetooth_mac_address[3],
        pbdrv_ev3_bluetooth_mac_address[4], pbdrv_ev3_bluetooth_mac_address[5]);

    // Not known until the Bluetooth chip has been read out, which is well
    // before the user can get to any screen that shows this.
    const pbdrv_bluetooth_btstack_local_version_info_t *bt =
        pbdrv_bluetooth_btstack_ev3_get_local_version_info();

    // Need Bluetooth runtime information to tell some versions apart.
    if (!bt) {
        return NULL;
    }

    DEBUG_PRINT("EV3 bluetooth: HCI %u rev 0x%04x, LMP %u subversion 0x%04x, manufacturer %u, %s\n",
        bt->hci_version, bt->hci_revision, bt->lmp_pal_version,
        bt->lmp_pal_subversion, bt->manufacturer,
        bt->lmp_pal_subversion == cc2560_info.lmp_version ? "CC2560" :
        bt->lmp_pal_subversion == cc2560a_info.lmp_version ? "CC2560A" :
        "unknown Bluetooth chip");

    // Detected in the wild, but unclear if there is any difference.
    if (hardware_id == 0x8) {
        return eeprom_version >= 0x07 ? "2018" : "2017";
    }

    // Two variants exist for this ID, both exclusively with eeprom version
    // 0x06. These are only told apart by the Bluetooth chip. Using || here in
    // case this transition is not exact.
    if (hardware_id == 0x7 || eeprom_version == 0x06) {
        return bt->lmp_pal_subversion == cc2560a_info.lmp_version ? "2015" : "2013";
    }

    // From public sources in lms2012.h, but these may not exist in the wild.
    if (eeprom_version == 0x05) {
        return "EP3";
    }
    if (eeprom_version == 0x04) {
        return "EP2";
    }
    if (eeprom_version == 0x03) {
        return "EP1";
    }

    // Unknown.
    return NULL;
}

#endif // PBDRV_CONFIG_HARDWARE_EV3
