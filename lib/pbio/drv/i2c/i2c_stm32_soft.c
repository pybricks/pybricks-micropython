// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors
// Copyright (c) 2026 SparkFun Electronics

// Software (bit-banged) I2C driver for STM32 MCUs.
//
// The I/O port pins are configured as open drain outputs with internal
// pull-ups. A shared basic timer generates a periodic interrupt that advances
// a small per-bus state machine by one quarter-bit phase per tick, using only
// atomic BSRR/IDR register accesses. The pbio protothread only awaits
// completion, just like the EV3 (PRU) I2C driver.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_I2C_STM32_SOFT

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pbio/error.h>
#include <pbio/os.h>

#include <pbdrv/gpio.h>
#include <pbdrv/i2c.h>

#include "./i2c_stm32_soft.h"

#include STM32_H

#define DEBUG 0
#if DEBUG
#include <pbio/debug.h>
#define debug_pr pbio_debug
#else
#define debug_pr(...)
#endif

// Number of times we try the operation before NAK is raised as an IO error.
#define I2C_MAX_NUM_TRIES_ON_NAK    (2)

// Nominal SCL frequency. Each bit takes 4 timer ticks. Effective rate is
// limited by the rise time from the internal pull-ups (~40k).
#define I2C_SCL_HZ_DEFAULT          (100000)

// Allowed SCL frequency range. The upper limit keeps the interrupt load
// manageable. The lower limit keeps the timer period within 16 bits.
#define I2C_SCL_HZ_MIN              (1000)
#define I2C_SCL_HZ_MAX              (100000)

// Current SCL frequency, shared by all buses.
static uint32_t i2c_scl_hz;

// Give up if SCL is held low (clock stretch or stuck bus) this long. Depends
// on the tick rate, so it is set along with the frequency.
static uint32_t i2c_stretch_timeout_ticks;

// Transaction status, set by the timer interrupt on completion.
enum {
    I2C_STAT_OK,
    I2C_STAT_TIMEOUT,
    I2C_STAT_NAK,
};

// Stages of one write-then-read transaction.
enum {
    I2C_STAGE_START,
    I2C_STAGE_WRITE_BYTE,
    I2C_STAGE_READ_BYTE,
    I2C_STAGE_STOP,
};

struct _pbdrv_i2c_dev_t {
    /**
     * Transaction buffer, attached by the user. May be set to NULL from
     * thread context while the ISR is busy, which then ends the transaction
     * at the next byte boundary. Since the ISR can't be preempted by thread
     * code, it sees a consistent value for the duration of one tick.
     */
    uint8_t *volatile buffer;
    size_t buffer_size;
    void *buffer_container;
    /** Pin roles, selectable at runtime. */
    const pbdrv_gpio_t *sda;
    const pbdrv_gpio_t *scl;
    volatile bool is_busy;
    /** Final status of last transaction, one of I2C_STAT_*. */
    volatile uint8_t status;
    /** Status to report after the stop condition completes. */
    uint8_t pending_status;
    /** Transaction parameters. */
    uint8_t addr;
    uint16_t wlen;
    uint16_t rlen;
    /** Bit engine state, only touched by the ISR while busy. */
    uint8_t stage;
    uint8_t phase;
    uint8_t bit;
    uint8_t cur_byte;
    /** True while cur_byte is the address byte rather than a data byte. */
    bool is_addr;
    /**
     * R/W bit for the next address byte, high for read. Also tells
     * i2c_next_after_write that the address just sent was the read address,
     * so data reading starts next.
     */
    bool addr_read;
    uint16_t byte_idx;
    uint32_t stretch_count;
    /** Retry counter for the protothread. */
    size_t try_count;
};

static pbdrv_i2c_dev_t i2c_devs[PBDRV_CONFIG_I2C_STM32_SOFT_NUM_DEV];

pbio_error_t pbdrv_i2c_get_instance(uint8_t id, pbdrv_i2c_dev_t **i2c_dev) {
    if (id >= PBDRV_CONFIG_I2C_STM32_SOFT_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }
    pbdrv_i2c_dev_t *dev = &i2c_devs[id];
    *i2c_dev = dev;
    return PBIO_SUCCESS;
}

