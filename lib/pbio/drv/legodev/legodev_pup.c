// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LEGODEV_PUP

#define DEBUG (0)

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include <contiki.h>

#include <pbdrv/gpio.h>

#include <pbsys/status.h>

#include <pbio/angle.h>
#include <pbio/dcmotor.h>
#include <pbio/error.h>
#include <pbdrv/legodev.h>
#include <pbio/port.h>

#include <pbdrv/counter.h>
#include <pbdrv/legodev.h>
#include "../ioport/ioport_pup.h"

#include "legodev_pup.h"
#include "legodev_pup_uart.h"

/** The number of consecutive repeated detections needed for an affirmative ID. */
#define AFFIRMATIVE_MATCH_COUNT 20

typedef enum {
    DEV_ID_GROUP_GND,
    DEV_ID_GROUP_VCC,
    DEV_ID_GROUP_PULL_DOWN,
    DEV_ID_GROUP_OPEN,
} dev_id_group_t;

// Device connection manager state for each port
typedef struct {
    dev_id_group_t dev_id1_group;
    pbdrv_legodev_type_id_t type_id;
    pbdrv_legodev_type_id_t prev_type_id;
    uint8_t gpio_value;
    uint8_t prev_gpio_value;
    uint8_t dev_id_match_count;
} dcm_data_t;

typedef struct {
    const pbdrv_ioport_pup_pins_t *pins;
    const pbdrv_legodev_pup_ext_platform_data_t *pdata;
    pbdrv_legodev_pup_uart_dev_t *uart_dev;
    dcm_data_t dcm;
    struct pt pt;
    pbdrv_legodev_type_id_t connected_type_id;
    pbdrv_legodev_type_id_t prev_type_id;
    pbio_angle_t angle;
} ext_dev_t;

static ext_dev_t ext_devs[PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV];

static const pbdrv_legodev_type_id_t legodev_pup_type_id_lookup[3][3] = {
    /* ID1 */ [DEV_ID_GROUP_GND] = {
        /* ID2 */ [DEV_ID_GROUP_GND] = PBDRV_LEGODEV_TYPE_ID_LPF2_POWER,
        /* ID2 */ [DEV_ID_GROUP_VCC] = PBDRV_LEGODEV_TYPE_ID_LPF2_TURN,
        /* ID2 */ [DEV_ID_GROUP_PULL_DOWN] = PBDRV_LEGODEV_TYPE_ID_LPF2_LIGHT2,
    },
    /* ID1 */ [DEV_ID_GROUP_VCC] = {
        /* ID2 */ [DEV_ID_GROUP_GND] = PBDRV_LEGODEV_TYPE_ID_LPF2_TRAIN,
        /* ID2 */ [DEV_ID_GROUP_VCC] = PBDRV_LEGODEV_TYPE_ID_LPF2_LMOTOR,
        /* ID2 */ [DEV_ID_GROUP_PULL_DOWN] = PBDRV_LEGODEV_TYPE_ID_LPF2_LIGHT1,
    },
    /* ID1 */ [DEV_ID_GROUP_PULL_DOWN] = {
        /* ID2 */ [DEV_ID_GROUP_GND] = PBDRV_LEGODEV_TYPE_ID_LPF2_MMOTOR,
        /* ID2 */ [DEV_ID_GROUP_VCC] = PBDRV_LEGODEV_TYPE_ID_LPF2_XMOTOR,
        /* ID2 */ [DEV_ID_GROUP_PULL_DOWN] = PBDRV_LEGODEV_TYPE_ID_LPF2_LIGHT,
    },
};

PROCESS(pbio_legodev_pup_process, "I/O port");

static void legodev_pup_enable_uart(const pbdrv_ioport_pup_pins_t *pins) {
    // REVISIT: Move to ioport.
    pbdrv_gpio_alt(&pins->uart_rx, pins->uart_alt);
    pbdrv_gpio_alt(&pins->uart_tx, pins->uart_alt);
    pbdrv_gpio_out_low(&pins->uart_buf);
}

