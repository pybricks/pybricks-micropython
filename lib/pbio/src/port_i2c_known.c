// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Detects and polls a few known NXT-style I2C devices.
//
// When the device connection manager detects an I2C device, the port process
// scans for these devices. If one is found, it is polled in the background
// until it stops responding. Sensor classes read the latest polled data and
// never access the I2C bus themselves.
//
// If no known device is found, the bus is left alone so that user code can use
// it, until the device connection manager detects that it was unplugged.
//
// NB: We should not be adding third-party sensors to this driver. This can
// easily be done in user modules. This wrapper here only exists because 2 out
// of 3 official NXT sensors do not follow the NXT I2C device pin 2 convention,
// so disconnection cannot be tested on EV3 without trying I2C ops. So
// technically the Ultrasonic Sensor need not be here either, but we do include
// it anyway since it is so omnipresent.

#include <pbio/config.h>

#if PBIO_CONFIG_PORT_I2C_KNOWN

#include <string.h>

#include <pbio/port_i2c_known.h>
#include <pbio/util.h>

#define DEBUG 0
#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

/**
 * Time between polls of a known device.
 */
#define POLL_INTERVAL_MS (100)

/**
 * Number of consecutive failed polls after which the device is considered
 * disconnected. The driver already retries on NAK.
 */
#define POLL_MAX_FAILURES (3)

/**
 * Size of the largest poll of any known device.
 */
#define POLL_MAX_LEN (14)

/**
 * Scan and poll state.
 */
typedef enum {
    /**
     * Not running, so no I2C device is attached.
     */
    STATE_NONE,
    /**
     * Scanning for known devices.
     */
    STATE_SCANNING,
    /**
     * Scan done, but no known device was found. The bus is free for user code.
     */
    STATE_UNKNOWN,
    /**
     * Known device found and being polled.
     */
    STATE_KNOWN,
} pbio_port_i2c_known_state_t;

/**
 * Fixed properties of a known device.
 */
typedef struct {
    /** Type of the device. */
    lego_device_type_id_t type_id;
    /** I2C address (unshifted). */
    uint8_t address;
    /** Whether the device needs the NXT I2C transaction quirk. */
    bool nxt_quirk;
    /** Register to read when polling. */
    uint8_t poll_reg;
    /** Number of bytes to read when polling. */
    uint8_t poll_len;
    /** Power needed on pins 1 and 2. */
    pbio_port_power_requirements_t power;
} pbio_port_i2c_known_info_t;

static const pbio_port_i2c_known_info_t info_ultrasonic = {
    .type_id = LEGO_DEVICE_TYPE_ID_NXT_ULTRASONIC_SENSOR,
    .address = 0x01,
    .nxt_quirk = true,
    .poll_reg = 0x42,
    .poll_len = 1,
    .power = PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P1_POS,
};

static const pbio_port_i2c_known_info_t info_temperature = {
    .type_id = LEGO_DEVICE_TYPE_ID_NXT_TEMPERATURE_SENSOR,
    .address = 0x4C,
    .nxt_quirk = false,
    .poll_reg = 0x00,
    .poll_len = 2,
    .power = PBIO_PORT_POWER_REQUIREMENTS_NONE,
};

static const pbio_port_i2c_known_info_t info_energy_meter = {
    .type_id = LEGO_DEVICE_TYPE_ID_NXT_ENERGY_METER,
    .address = 0x02,
    .nxt_quirk = false,
    .poll_reg = 0x0A,
    .poll_len = 14,
    .power = PBIO_PORT_POWER_REQUIREMENTS_NONE,
};

struct _pbio_port_i2c_known_dev_t {
    /** Scan and poll state. */
    pbio_port_i2c_known_state_t state;
    /** The known device, if any. */
    const pbio_port_i2c_known_info_t *info;
    /** Whether data has been polled since the device was found. */
    bool has_data;
    /** Number of consecutive failed polls. */
    uint8_t num_failures;
    /** Protothread state for I2C operations. */
    pbio_os_state_t child;
    /** Read buffer of the most recent I2C operation. */
    uint8_t *rdata;
    /** Raw register data of the latest poll. */
    uint8_t data[POLL_MAX_LEN];
};