// Pin helpers. Pins are open drain, so "high" means released.
// BSRR writes and IDR reads are atomic, making them safe in both
// interrupt and thread context.

static void i2c_pin_high(const pbdrv_gpio_t *gpio) {
    ((GPIO_TypeDef *)gpio->bank)->BSRR = 1 << gpio->pin;
}

static void i2c_pin_low(const pbdrv_gpio_t *gpio) {
    ((GPIO_TypeDef *)gpio->bank)->BSRR = (1 << 16) << gpio->pin;
}

static bool i2c_pin_read(const pbdrv_gpio_t *gpio) {
    return ((GPIO_TypeDef *)gpio->bank)->IDR & (1 << gpio->pin);
}

static void i2c_pin_config_open_drain(const pbdrv_gpio_t *gpio) {
    GPIO_TypeDef *bank = gpio->bank;
    bank->BSRR = 1 << gpio->pin;
    bank->OTYPER |= 1 << gpio->pin;
    bank->PUPDR = (bank->PUPDR & ~(3 << (gpio->pin * 2))) | (PBDRV_GPIO_PULL_UP << (gpio->pin * 2));
    bank->MODER = (bank->MODER & ~(3 << (gpio->pin * 2))) | (1 << (gpio->pin * 2));
}

void pbdrv_i2c_configure_port(pbdrv_i2c_dev_t *i2c_dev, const pbdrv_gpio_t *scl, const pbdrv_gpio_t *sda) {
    // Must be called before port is used.
    i2c_dev->scl = scl;
    i2c_dev->sda = sda;
    i2c_pin_config_open_drain(i2c_dev->scl);
    i2c_pin_config_open_drain(i2c_dev->sda);
}

void pbdrv_i2c_release_port(pbdrv_i2c_dev_t *i2c_dev) {
    // Not all ports have an I2C device, and this may be called before
    // configuring the port.
    if (!i2c_dev || !i2c_dev->scl || !i2c_dev->sda) {
        return;
    }

    // Restore push-pull, which other port modes expect when driving the pins.
    // The caller resets the pin mode and pull but the pin pointers are kept.
    ((GPIO_TypeDef *)i2c_dev->scl->bank)->OTYPER &= ~(1 << i2c_dev->scl->pin);
    ((GPIO_TypeDef *)i2c_dev->sda->bank)->OTYPER &= ~(1 << i2c_dev->sda->pin);
}

void pbdrv_i2c_set_buffer(pbdrv_i2c_dev_t *i2c_dev, void *container, uint8_t *buffer, size_t size) {
    i2c_dev->buffer_size = size;
    i2c_dev->buffer_container = container;
    i2c_dev->buffer = buffer;
}

void *pbdrv_i2c_get_buffer_container(pbdrv_i2c_dev_t *i2c_dev) {
    return i2c_dev->buffer_container;
}

/**
 * Finishes a transaction and wakes up the awaiting protothread.
 */
static void i2c_finish(pbdrv_i2c_dev_t *dev, uint8_t status) {
    debug_pr("i2c fin: stat=%u stage=%u is_addr=%u byte=%u bit=%u\n",
        status, dev->stage, dev->is_addr, dev->byte_idx, dev->bit);
    dev->status = status;
    dev->is_busy = false;
    pbio_os_request_poll();
}

/**
 * Called when SCL was released but reads back low: a device is stretching
 * the clock or the bus is stuck. Returns true if we should keep waiting.
 */
static bool i2c_await_scl_high(pbdrv_i2c_dev_t *dev) {
    if (i2c_pin_read(dev->scl)) {
        return false;
    }
    if (++dev->stretch_count > i2c_stretch_timeout_ticks) {
        // Bus stuck; release everything and give up.
        i2c_pin_high(dev->sda);
        i2c_pin_high(dev->scl);
        i2c_finish(dev, I2C_STAT_TIMEOUT);
    }
    return true;
}

/**
 * Decides what to do after a byte was successfully written (ACK received).
 */