// This is the device connection manager (dcm). It monitors the ID1 and ID2 pins
// on the port to see when devices are connected or disconnected.
// It is expected for there to be a 2ms delay between calls to this function.
static PT_THREAD(poll_dcm(ext_dev_t * dev)) {
    struct pt *pt = &dev->pt;
    dcm_data_t *data = &dev->dcm;
    const pbdrv_ioport_pup_pins_t *pins = dev->pins;

    PT_BEGIN(pt);

    data->type_id = PBDRV_LEGODEV_TYPE_ID_NONE;
    data->dev_id1_group = DEV_ID_GROUP_OPEN;

    // set ID1 high
    pbdrv_gpio_out_high(&pins->uart_tx);
    pbdrv_gpio_out_low(&pins->uart_buf);

    // set ID2 as input
    pbdrv_gpio_input(&pins->gpio2);

    PT_YIELD(pt);

    // save current ID2 value
    data->prev_gpio_value = pbdrv_gpio_input(&pins->gpio2);

    // set ID1 low
    pbdrv_gpio_out_low(&pins->uart_tx);

    PT_YIELD(pt);

    // read ID2
    data->gpio_value = pbdrv_gpio_input(&pins->gpio2);

    // if ID2 changed from high to low
    if (data->prev_gpio_value == 1 && data->gpio_value == 0) {
        // we have touch sensor
        data->type_id = PBDRV_LEGODEV_TYPE_ID_LPF2_TOUCH;

        // set ID1 as input
        pbdrv_gpio_out_high(&pins->uart_buf);
        pbdrv_gpio_input(&pins->uart_tx);

        PT_YIELD(pt);

        // ID1 is inverse of touch sensor value
        // TODO: save this value to sensor data
        // sensor_data = !pbdrv_gpio_input(&pins->gpio1);
    }
    // if ID2 changed from low to high
    else if (data->prev_gpio_value == 0 && data->gpio_value == 1) {
        data->type_id = PBDRV_LEGODEV_TYPE_ID_LPF2_TPOINT;
    } else {
        // read ID1
        data->prev_gpio_value = pbdrv_gpio_input(&pins->gpio1);

        // set ID1 high
        pbdrv_gpio_out_high(&pins->uart_tx);

        PT_YIELD(pt);

        // read ID1
        data->gpio_value = pbdrv_gpio_input(&pins->gpio1);

        // if ID1 did not change and is high
        if (data->prev_gpio_value == 1 && data->gpio_value == 1) {
            // we have ID1 == VCC
            data->dev_id1_group = DEV_ID_GROUP_VCC;
        }
        // if ID1 did not change and is low
        else if (data->prev_gpio_value == 0 && data->gpio_value == 0) {
            // we have ID1 == GND
            data->dev_id1_group = DEV_ID_GROUP_GND;
        } else {
            // set ID1 as input
            pbdrv_gpio_out_high(&pins->uart_buf);
            pbdrv_gpio_input(&pins->uart_tx);

            PT_YIELD(pt);

            // read ID1
            if (pbdrv_gpio_input(&pins->gpio1) == 1) {
                // we have ID1 == open
                data->dev_id1_group = DEV_ID_GROUP_OPEN;
            } else {
                // we have ID1 == pull down
                data->dev_id1_group = DEV_ID_GROUP_PULL_DOWN;
            }
        }

        PT_YIELD(pt);

        // set ID1 as input
        pbdrv_gpio_out_high(&pins->uart_buf);
        pbdrv_gpio_input(&pins->uart_tx);

        // set ID2 high
        pbdrv_gpio_out_high(&pins->gpio2);

        PT_YIELD(pt);

        // read ID1
        data->prev_gpio_value = pbdrv_gpio_input(&pins->gpio1);

        // set ID2 low
        pbdrv_gpio_out_low(&pins->gpio2);

        PT_YIELD(pt);

        // read ID1
        data->gpio_value = pbdrv_gpio_input(&pins->gpio1);

        // if ID1 changed from high to low
        if (data->prev_gpio_value == 1 && data->gpio_value == 0) {
            // if we have ID1 = open
            if (data->dev_id1_group == DEV_ID_GROUP_OPEN) {
                // then we have this
                data->type_id = PBDRV_LEGODEV_TYPE_ID_LPF2_3_PART;
            }
        }
        // if ID1 changed from low to high
        else if (data->prev_gpio_value == 0 && data->gpio_value == 1) {
            // something might explode
            data->type_id = PBDRV_LEGODEV_TYPE_ID_LPF2_EXPLOD;
        } else {
            // set ID1 high
            pbdrv_gpio_out_high(&pins->uart_tx);
            pbdrv_gpio_out_low(&pins->uart_buf);

            // set ID2 high
            pbdrv_gpio_out_high(&pins->gpio2);

            PT_YIELD(pt);

            // if ID2 is high
            if (pbdrv_gpio_input(&pins->uart_rx) == 1) {
                // set ID2 low
                pbdrv_gpio_out_low(&pins->gpio2);

                // There must be some capacitance in the circuit because the
                // uart_rx pin seems to need some extra help being driven low.
                // Otherwise, pbdrv_gpio_input(&pins->uart_rx) below can
                // sometimes read the wrong value, resulting in improper
                // detection.
                pbdrv_gpio_out_low(&pins->uart_rx);

                PT_YIELD(pt);

                // if ID2 is low
                if (pbdrv_gpio_input(&pins->uart_rx) == 0) {
                    if (data->dev_id1_group < 3) {
                        data->type_id = legodev_pup_type_id_lookup[data->dev_id1_group][DEV_ID_GROUP_PULL_DOWN];
                    }
                } else {
                    if (data->dev_id1_group < 3) {
                        data->type_id = legodev_pup_type_id_lookup[data->dev_id1_group][DEV_ID_GROUP_VCC];
                    }
                }
            } else {
                // we know the device now
                if (data->dev_id1_group < 3) {
                    data->type_id = legodev_pup_type_id_lookup[data->dev_id1_group][DEV_ID_GROUP_GND];
                } else {
                    data->type_id = PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART;
                }
            }
        }
    }

    PT_YIELD(pt);

    // set ID2 as input
    pbdrv_gpio_input(&pins->gpio2);

    // set ID1 high
    pbdrv_gpio_out_high(&pins->uart_tx);
    pbdrv_gpio_out_low(&pins->uart_buf);

    // We don't consider the detected device "affirmative" until we have
    // detected the same device multiple times in a row. Similarly,
    if (data->type_id == data->prev_type_id) {
        if (data->dev_id_match_count < UINT8_MAX) {
            data->dev_id_match_count++;
        }

        if (data->dev_id_match_count >= AFFIRMATIVE_MATCH_COUNT) {
            dev->connected_type_id = data->type_id;
        }
    } else {
        data->dev_id_match_count = 0;
    }

    data->prev_type_id = data->type_id;

    PT_END(pt);
}

