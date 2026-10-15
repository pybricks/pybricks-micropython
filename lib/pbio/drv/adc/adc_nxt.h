// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_ADC_NXT_H_
#define _INTERNAL_PBDRV_ADC_NXT_H_

// The NXT has two unrelated sources of analog values, so pbdrv_adc_get_ch()
// channels 0--7 are the AT91SAM7S256 inputs AD0--AD7 and channels 8 and up are
// the inputs sampled by the AVR co-processor.

#define PBDRV_ADC_NXT_NUM_CH_AT91 (8)

/** Sensor port @p n (0--3) pin 1, sampled by the AVR. */
#define PBDRV_ADC_NXT_CH_AVR_SENSOR(n) (PBDRV_ADC_NXT_NUM_CH_AT91 + (n))

/** Marked USB_ADC on the schematic, where it goes to pin 1 of the USB port. */
#define PBDRV_ADC_NXT_CH_USB (4)

/**
 * BC4 Bluetooth chip command/stream mode line. More than half of the 10-bit
 * range means stream mode.
 */
#define PBDRV_ADC_NXT_CH_BT_MODE (6)

#endif // _INTERNAL_PBDRV_ADC_NXT_H_