static void i2c_next_after_write(pbdrv_i2c_dev_t *dev) {
    // Increment data index if there is anything left to write unless
    // we only just write the address. Either way, next won't be an address.
    if (!dev->is_addr && dev->byte_idx < dev->wlen) {
        dev->byte_idx++;
    }
    dev->is_addr = false;

    if (dev->addr_read) {
        // Read address was just acknowledged; start reading data.
        dev->stage = I2C_STAGE_READ_BYTE;
        dev->byte_idx = 0;
        dev->cur_byte = 0;
        dev->bit = 0;
        dev->phase = 0;
        return;
    }

    uint8_t *buffer = dev->buffer;
    if (!buffer) {
        // Buffer was detached. A stop after an acknowledged byte is always
        // valid, so end the transaction here to leave the bus idle.
        dev->pending_status = I2C_STAT_OK;
        dev->stage = I2C_STAGE_STOP;
        dev->phase = 0;
        return;
    }

    if (dev->byte_idx < dev->wlen) {
        // Write the next data byte.
        dev->cur_byte = buffer[dev->byte_idx];
        dev->stage = I2C_STAGE_WRITE_BYTE;
        dev->bit = 0;
        dev->phase = 0;
        return;
    }

    if (dev->rlen) {
        // All written; switch to reading with a repeated start.
        dev->addr_read = true;
        dev->stage = I2C_STAGE_START;
        dev->phase = 0;
        return;
    }

    // Write-only (or ping) transaction complete.
    dev->pending_status = I2C_STAT_OK;
    dev->stage = I2C_STAGE_STOP;
    dev->phase = 0;
}

/**
 * Advances the bus state machine by one quarter-bit phase.
 *
 * Each bit, start and stop takes four phases, one per tick. SCL is released
 * in phase 1 and awaited in phase 2. If the device stretches the clock,
 * phase 2 repeats until SCL actually reads high.
 */