#if DEBUG
static void debug_state_change(ext_dev_t *dev) {
    uint8_t i;
    for (i = 0; i < PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV; i++) {
        if (&ext_devs[i] == dev) {
            break;
        }
    }
    printf("Port %c: ", i + 'A' + PBDRV_CONFIG_LEGODEV_PUP_NUM_INT_DEV);
    if (dev->connected_type_id == PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
        printf("UART device detected.\n");
    } else if (dev->connected_type_id == PBDRV_LEGODEV_TYPE_ID_NONE) {
        printf("Device unplugged.\n");
    } else {
        printf("Passive device detected with id %d.\n", dev->connected_type_id);
    }
}
#else
#define debug_state_change(...)
#endif

PROCESS_THREAD(pbio_legodev_pup_process, ev, data) {
    static struct etimer timer;

    static ext_dev_t *dev;

    PROCESS_BEGIN();

    etimer_set(&timer, 2);

    while (!pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        PROCESS_WAIT_EVENT();
        if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            etimer_reset(&timer);

            for (int i = 0; i < PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV; i++) {
                dev = &ext_devs[i];

                // Keep running device detection unless UART sensor is attached.
                if (dev->connected_type_id != PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
                    poll_dcm(dev);
                }

                // Nothing more to do if it's the same device as before.
                if (dev->connected_type_id == dev->prev_type_id) {
                    continue;
                }

                debug_state_change(dev);

                // New device. Enable UART if needed for this device.
                if (dev->connected_type_id == PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
                    legodev_pup_enable_uart(dev->pins);

                    pbdrv_legodev_pup_uart_start_sync(dev->uart_dev);
                }
                dev->prev_type_id = dev->connected_type_id;
            }
        }
    }

    PROCESS_END();
}

struct _pbdrv_legodev_dev_t {
    bool is_internal;
    union {
        // Internal devices only have constant platform data.
        const pbdrv_legodev_pup_int_platform_data_t *int_dev;
        // External devices have both a state and constant platform data.
        ext_dev_t *ext_dev;
    };
};

static pbdrv_legodev_dev_t devs[PBDRV_CONFIG_LEGODEV_PUP_NUM_INT_DEV + PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV];

