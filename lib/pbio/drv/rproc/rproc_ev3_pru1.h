// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_RPROC_EV3_PRU1_H_
#define _INTERNAL_PBDRV_RPROC_EV3_PRU1_H_

#include <stdint.h>

// This file is shared between the PRU1 source code
// and the Pybricks source code. It defines the ABI
// between the two codebases and must be kept in sync.

#define PBDRV_RPROC_EV3_PRU1_NUM_PWM_CHANNELS   4
#define PBDRV_RPROC_EV3_PRU1_NUM_I2C_BUSES      4

#define PBDRV_RPROC_EV3_PRU1_I2C_CLK_SPEED_HZ   10000

// I2C command bits, valid when bit7 == 0
// Start an I2C transaction
#define PBDRV_RPROC_EV3_PRU1_I2C_CMD_START          (1 << 0)
// Generate a stop, clock pulse, and start instead of a repeated start
// Used for the NXT ultrasonic sensor.
#define PBDRV_RPROC_EV3_PRU1_I2C_CMD_NXT_QUIRK      (1 << 1)

// I2C status bits, valid when bit7 == 1
// Indicates transaction is complete
#define PBDRV_RPROC_EV3_PRU1_I2C_STAT_DONE          (1 << 7)
// Mask for the status code
#define PBDRV_RPROC_EV3_PRU1_I2C_STAT_MASK          0x7f

// I2C transaction status codes
enum {
    PBDRV_RPROC_EV3_PRU1_I2C_STAT_OK,
    PBDRV_RPROC_EV3_PRU1_I2C_STAT_TIMEOUT,
    PBDRV_RPROC_EV3_PRU1_I2C_STAT_NAK,
};

#define PBDRV_RPROC_EV3_PRU1_I2C_PACK_FLAGS(daddr, rlen, wlen, flags)   \
    ((((daddr) & 0xff) << 24) |                                         \
    (((rlen) & 0xff) << 16) |                                           \
    (((wlen) & 0xff) << 8) |                                            \
    ((flags) & 0xff))

typedef struct {
    // bit[7:0]     status or flags
    // bit[15:8]    write length
    // bit[23:16]   read length
    // bit[31:24]   device address (unshifted)
    uint32_t flags;
    // Physical address of a transaction buffer
    uintptr_t buffer;
} pbdrv_rproc_ev3_pru1_i2c_command_t;

typedef struct {
    union {
        // u32 used by the PRU codebase to get more-efficient codegen
        uint32_t pwms;
        // PWM duty cycle for each channel, range [0-255]
        uint8_t pwm_duty_cycle[PBDRV_RPROC_EV3_PRU1_NUM_PWM_CHANNELS];
    };
    // Because the PRU needs to manipulate the GPIO direction
    // for doing I2C, and because these registers do *not*
    // support atomic bit access, we give the PRU full ownership
    // of them and route ARM accesses through the PRU.
    uint32_t gpio_bank_01_dir_set;
    uint32_t gpio_bank_01_dir_clr;
    pbdrv_rproc_ev3_pru1_i2c_command_t i2c[PBDRV_RPROC_EV3_PRU1_NUM_I2C_BUSES];
} pbdrv_rproc_ev3_pru1_shared_ram_t;

#endif // _INTERNAL_PBDRV_RPROC_EV3_PRU1_H_
