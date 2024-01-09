// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdint.h>
#include <stdbool.h>
#include <pbio/util.h>
#include <pbio/lwp3.h>
#include <pbdrv/bluetooth.h>

/**
 * LEGO Wireless Protocol v3 Hub Service UUID.
 *
 * 00001623-1212-EFDE-1623-785FEABCD123
 */
const uint8_t pbio_lwp3_hub_service_uuid[] = {
    0x00, 0x00, 0x16, 0x23, 0x12, 0x12, 0xEF, 0xDE,
    0x16, 0x23, 0x78, 0x5F, 0xEA, 0xBC, 0xD1, 0x23,
};

/**
 * LEGO Wireless Protocol v3 Hub Characteristic UUID.
 *
 * 00001624-1212-EFDE-1623-785FEABCD123
 */
const uint8_t pbio_lwp3_hub_char_uuid[] = {
    0x00, 0x00, 0x16, 0x24, 0x12, 0x12, 0xEF, 0xDE,
    0x16, 0x23, 0x78, 0x5F, 0xEA, 0xBC, 0xD1, 0x23,
};

/*
 * Check if the advertisement matches the LEGO Wireless Protocol v3 Hub Service.
 *
 * @param event_type    The type of advertisement.
 * @param data          The advertisement data.
 * @param hub_kind      The kind of hub to match.
 * @return              The result of the match.
 */
pbdrv_bluetooth_ad_match_result_type_t pbio_lwp3_advertisement_matches(uint8_t event_type, const uint8_t *data, lwp3_hub_kind_t hub_kind) {
    bool match =
        event_type == PBDRV_BLUETOOTH_AD_TYPE_ADV_IND
        && data[3] == 17 /* length */
        && data[4] == PBDRV_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_COMPLETE_LIST
        && pbio_uuid128_reverse_compare(&data[5], pbio_lwp3_hub_service_uuid)
        && data[26] == hub_kind;
    return match ? PBDRV_BLUETOOTH_AD_MATCH_SUCCESS : PBDRV_BLUETOOTH_AD_MATCH_FAIL;
}