static void i2c_soft_tick(pbdrv_i2c_dev_t *dev) {

    const pbdrv_gpio_t *sda = dev->sda;
    const pbdrv_gpio_t *scl = dev->scl;

    switch (dev->stage) {

        // Start: SDA high, SCL high, SDA low, SCL low.
        //
        // From an idle bus, both lines are already high, so phases 0 and 1
        // change nothing:
        //
        //     phase  0     1     2     3
        //            |     |     |     |
        //     SCL  ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯\___
        //     SDA  ¯¯¯¯¯¯¯¯¯¯¯¯¯¯\_________
        //                        ^ start
        //
        // After the device's ACK to the last written byte, SCL is low and
        // SDA may be either: the device releases its ACK some time after SCL
        // falls. This is a repeated start:
        //
        //     phase  0     1     2     3
        //            |     |     |     |
        //     SCL  ________/¯¯¯¯¯¯¯¯¯¯¯\___
        //     SDA  ==X¯¯¯¯¯¯¯¯¯¯¯¯\_________
        //                        ^ repeated start
        case I2C_STAGE_START:
            switch (dev->phase) {
                case 0:
                    // Release SDA. On an idle bus it's already high. For a
                    // repeated start, SCL is still low, so SDA may rise
                    // without it being a stop.
                    i2c_pin_high(sda);
                    dev->phase = 1;
                    return;
                case 1:
                    i2c_pin_high(scl);
                    dev->stretch_count = 0;
                    dev->phase = 2;
                    return;
                case 2:
                    if (i2c_await_scl_high(dev)) {
                        return;
                    }
                    // SDA falls while SCL is high: this is the start condition.
                    i2c_pin_low(sda);
                    dev->phase = 3;
                    return;
                case 3:
                    // Pull SCL low, ready for the first bit.
                    i2c_pin_low(scl);
                    // Send the address byte next.
                    dev->cur_byte = (dev->addr << 1) | (dev->addr_read ? 1 : 0);
                    dev->is_addr = true;
                    dev->stage = I2C_STAGE_WRITE_BYTE;
                    dev->bit = 0;
                    dev->phase = 0;
                    return;
            }
            break;

        // Write byte: 8 data bits MSB first, then release SDA and sample ACK.
        // Each bit, starting just after the previous falling edge of SCL:
        //
        //     phase  0     1     2     3     0
        //            |     |     |     |     |
        //     SCL  ________/¯¯¯¯¯¯¯¯¯¯¯\__________
        //     SDA  ==X=======================X===
        //            ^ we put the bit on SDA
        //                        ^ device samples
        //
        // The "9th bit" (bit == 8) is the ACK, where the roles swap: we release
        // SDA in phase 0 and sample the device's ACK in phase 2.
        case I2C_STAGE_WRITE_BYTE:
            switch (dev->phase) {
                case 0:
                    if (dev->bit < 8) {
                        // Put the data bit on SDA while SCL is low.
                        if ((dev->cur_byte >> (7 - dev->bit)) & 1) {
                            i2c_pin_high(sda);
                        } else {
                            i2c_pin_low(sda);
                        }
                    } else {
                        // Release SDA so the device can ACK.
                        i2c_pin_high(sda);
                    }
                    dev->phase = 1;
                    return;
                case 1:
                    i2c_pin_high(scl);
                    dev->stretch_count = 0;
                    dev->phase = 2;
                    return;
                case 2:
                    if (i2c_await_scl_high(dev)) {
                        return;
                    }
                    // The device samples SDA while SCL is high, except on the
                    // ACK bit, where we sample it.
                    if (dev->bit == 8) {
                        // Byte is fully sent, so reuse cur_byte to latch ACK.
                        dev->cur_byte = i2c_pin_read(sda);
                    }
                    dev->phase = 3;
                    return;
                case 3:
                    // Pull SCL low, so SDA may change for the next bit. After
                    // the ACK bit, the device releases SDA.
                    i2c_pin_low(scl);
                    if (dev->bit < 8) {
                        dev->bit++;
                        dev->phase = 0;
                    } else if (dev->cur_byte) {
                        // NAK. Finish with a clean stop.
                        debug_pr("i2c nak: is_addr=%u read=%u byte=%u\n",
                            dev->is_addr, dev->addr_read, dev->byte_idx);
                        dev->pending_status = I2C_STAT_NAK;
                        dev->stage = I2C_STAGE_STOP;
                        dev->phase = 0;
                    } else {
                        i2c_next_after_write(dev);
                    }
                    return;
            }
            break;

        // Read byte: 8 data bits MSB first, then send ACK, or NACK on the last
        // byte. Each bit, starting just after the previous falling edge of SCL:
        //
        //     phase  0     1     2     3     0
        //            |     |     |     |     |
        //     SCL  ________/¯¯¯¯¯¯¯¯¯¯¯\__________
        //     SDA  ==X=======================X===
        //            ^ device puts the bit on SDA
        //                        ^ we sample
        //
        // The device may change SDA anywhere while SCL is low, not just in
        // phase 0. The 9th bit (bit == 8) is the ACK, where the roles swap:
        // we put our ACK (or NACK) on SDA in phase 0 and the device samples
        // it in phase 2.
        case I2C_STAGE_READ_BYTE:
            switch (dev->phase) {
                case 0:
                    if (dev->bit == 0) {
                        // Release our ACK of the previous byte, a full tick
                        // after the SCL falling edge (see phase 3). The device
                        // may already be driving its first bit, but a 1 only
                        // shows on SDA once we let go. For the first byte,
                        // SDA is already released, so this changes nothing.
                        i2c_pin_high(sda);
                    } else if (dev->bit == 8) {
                        // If the buffer was detached, make this the last byte.
                        // The device must see a NACK before we send a stop.
                        if (!dev->buffer) {
                            dev->rlen = dev->byte_idx + 1;
                        }
                        // ACK, or NACK after the last byte.
                        if (dev->byte_idx == dev->rlen - 1) {
                            i2c_pin_high(sda);
                        } else {
                            i2c_pin_low(sda);
                        }
                    }
                    dev->phase = 1;
                    return;
                case 1:
                    i2c_pin_high(scl);
                    dev->stretch_count = 0;
                    dev->phase = 2;
                    return;
                case 2:
                    if (i2c_await_scl_high(dev)) {
                        return;
                    }
                    // Sample SDA while SCL is high, except on the ACK bit,
                    // where the device samples it.
                    if (dev->bit < 8) {
                        dev->cur_byte = (dev->cur_byte << 1) | i2c_pin_read(sda);
                    }
                    dev->phase = 3;
                    return;
                case 3:
                    // Pull SCL low, so the device may put the next bit on SDA.
                    i2c_pin_low(scl);
                    if (dev->bit < 8) {
                        uint8_t *buffer = dev->buffer;
                        if (dev->bit == 7 && buffer) {
                            buffer[dev->wlen + dev->byte_idx] = dev->cur_byte;
                        }
                        dev->bit++;
                        dev->phase = 0;
                    } else {
                        // NB: Do not release SDA (ACK) here: it would rise
                        // while the device may still see SCL as high, reading
                        // it as a stop condition. The next phase releases it
                        // a full tick after the SCL falling edge.
                        dev->byte_idx++;
                        if (dev->byte_idx < dev->rlen) {
                            dev->cur_byte = 0;
                            dev->bit = 0;
                            dev->phase = 0;
                        } else {
                            dev->pending_status = I2C_STAT_OK;
                            dev->stage = I2C_STAGE_STOP;
                            dev->phase = 0;
                        }
                    }
                    return;
            }
            break;

        // Stop: SDA low, SCL high, SDA high.
        //
        //     phase  0     1     2     3
        //            |     |     |     |
        //     SCL  ________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
        //     SDA  ==X_________________/¯¯¯
        //                              ^ stop
        case I2C_STAGE_STOP:
            switch (dev->phase) {
                case 0:
                    // Pull SDA low while SCL is low, so it can rise later.
                    i2c_pin_low(sda);
                    dev->phase = 1;
                    return;
                case 1:
                    i2c_pin_high(scl);
                    dev->stretch_count = 0;
                    dev->phase = 2;
                    return;
                case 2:
                    if (i2c_await_scl_high(dev)) {
                        return;
                    }
                    dev->phase = 3;
                    return;
                case 3:
                    // SDA rises while SCL is high: this is the stop condition.
                    // Both lines are now high, so the bus is idle.
                    i2c_pin_high(sda);
                    i2c_finish(dev, dev->pending_status);
                    return;
            }
            break;
    }
}