static pbio_port_i2c_known_dev_t devs[PBIO_CONFIG_PORT_I2C_KNOWN_NUM_DEV];

pbio_port_i2c_known_dev_t *pbio_port_i2c_known_init_instance(uint8_t index) {
    if (index >= PBIO_CONFIG_PORT_I2C_KNOWN_NUM_DEV) {
        return NULL;
    }
    return &devs[index];
}

/**
 * Resets the state to no device attached.
 *
 * @param [in]  dev     The known I2C device instance.
 */
void pbio_port_i2c_known_reset(pbio_port_i2c_known_dev_t *dev) {
    if (!dev) {
        return;
    }
    dev->state = STATE_NONE;
    dev->info = NULL;
    dev->has_data = false;
}

static pbio_error_t pbio_port_i2c_known_set_found(pbio_port_i2c_known_dev_t *dev, const pbio_port_i2c_known_info_t *info) {
    DEBUG_PRINT("Known I2C device: %d\n", info ? info->type_id : LEGO_DEVICE_TYPE_ID_NONE);
    dev->info = info;
    dev->state = info ? STATE_KNOWN : STATE_UNKNOWN;
    return PBIO_SUCCESS;
}

/**
 * Scans the bus for known devices and initializes them if found.
 *
 * Completes when the scan is done, whether or not a device was found. Use
 * ::pbio_port_i2c_known_is_found to check the result.
 *
 * @param [in]  state       The protothread state.
 * @param [in]  dev         The known I2C device instance.
 * @param [in]  i2c_dev     The I2C driver of this port, or NULL if unsupported.
 * @param [in]  timer       The timer to use for timing.
 * @return                  ::PBIO_SUCCESS when done.
 */
pbio_error_t pbio_port_i2c_known_scan_thread(pbio_os_state_t *state, pbio_port_i2c_known_dev_t *dev, pbdrv_i2c_dev_t *i2c_dev, pbio_os_timer_t *timer) {

    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    pbio_port_i2c_known_reset(dev);
    dev->state = STATE_SCANNING;
    DEBUG_PRINT("Start known I2C device scan\n");

    // Without an I2C driver, there is nothing to find.
    if (!i2c_dev) {
        return pbio_port_i2c_known_set_found(dev, NULL);
    }

    // NXT Ultrasonic Sensor appears to need some time after initializing I2C
    // pins before it can receive data. Give the others some time too.
    PBIO_OS_AWAIT_MS(state, timer, 100);

    // NXT Energy Meter, has no identifier registers, so try reading data. This
    // must come first, because the energy meter holds the clock line low after
    // seeing a transaction to another address that is not acknowledged. It
    // stays that way until it is switched off and on again.
    dev->rdata = NULL;
    PBIO_OS_AWAIT(state, &dev->child, err = pbdrv_i2c_write_then_read(&dev->child, i2c_dev, info_energy_meter.address,
        &info_energy_meter.poll_reg, 1, &dev->rdata, info_energy_meter.poll_len, info_energy_meter.nxt_quirk));
    DEBUG_PRINT("Energy meter: %d\n", err);
    if (err == PBIO_SUCCESS) {
        return pbio_port_i2c_known_set_found(dev, &info_energy_meter);
    }

    // NXT Ultrasonic Sensor, identified by its vendor and product strings.
    static const uint8_t ultrasonic_reg_vendor[] = { 0x08 };
    dev->rdata = NULL;
    PBIO_OS_AWAIT(state, &dev->child, err = pbdrv_i2c_write_then_read(&dev->child, i2c_dev, info_ultrasonic.address,
        ultrasonic_reg_vendor, sizeof(ultrasonic_reg_vendor), &dev->rdata, 4, info_ultrasonic.nxt_quirk));
    DEBUG_PRINT("Ultrasonic vendor: %d\n", err);
    if (err == PBIO_SUCCESS && !memcmp(dev->rdata, "LEGO", 4)) {
        static const uint8_t ultrasonic_reg_product[] = { 0x10 };
        dev->rdata = NULL;
        PBIO_OS_AWAIT(state, &dev->child, err = pbdrv_i2c_write_then_read(&dev->child, i2c_dev, info_ultrasonic.address,
            ultrasonic_reg_product, sizeof(ultrasonic_reg_product), &dev->rdata, 5, info_ultrasonic.nxt_quirk));
        DEBUG_PRINT("Ultrasonic product: %d\n", err);
        if (err == PBIO_SUCCESS && !memcmp(dev->rdata, "Sonar", 5)) {
            return pbio_port_i2c_known_set_found(dev, &info_ultrasonic);
        }
    }

    // NXT Temperature Sensor, identified by accepting the resolution setting.
    // Sets to 0.125 degrees as a fair balance between speed and accuracy.
    static const uint8_t temperature_set_resolution[] = { 0x01, (1 << 6) | (0 << 5) };
    dev->rdata = NULL;
    PBIO_OS_AWAIT(state, &dev->child, err = pbdrv_i2c_write_then_read(&dev->child, i2c_dev, info_temperature.address,
        temperature_set_resolution, sizeof(temperature_set_resolution), &dev->rdata, 0, info_temperature.nxt_quirk));
    DEBUG_PRINT("Temperature: %d\n", err);
    if (err == PBIO_SUCCESS) {
        return pbio_port_i2c_known_set_found(dev, &info_temperature);
    }

    DEBUG_PRINT("No known I2C device found\n");
    PBIO_OS_ASYNC_END(pbio_port_i2c_known_set_found(dev, NULL));
}

