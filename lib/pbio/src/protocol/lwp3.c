// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdint.h>
#include <stdbool.h>
#include <pbio/util.h>
#include <pbio/lwp3.h>

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

// TODO: Get these from 3 different BLE APIs.
#define ADV_IND (0x00) // Connectable undirected advertisement
#define GAP_ADTYPE_128BIT_COMPLETE (0x07)

/*
 * Check if the advertisement matches the LEGO Wireless Protocol v3 Hub Service.
 *
 * @param event_type    The type of advertisement.
 * @param data          The advertisement data.
 * @param hub_kind      The kind of hub to match.
 * @return              True if the advertisement matches, false otherwise.
 */
bool pbio_lwp3_advertisement_matches(uint8_t event_type, const uint8_t *data, lwp3_hub_kind_t hub_kind) {
    return
        event_type == ADV_IND
        && data[3] == 17 /* length */
        && data[4] == GAP_ADTYPE_128BIT_COMPLETE
        && pbio_uuid128_reverse_compare(&data[5], pbio_lwp3_hub_service_uuid)
        && data[26] == hub_kind;
}