void pbdrv_i2c_stm32_soft_handle_timer_irq(void) {
    TIM_TypeDef *tim = pbdrv_i2c_stm32_soft_platform_data.tim;
    tim->SR = 0;

    bool any_busy = false;
    for (int i = 0; i < PBDRV_CONFIG_I2C_STM32_SOFT_NUM_DEV; i++) {
        pbdrv_i2c_dev_t *dev = &i2c_devs[i];
        if (dev->is_busy) {
            i2c_soft_tick(dev);
            any_busy |= dev->is_busy;
        }
    }

    // The timer is shared by all buses, so only stop it once all are idle.
    // The next transaction restarts it with i2c_timer_start.
    if (!any_busy) {
        tim->CR1 &= ~TIM_CR1_CEN;
    }
}

static bool i2c_timer_is_running(void) {
    return pbdrv_i2c_stm32_soft_platform_data.tim->CR1 & TIM_CR1_CEN;
}

/**
 * Sets the timer period for the given SCL frequency. Timer must be stopped.
 */
static void i2c_timer_set_frequency(uint32_t scl_hz) {
    const pbdrv_i2c_stm32_soft_platform_data_t *pdata = &pbdrv_i2c_stm32_soft_platform_data;
    uint32_t tick_hz = scl_hz * 4;
    // Round up so SCL is never faster than requested. This matters most on
    // platforms with a low timer clock rate.
    pdata->tim->ARR = (pdata->tim_clock_rate + tick_hz - 1) / tick_hz - 1;
    i2c_stretch_timeout_ticks = tick_hz / 20; // 50 ms
    i2c_scl_hz = scl_hz;
}

pbio_error_t pbdrv_i2c_set_frequency(uint32_t frequency) {
    if (frequency == 0) {
        frequency = I2C_SCL_HZ_DEFAULT;
    }
    if (frequency < I2C_SCL_HZ_MIN || frequency > I2C_SCL_HZ_MAX) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Nothing to do, so not an error even if busy.
    if (frequency == i2c_scl_hz) {
        return PBIO_SUCCESS;
    }

    // The timer is shared by all buses, so it can't change mid-transaction
    // on any of them.
    if (i2c_timer_is_running()) {
        return PBIO_ERROR_BUSY;
    }

    i2c_timer_set_frequency(frequency);
    return PBIO_SUCCESS;
}