void pbdrv_legodev_init(void) {
    #if PBDRV_CONFIG_LEGODEV_PUP_NUM_INT_DEV > 0
    for (uint8_t i = 0; i < PBDRV_CONFIG_LEGODEV_PUP_NUM_INT_DEV; i++) {
        // Initialize common device type as internal.
        devs[i].is_internal = true;
        devs[i].int_dev = &pbdrv_legodev_pup_int_platform_data[i];
    }
    #endif

    for (uint8_t i = 0; i < PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV; i++) {
        // Initialize common device type as external.
        pbdrv_legodev_dev_t *legodev = &devs[i + PBDRV_CONFIG_LEGODEV_PUP_NUM_INT_DEV];
        legodev->is_internal = false;
        legodev->ext_dev = &ext_devs[i];

        // Initialize external device manager.
        const pbdrv_legodev_pup_ext_platform_data_t *legodev_data = &pbdrv_legodev_pup_ext_platform_data[i];
        const pbdrv_ioport_pup_port_platform_data_t *port_data = &pbdrv_ioport_pup_platform_data.ports[legodev_data->ioport_index];
        legodev->ext_dev->pdata = legodev_data;
        legodev->ext_dev->pins = &port_data->pins;

        // Initialize uart device manager.
        pbio_dcmotor_t *dcmotor;
        pbio_dcmotor_get_dcmotor(legodev, &dcmotor);
        legodev->ext_dev->uart_dev = pbdrv_legodev_pup_uart_configure(legodev_data->ioport_index, port_data->uart_driver_index, dcmotor);

    }
    process_start(&pbio_legodev_pup_process);
    pbdrv_legodev_pup_uart_process_start();
}

/**
 * Resets device detection. Called by legodev_uart if a device gets out of sync, usually by unplugging.
 *
 * @param [in]  dev           legodev device.
 */
void pbdrv_legodev_pup_reset_device_detection(pbdrv_legodev_pup_uart_dev_t *uartdev) {
    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(devs); i++) {
        pbdrv_legodev_dev_t *dev = &devs[i];
        if (dev->is_internal) {
            // Internal devices don't have a uart device.
            continue;
        }
        if (dev->ext_dev->uart_dev == uartdev) {
            // On match reset device connection manager state.
            dev->ext_dev->connected_type_id = PBDRV_LEGODEV_TYPE_ID_NONE;
            dev->ext_dev->dcm.dev_id_match_count = 0;
            return;
        }
    }
}

pbdrv_legodev_pup_uart_dev_t *pbdrv_legodev_get_uart_dev(pbdrv_legodev_dev_t *legodev) {
    // Device is not a uart device.
    if (legodev->is_internal || legodev->ext_dev->connected_type_id != PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
        return NULL;
    }
    return legodev->ext_dev->uart_dev;
}

pbio_error_t pbdrv_legodev_get_motor_index(pbdrv_legodev_dev_t *legodev, uint8_t *index) {
    if (legodev->is_internal) {
        *index = legodev->int_dev->motor_driver_index;
    } else {
        *index = pbdrv_ioport_pup_platform_data.ports[legodev->ext_dev->pdata->ioport_index].motor_driver_index;
    }
    return PBIO_SUCCESS;
}

static pbio_error_t pbdrv_legodev_update_angle(pbdrv_legodev_dev_t *legodev) {
    if (legodev->is_internal) {
        return PBIO_ERROR_NOT_IMPLEMENTED;
    }

    pbdrv_legodev_info_t *info;
    pbio_error_t err = pbdrv_legodev_get_info(legodev, &info);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    void *data;
    uint8_t mode = info->flags & PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS ? PBDRV_LEGODEV_MODE_PUP_ABS_MOTOR__CALIB : PBDRV_LEGODEV_MODE_PUP_REL_MOTOR__POS;
    err = pbdrv_legodev_get_data(legodev, mode, &data);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    pbio_angle_t *angle = &legodev->ext_dev->angle;

    if ((info->flags & PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS)) {
        // For absolute encoders, we need to keep track of whole rotations.
        // First, read value in tenths of degrees.
        int32_t abs_now = (int16_t)pbio_get_uint16_le(data + 2);
        int32_t abs_prev = angle->millidegrees / 100;

        // Store measured millidegree state value.
        angle->millidegrees = abs_now * 100;

        // Update rotation counter as encoder passes through 0
        if (abs_prev > 2700 && abs_now < 900) {
            angle->rotations += 1;
        }
        if (abs_prev < 900 && abs_now > 2700) {
            angle->rotations -= 1;
        }
        return PBIO_SUCCESS;
    }

    // For relative encoders, take angle as-is. At max speed (1000 deg/s),
    // degrees overflows after 24 days, so no need for buffering / resetting.
    int32_t degrees = pbio_get_uint32_le(data);
    angle->millidegrees = (degrees % 360) * 1000;
    angle->rotations = degrees / 360;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_legodev_get_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle) {
    if (legodev->is_internal) {
        pbdrv_counter_dev_t *counter;
        pbio_error_t err = pbdrv_counter_get_dev(legodev->int_dev->quadrature_index, &counter);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        return pbdrv_counter_get_angle(counter, &angle->rotations, &angle->millidegrees);
    }

    // Update internal angle state and return result.
    pbio_error_t err = pbdrv_legodev_update_angle(legodev);
    *angle = legodev->ext_dev->angle;
    return err;
}