/**
 * Polls the known device until it stops responding.
 *
 * @param [in]  state       The protothread state.
 * @param [in]  dev         The known I2C device instance.
 * @param [in]  i2c_dev     The I2C driver of this port.
 * @param [in]  timer       The timer to use for timing.
 * @return                  ::PBIO_SUCCESS when the device stopped responding.
 */
pbio_error_t pbio_port_i2c_known_poll_thread(pbio_os_state_t *state, pbio_port_i2c_known_dev_t *dev, pbdrv_i2c_dev_t *i2c_dev, pbio_os_timer_t *timer) {

    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    for (dev->num_failures = 0; dev->num_failures < POLL_MAX_FAILURES;) {
        pbio_os_timer_set(timer, POLL_INTERVAL_MS);

        dev->rdata = NULL;
        PBIO_OS_AWAIT(state, &dev->child, err = pbdrv_i2c_write_then_read(&dev->child, i2c_dev, dev->info->address,
            &dev->info->poll_reg, 1, &dev->rdata, dev->info->poll_len, dev->info->nxt_quirk));

        if (err == PBIO_SUCCESS) {
            memcpy(dev->data, dev->rdata, dev->info->poll_len);
            dev->has_data = true;
            dev->num_failures = 0;
        } else {
            DEBUG_PRINT("Poll failed: %d\n", err);
            dev->num_failures++;
        }

        PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(timer));
    }

    DEBUG_PRINT("Known I2C device stopped responding\n");
    pbio_port_i2c_known_reset(dev);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Tests whether the scan found a known device.
 *
 * @param [in]  dev     The known I2C device instance.
 * @return              True if a known device was found.
 */
bool pbio_port_i2c_known_is_found(pbio_port_i2c_known_dev_t *dev) {
    return dev && dev->state == STATE_KNOWN;
}

/**
 * Asserts that the attached I2C device is not a known device, so that user
 * code can access the bus without conflicting with the polling process.
 *
 * Only meaningful if an I2C device was detected on the port.
 *
 * @param [in]  dev     The known I2C device instance.
 * @return              ::PBIO_SUCCESS if the device is not a known device.
 *                      ::PBIO_ERROR_AGAIN if the scan is not done yet.
 *                      ::PBIO_ERROR_NO_DEV if a known device is attached.
 */
pbio_error_t pbio_port_i2c_known_assert_unknown(pbio_port_i2c_known_dev_t *dev) {
    if (!dev) {
        return PBIO_SUCCESS;
    }
    switch (dev->state) {
        case STATE_UNKNOWN:
            return PBIO_SUCCESS;
        case STATE_KNOWN:
            return PBIO_ERROR_NO_DEV;
        default:
            return PBIO_ERROR_AGAIN;
    }
}

/**
 * Gets the raw register data last polled from a known device.
 *
 * @param [in]  dev         The known I2C device instance.
 * @param [in]  type_id     The expected type of the device.
 * @param [out] data        The raw data, as read from the poll register.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_AGAIN if still scanning or no data
 *                          has been polled yet.
 *                          ::PBIO_ERROR_NO_DEV if the expected device is not
 *                          attached.
 */
