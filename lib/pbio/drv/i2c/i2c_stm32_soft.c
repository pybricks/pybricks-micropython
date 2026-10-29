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

// Max 255 bytes write followed by max 255 bytes read, like the EV3 driver.
#define I2C_MAX_BYTES_PER_TXN       (512)

// Number of times we try the operation before NAK is raised as an IO error.
#define I2C_MAX_NUM_TRIES_ON_NAK    (2)

// Nominal SCL frequency. Each bit takes 4 timer ticks. Effective rate is
// limited by the rise time from the internal pull-ups (~40k).
#define I2C_SCL_HZ                  (100000)
#define I2C_TICK_HZ                 (I2C_SCL_HZ * 4)

// Give up if SCL is held low (clock stretch or stuck bus) this long.
#define I2C_STRETCH_TIMEOUT_TICKS   (I2C_TICK_HZ / 20) // 50 ms

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
    uint8_t *buffer;
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
    bool is_addr;
    bool addr_read;
    uint16_t byte_idx;
    uint32_t stretch_count;
    /** Retry counter for the protothread. */
    size_t try_count;
};

static pbdrv_i2c_dev_t i2c_devs[PBDRV_CONFIG_I2C_STM32_SOFT_NUM_DEV];

static uint8_t pbdrv_i2c_buffers[PBDRV_CONFIG_I2C_STM32_SOFT_NUM_DEV][I2C_MAX_BYTES_PER_TXN];

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
    if (++dev->stretch_count > I2C_STRETCH_TIMEOUT_TICKS) {
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

    if (dev->byte_idx < dev->wlen) {
        // Write the next data byte.
        dev->cur_byte = dev->buffer[dev->byte_idx];
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
 */
static void i2c_soft_tick(pbdrv_i2c_dev_t *dev) {

    const pbdrv_gpio_t *sda = dev->sda;
    const pbdrv_gpio_t *scl = dev->scl;

    switch (dev->stage) {

        case I2C_STAGE_START:
            // (Repeated) start: SDA high, SCL high, SDA low, SCL low.
            switch (dev->phase) {
                case 0:
                    i2c_pin_high(sda);
                    dev->phase = 1;
                    break;
                case 1:
                    i2c_pin_high(scl);
                    dev->stretch_count = 0;
                    dev->phase = 2;
                    break;
                case 2:
                    if (i2c_await_scl_high(dev)) {
                        break;
                    }
                    i2c_pin_low(sda);
                    dev->phase = 3;
                    break;
                case 3:
                    i2c_pin_low(scl);
                    // Send the address byte next.
                    dev->cur_byte = (dev->addr << 1) | (dev->addr_read ? 1 : 0);
                    dev->is_addr = true;
                    dev->stage = I2C_STAGE_WRITE_BYTE;
                    dev->bit = 0;
                    dev->phase = 0;
                    break;
            }
            break;

        case I2C_STAGE_WRITE_BYTE:
            // 8 data bits MSB first, then release SDA and sample ACK.
            switch (dev->phase) {
                case 0:
                    if (dev->bit < 8) {
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
                    break;
                case 1:
                    i2c_pin_high(scl);
                    dev->stretch_count = 0;
                    dev->phase = 2;
                    break;
                case 2:
                    if (i2c_await_scl_high(dev)) {
                        break;
                    }
                    if (dev->bit == 8) {
                        // Byte is fully sent, so reuse cur_byte to latch ACK.
                        dev->cur_byte = i2c_pin_read(sda);
                    }
                    dev->phase = 3;
                    break;
                case 3:
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
                    break;
            }
            break;

        case I2C_STAGE_READ_BYTE:
            // 8 data bits MSB first, then send ACK, or NACK on the last byte.
            switch (dev->phase) {
                case 0:
                    if (dev->bit == 0) {
                        // Release SDA so the device can drive it.
                        i2c_pin_high(sda);
                    } else if (dev->bit == 8) {
                        // ACK, or NACK after the last byte.
                        if (dev->byte_idx == dev->rlen - 1) {
                            i2c_pin_high(sda);
                        } else {
                            i2c_pin_low(sda);
                        }
                    }
                    dev->phase = 1;
                    break;
                case 1:
                    i2c_pin_high(scl);
                    dev->stretch_count = 0;
                    dev->phase = 2;
                    break;
                case 2:
                    if (i2c_await_scl_high(dev)) {
                        break;
                    }
                    if (dev->bit < 8) {
                        dev->cur_byte = (dev->cur_byte << 1) | i2c_pin_read(sda);
                    }
                    dev->phase = 3;
                    break;
                case 3:
                    i2c_pin_low(scl);
                    if (dev->bit < 8) {
                        if (dev->bit == 7) {
                            dev->buffer[dev->wlen + dev->byte_idx] = dev->cur_byte;
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
                    break;
            }
            break;

        case I2C_STAGE_STOP:
            // Stop: SDA low, SCL high, SDA high.
            switch (dev->phase) {
                case 0:
                    i2c_pin_low(sda);
                    dev->phase = 1;
                    break;
                case 1:
                    i2c_pin_high(scl);
                    dev->stretch_count = 0;
                    dev->phase = 2;
                    break;
                case 2:
                    if (i2c_await_scl_high(dev)) {
                        break;
                    }
                    dev->phase = 3;
                    break;
                case 3:
                    i2c_pin_high(sda);
                    i2c_finish(dev, dev->pending_status);
                    break;
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

    if (!any_busy) {
        tim->CR1 &= ~TIM_CR1_CEN;
    }
}

static void i2c_timer_start(void) {
    TIM_TypeDef *tim = pbdrv_i2c_stm32_soft_platform_data.tim;
    if (!(tim->CR1 & TIM_CR1_CEN)) {
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

        // Ensure engine state is committed before the ISR can see is_busy.
        __asm volatile ("" ::: "memory");
        i2c_dev->is_busy = true;
        i2c_timer_start();

        // Wait for transfer to finish
        PBIO_OS_AWAIT_WHILE(state, i2c_dev->is_busy);

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

    for (int i = 0; i < PBDRV_CONFIG_I2C_STM32_SOFT_NUM_DEV; i++) {
        pbdrv_i2c_dev_t *dev = &i2c_devs[i];
        dev->buffer = pbdrv_i2c_buffers[i];
    }

    // One tick is a quarter bit. Timer counts but interrupt only fires while
    // a transaction is ongoing; it is enabled on demand.
    TIM_TypeDef *tim = pdata->tim;
    tim->PSC = 0;
    tim->ARR = pdata->tim_clock_rate / I2C_TICK_HZ - 1;
    tim->DIER = TIM_DIER_UIE;

    NVIC_SetPriority(pdata->tim_irq, 5);
    NVIC_EnableIRQ(pdata->tim_irq);
}

#endif // PBDRV_CONFIG_I2C_STM32_SOFT