pbio_error_t pbdrv_legodev_get_abs_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle) {
    if (legodev->is_internal) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbdrv_legodev_info_t *info;
    pbio_error_t err = pbdrv_legodev_get_info(legodev, &info);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    if (!(info->flags & PBDRV_LEGODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS)) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    err = pbdrv_legodev_update_angle(legodev);
    angle->rotations = 0;
    angle->millidegrees = legodev->ext_dev->angle.millidegrees;
    if (angle->millidegrees >= 180000) {
        angle->millidegrees -= 360000;
    }
    return err;
}

static bool type_id_matches(pbdrv_legodev_type_id_t *type, pbdrv_legodev_type_id_t actual_type) {

    // Returns what was actually detected.
    pbdrv_legodev_type_id_t desired_type = *type;
    *type = actual_type;

    // Pass for any dc motor.
    if (desired_type == PBDRV_LEGODEV_TYPE_ID_ANY_DC_MOTOR) {
        return actual_type == PBDRV_LEGODEV_TYPE_ID_LPF2_MMOTOR || actual_type == PBDRV_LEGODEV_TYPE_ID_LPF2_TRAIN;
    }

    // Pass for any encoded motor.
    if (desired_type == PBDRV_LEGODEV_TYPE_ID_ANY_ENCODED_MOTOR) {
        return actual_type == PBDRV_LEGODEV_TYPE_ID_INTERACTIVE_MOTOR ||
               actual_type == PBDRV_LEGODEV_TYPE_ID_SPIKE_M_MOTOR ||
               actual_type == PBDRV_LEGODEV_TYPE_ID_SPIKE_L_MOTOR ||
               actual_type == PBDRV_LEGODEV_TYPE_ID_SPIKE_S_MOTOR ||
               actual_type == PBDRV_LEGODEV_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR ||
               actual_type == PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR;
    }

    // Pass for any LEGO UART device.
    if (desired_type == PBDRV_LEGODEV_TYPE_ID_ANY_LUMP_UART) {
        return actual_type > PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART && actual_type <= PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR;
    }

    // Require an exact match.
    return desired_type == actual_type;
}

pbio_error_t pbdrv_legodev_get_device(pbio_port_id_t port_id, pbdrv_legodev_type_id_t *type_id, pbdrv_legodev_dev_t **legodev) {

    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(devs); i++) {
        pbdrv_legodev_dev_t *candidate = &devs[i];

        // Internal can be matched directly by port.
        if (candidate->is_internal) {
            if (candidate->int_dev->port_id != port_id) {
                continue;
            }
            *legodev = candidate;

            // Return success if device is of desired type.
            return type_id_matches(type_id, candidate->int_dev->type_id) ? PBIO_SUCCESS : PBIO_ERROR_NO_DEV;
        }

        // Otherwise handle external devices. Skip if not matching port.
        if (candidate->ext_dev->pdata->port_id != port_id) {
            continue;
        }

        // Found device instance object, now test if something is attached.
        if (candidate->ext_dev->connected_type_id == PBDRV_LEGODEV_TYPE_ID_NONE) {
            return PBIO_ERROR_NO_DEV;
        }

        // Now we can still have a passive or active device.
        *legodev = candidate;

        // Passive devices are always ready.
        if (candidate->ext_dev->connected_type_id != PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
            return type_id_matches(type_id, candidate->ext_dev->connected_type_id) ? PBIO_SUCCESS : PBIO_ERROR_NO_DEV;
        }

        // Get device information, and check if device is ready.
        pbdrv_legodev_info_t *info;
        pbio_error_t err = pbdrv_legodev_get_info(*legodev, &info);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        return type_id_matches(type_id, info->type_id) ? PBIO_SUCCESS : PBIO_ERROR_NO_DEV;
    }
    return PBIO_ERROR_NO_DEV;
}

bool pbdrv_legodev_needs_permanent_power(pbdrv_legodev_dev_t *legodev) {

    // Known internal devices don't need permanent power.
    if (legodev->is_internal) {
        return false;
    }

    // Known passive devices don't need permanent power.
    if (legodev->ext_dev->connected_type_id != PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART) {
        return false;
    }

    // Get device information, and check if device is ready.
    pbdrv_legodev_info_t *info;
    if (pbdrv_legodev_get_info(legodev, &info) != PBIO_SUCCESS) {
        return false;
    }

    return info->flags & (PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN1 | PBDRV_LEGODEV_CAPABILITY_FLAG_NEEDS_SUPPLY_PIN2);
}

#endif // PBDRV_CONFIG_LEGODEV_PUP