pbio_error_t pbio_port_i2c_known_get_data(pbio_port_i2c_known_dev_t *dev, lego_device_type_id_t type_id, const uint8_t **data) {
    if (!dev) {
        return PBIO_ERROR_NO_DEV;
    }
    if (dev->state == STATE_SCANNING) {
        return PBIO_ERROR_AGAIN;
    }
    if (dev->state != STATE_KNOWN || dev->info->type_id != type_id) {
        return PBIO_ERROR_NO_DEV;
    }
    if (!dev->has_data) {
        return PBIO_ERROR_AGAIN;
    }
    *data = dev->data;
    return PBIO_SUCCESS;
}

/**
 * Gets the power requirements of the known device, if any.
 *
 * @param [in]  dev     The known I2C device instance.
 * @return              The power requirements.
 */
pbio_port_power_requirements_t pbio_port_i2c_known_get_power_requirements(pbio_port_i2c_known_dev_t *dev) {
    if (!pbio_port_i2c_known_is_found(dev)) {
        return PBIO_PORT_POWER_REQUIREMENTS_NONE;
    }
    return dev->info->power;
}

/**
 * Gets the telemetry report for the attached I2C device.
 *
 * @param [in]     dev      The known I2C device instance.
 * @param [out]    tel      The packet to populate.
 * @param [in,out] size     Room available for the payload, set to the size of
 *                          the payload on success.
 * @return                  ::PBSYS_TELEMETRY_SUCCESS on success.
 */
pbsys_telemetry_error_t pbio_port_i2c_known_get_telemetry(pbio_port_i2c_known_dev_t *dev, pbsys_telemetry_packet_t *tel, uint32_t *size) {

    if (!dev) {
        return PBSYS_TELEMETRY_ERROR_NO_REPORT;
    }

    // Some other I2C device that user code may access.
    if (dev->state == STATE_UNKNOWN) {
        tel->manufacturer = PBSYS_TELEMETRY_MANUFACTURER_UKNOWN;
        tel->id = PBSYS_TELEMETRY_DEVICE_UKNOWN_I2C;
        tel->mode = 0;
        *size = 0;
        return PBSYS_TELEMETRY_SUCCESS;
    }

    if (dev->state != STATE_KNOWN || !dev->has_data) {
        return PBSYS_TELEMETRY_ERROR_NO_REPORT;
    }

    tel->id = dev->info->type_id;
    tel->mode = 0;

    switch (dev->info->type_id) {
        case LEGO_DEVICE_TYPE_ID_NXT_ULTRASONIC_SENSOR:
            // Distance in mm.
            if (*size < sizeof(uint16_t)) {
                return PBSYS_TELEMETRY_ERROR_NO_ROOM;
            }
            pbio_set_uint16_le(&tel->payload[0], dev->data[0] * 10);
            *size = sizeof(uint16_t);
            return PBSYS_TELEMETRY_SUCCESS;
        case LEGO_DEVICE_TYPE_ID_NXT_TEMPERATURE_SENSOR: {
            // Temperature in 1/16 degrees Celsius.
            if (*size < sizeof(int16_t)) {
                return PBSYS_TELEMETRY_ERROR_NO_ROOM;
            }
            int16_t combined = ((uint16_t)dev->data[0] << 8) | dev->data[1];
            pbio_set_uint16_le(&tel->payload[0], combined >> 4);
            *size = sizeof(int16_t);
            return PBSYS_TELEMETRY_SUCCESS;
        }
        case LEGO_DEVICE_TYPE_ID_NXT_ENERGY_METER:
            // Registers are already little endian 16-bit values.
            if (*size < info_energy_meter.poll_len) {
                return PBSYS_TELEMETRY_ERROR_NO_ROOM;
            }
            memcpy(&tel->payload[0], dev->data, info_energy_meter.poll_len);
            *size = info_energy_meter.poll_len;
            return PBSYS_TELEMETRY_SUCCESS;
        default:
            *size = 0;
            return PBSYS_TELEMETRY_SUCCESS;
    }
}

#endif // PBIO_CONFIG_PORT_I2C_KNOWN