static void i2c_timer_start(void) {
    TIM_TypeDef *tim = pbdrv_i2c_stm32_soft_platform_data.tim;

    // Only start if not already running for another bus, so that resetting
    // the counter doesn't disturb its bit timing.
    if (!i2c_timer_is_running()) {
        tim->CNT = 0;
        tim->SR = 0;
        tim->CR1 |= TIM_CR1_CEN;
    }
}

pbio_error_t pbdrv_i2c_write_then_read(
    pbio_os_state_t *state,
    pbdrv_i2c_dev_t *i2c_dev,
    uint8_t dev_addr,
    const uint8_t *wdata,
    size_t wlen,
    uint8_t **rdata,
    size_t rlen,
    bool nxt_quirk) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (nxt_quirk) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }
    if (wlen && !wdata) {
        return PBIO_ERROR_INVALID_ARG;
    }
    if (*rdata) {
        return PBIO_ERROR_INVALID_ARG;
    }
    if (wlen > 0xff || rlen > 0xff) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (i2c_dev->is_busy) {
        return PBIO_ERROR_BUSY;
    }

    if (!i2c_dev->buffer) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Storing read data after write data rather than overriding so
    // we can retry writing if needed.
    if (wlen + rlen > i2c_dev->buffer_size) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Prepare TX data
    if (wlen) {
        memcpy(i2c_dev->buffer, wdata, wlen);
    }

    for (i2c_dev->try_count = 0; i2c_dev->try_count < I2C_MAX_NUM_TRIES_ON_NAK; i2c_dev->try_count++) {

        // Arm the state machine and kick off the transfer.
        i2c_dev->addr = dev_addr;
        i2c_dev->wlen = wlen;
        i2c_dev->rlen = rlen;
        i2c_dev->addr_read = (wlen == 0 && rlen > 0);
        i2c_dev->byte_idx = 0;
        i2c_dev->stage = I2C_STAGE_START;
        i2c_dev->phase = 0;

        debug_pr("i2c go: addr=0x%02x w=%u r=%u sda=%u scl=%u\n",
            dev_addr, (unsigned)wlen, (unsigned)rlen,
            i2c_pin_read(i2c_dev->sda), i2c_pin_read(i2c_dev->scl));

        // Compiler barrier: the engine state above is not volatile, so without
        // this the compiler could reorder it past the is_busy store, and the
        // ISR could run with stale state.
        __asm volatile ("" ::: "memory");
        i2c_dev->is_busy = true;
        i2c_timer_start();

        // Wait for transfer to finish
        PBIO_OS_AWAIT_WHILE(state, i2c_dev->is_busy);

        // Buffer was detached while busy, so the result is lost.
        if (!i2c_dev->buffer) {
            return PBIO_ERROR_CANCELED;
        }

        if (i2c_dev->status == I2C_STAT_OK) {
            if (rlen) {
                *rdata = &i2c_dev->buffer[wlen];
            }
            return PBIO_SUCCESS;
        } else if (i2c_dev->status == I2C_STAT_TIMEOUT) {
            return PBIO_ERROR_TIMEDOUT;
        }
        // Otherwise NAK. Retrying occasionally helps, as on EV3.
    }

    // Didn't succeed even after trying allowed number of times.
    PBIO_OS_ASYNC_END(PBIO_ERROR_IO);
}

void pbdrv_i2c_init(void) {
    const pbdrv_i2c_stm32_soft_platform_data_t *pdata = &pbdrv_i2c_stm32_soft_platform_data;

    // One tick is a quarter bit. The update interrupt stays enabled, but the
    // counter only runs while a (any) transaction is ongoing; it is started on
    // demand and stopped when all buses are idle.
    TIM_TypeDef *tim = pdata->tim;
    tim->PSC = 0;
    i2c_timer_set_frequency(I2C_SCL_HZ_DEFAULT);
    tim->DIER = TIM_DIER_UIE;

    NVIC_SetPriority(pdata->tim_irq, 5);
    NVIC_EnableIRQ(pdata->tim_irq);
}

#endif // PBDRV_CONFIG_I2C_STM32_SOFT
