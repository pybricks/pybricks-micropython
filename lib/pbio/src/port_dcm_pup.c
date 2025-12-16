// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include <pbio/config.h>

#include <pbio/port_interface.h>
#include <pbio/port_dcm.h>
#include <pbdrv/ioport.h>

#if PBIO_CONFIG_PORT_DCM_PUP

/** The number of consecutive repeated detections needed for an affirmative ID. */
#define AFFIRMATIVE_MATCH_COUNT 20

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#include <inttypes.h>
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define debug_pr pbdrv_uart_debug_printf
#define DBG_ERR(expr) expr
#else
#define debug_pr(...)
#define DBG_ERR(expr)
#endif

typedef enum {
    DEV_ID_GROUP_GND,
    DEV_ID_GROUP_VCC,
    DEV_ID_GROUP_PULL_DOWN,
    DEV_ID_GROUP_OPEN,
} dev_id_group_t;

// Device connection manager state for each port
struct _pbio_port_dcm_t {
    /** Most recent one-off device ID candidate. */
    lego_device_type_id_t type_id;
    /** Previous one-off device ID candidate. */
    lego_device_type_id_t prev_type_id;
    /** Number of consecutive detections of the same device ID. */
    uint8_t dev_id_match_count;
    // Intermediate state values to preserve in between async calls.
    dev_id_group_t dev_id1_group;
    uint8_t gpio_value;
    uint8_t prev_gpio_value;
};

pbio_port_dcm_t dcm_state[PBIO_CONFIG_PORT_DCM_NUM_DEV];

static const lego_device_type_id_t legodev_pup_type_id_lookup[3][3] = {
    /* ID1 */ [DEV_ID_GROUP_GND] = {
        /* ID2 */ [DEV_ID_GROUP_GND] = LEGO_DEVICE_TYPE_ID_LPF2_POWER,
        /* ID2 */ [DEV_ID_GROUP_VCC] = LEGO_DEVICE_TYPE_ID_LPF2_TURN,
        /* ID2 */ [DEV_ID_GROUP_PULL_DOWN] = LEGO_DEVICE_TYPE_ID_LPF2_LIGHT2,
    },
    /* ID1 */ [DEV_ID_GROUP_VCC] = {
        /* ID2 */ [DEV_ID_GROUP_GND] = LEGO_DEVICE_TYPE_ID_LPF2_TRAIN,
        /* ID2 */ [DEV_ID_GROUP_VCC] = LEGO_DEVICE_TYPE_ID_LPF2_LMOTOR,
        /* ID2 */ [DEV_ID_GROUP_PULL_DOWN] = LEGO_DEVICE_TYPE_ID_LPF2_LIGHT1,
    },
    /* ID1 */ [DEV_ID_GROUP_PULL_DOWN] = {
        /* ID2 */ [DEV_ID_GROUP_GND] = LEGO_DEVICE_TYPE_ID_LPF2_MMOTOR,
        /* ID2 */ [DEV_ID_GROUP_VCC] = LEGO_DEVICE_TYPE_ID_LPF2_XMOTOR,
        /* ID2 */ [DEV_ID_GROUP_PULL_DOWN] = LEGO_DEVICE_TYPE_ID_LPF2_LIGHT,
    },
};

// This process needs 2ms between each yield point, giving the gpio tests
// enough time to settle.
#define DCM_AWAIT_MS (2)

/**
 * Thread that detects the device type. It monitors the ID1 and ID2 pins
 * on the port to see when devices are connected or disconnected.
 *
 * It ends once a LEGO UART device is connected or nothing is connected.
 *
 * @param [in]  state       The process thread state.
 * @param [in]  timer       The timer to use for timing.
 * @param [in]  dcm         The device connection manager.
 * @param [in]  pins        The ioport pins.
 */
pbio_error_t pbio_port_dcm_thread(pbio_os_state_t *state, pbio_os_timer_t *timer, pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins) {

    PBIO_OS_ASYNC_BEGIN(state);

    dcm->prev_type_id = LEGO_DEVICE_TYPE_ID_NONE;
    dcm->dev_id_match_count = 0;

    // Repeat until we definitively have a UART device or no device is
    // detected. So it stays here as long as a passive motor device is
    // attached.
    while (dcm->dev_id_match_count < AFFIRMATIVE_MATCH_COUNT || (dcm->prev_type_id != LEGO_DEVICE_TYPE_ID_LPF2_UNKNOWN_UART && dcm->prev_type_id != LEGO_DEVICE_TYPE_ID_NONE)) {

        dcm->type_id = LEGO_DEVICE_TYPE_ID_NONE;
        dcm->dev_id1_group = DEV_ID_GROUP_OPEN;

        // set ID1 high
        pbdrv_gpio_out_high(&pins->uart_tx);
        pbdrv_gpio_out_low(&pins->uart_buf);

        // set ID2 as input
        pbdrv_gpio_input(&pins->p6);

        PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

        // save current ID2 value
        dcm->prev_gpio_value = pbdrv_gpio_input(&pins->p6);

        // set ID1 low
        pbdrv_gpio_out_low(&pins->uart_tx);

        PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

        // read ID2
        dcm->gpio_value = pbdrv_gpio_input(&pins->p6);

        // if ID2 changed from high to low
        if (dcm->prev_gpio_value == 1 && dcm->gpio_value == 0) {
            // we have touch sensor
            dcm->type_id = LEGO_DEVICE_TYPE_ID_LPF2_TOUCH;

            // set ID1 as input
            pbdrv_gpio_out_high(&pins->uart_buf);
            pbdrv_gpio_input(&pins->uart_tx);

            PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

            // ID1 is inverse of touch sensor value
            // TODO: save this value to sensor dcm
            // sensor_data = !pbdrv_gpio_input(&pins->p5);
        }
        // if ID2 changed from low to high
        else if (dcm->prev_gpio_value == 0 && dcm->gpio_value == 1) {
            dcm->type_id = LEGO_DEVICE_TYPE_ID_LPF2_TPOINT;
        } else {
            // read ID1
            dcm->prev_gpio_value = pbdrv_gpio_input(&pins->p5);

            // set ID1 high
            pbdrv_gpio_out_high(&pins->uart_tx);

            PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

            // read ID1
            dcm->gpio_value = pbdrv_gpio_input(&pins->p5);

            // if ID1 did not change and is high
            if (dcm->prev_gpio_value == 1 && dcm->gpio_value == 1) {
                // we have ID1 == VCC
                dcm->dev_id1_group = DEV_ID_GROUP_VCC;
            }
            // if ID1 did not change and is low
            else if (dcm->prev_gpio_value == 0 && dcm->gpio_value == 0) {
                // we have ID1 == GND
                dcm->dev_id1_group = DEV_ID_GROUP_GND;
            } else {
                // set ID1 as input
                pbdrv_gpio_out_high(&pins->uart_buf);
                pbdrv_gpio_input(&pins->uart_tx);

                PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

                // read ID1
                if (pbdrv_gpio_input(&pins->p5) == 1) {
                    // we have ID1 == open
                    dcm->dev_id1_group = DEV_ID_GROUP_OPEN;
                } else {
                    // we have ID1 == pull down
                    dcm->dev_id1_group = DEV_ID_GROUP_PULL_DOWN;
                }
            }

            PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

            // set ID1 as input
            pbdrv_gpio_out_high(&pins->uart_buf);
            pbdrv_gpio_input(&pins->uart_tx);

            // set ID2 high
            pbdrv_gpio_out_high(&pins->p6);

            PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

            // read ID1
            dcm->prev_gpio_value = pbdrv_gpio_input(&pins->p5);

            // set ID2 low
            pbdrv_gpio_out_low(&pins->p6);

            PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

            // read ID1
            dcm->gpio_value = pbdrv_gpio_input(&pins->p5);

            // if ID1 changed from high to low
            if (dcm->prev_gpio_value == 1 && dcm->gpio_value == 0) {
                // if we have ID1 = open
                if (dcm->dev_id1_group == DEV_ID_GROUP_OPEN) {
                    // then we have this
                    dcm->type_id = LEGO_DEVICE_TYPE_ID_LPF2_3_PART;
                }
            }
            // if ID1 changed from low to high
            else if (dcm->prev_gpio_value == 0 && dcm->gpio_value == 1) {
                // something might explode
                dcm->type_id = LEGO_DEVICE_TYPE_ID_LPF2_EXPLOD;
            } else {
                // set ID1 high
                pbdrv_gpio_out_high(&pins->uart_tx);
                pbdrv_gpio_out_low(&pins->uart_buf);

                // set ID2 high
                pbdrv_gpio_out_high(&pins->p6);

                PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

                // if ID2 is high
                if (pbdrv_gpio_input(&pins->uart_rx) == 1) {
                    // set ID2 low
                    pbdrv_gpio_out_low(&pins->p6);

                    // There must be some capacitance in the circuit because the
                    // uart_rx pin seems to need some extra help being driven low.
                    // Otherwise, pbdrv_gpio_input(&pins->uart_rx) below can
                    // sometimes read the wrong value, resulting in improper
                    // detection.
                    pbdrv_gpio_out_low(&pins->uart_rx);

                    PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

                    // if ID2 is low
                    if (pbdrv_gpio_input(&pins->uart_rx) == 0) {
                        if (dcm->dev_id1_group < 3) {
                            dcm->type_id = legodev_pup_type_id_lookup[dcm->dev_id1_group][DEV_ID_GROUP_PULL_DOWN];
                        }
                    } else {
                        if (dcm->dev_id1_group < 3) {
                            dcm->type_id = legodev_pup_type_id_lookup[dcm->dev_id1_group][DEV_ID_GROUP_VCC];
                        }
                    }
                } else {
                    // we know the device now
                    if (dcm->dev_id1_group < 3) {
                        dcm->type_id = legodev_pup_type_id_lookup[dcm->dev_id1_group][DEV_ID_GROUP_GND];
                    } else {
                        dcm->type_id = LEGO_DEVICE_TYPE_ID_LPF2_UNKNOWN_UART;
                    }
                }
            }
        }

        PBIO_OS_AWAIT_MS(state, timer, DCM_AWAIT_MS);

        // set ID2 as input
        pbdrv_gpio_input(&pins->p6);

        // set ID1 high
        pbdrv_gpio_out_high(&pins->uart_tx);
        pbdrv_gpio_out_low(&pins->uart_buf);

        // We don't consider the detected device "affirmative" until we have
        // detected the same device multiple times in a row.
        if (dcm->type_id == dcm->prev_type_id) {
            if (dcm->dev_id_match_count < AFFIRMATIVE_MATCH_COUNT) {
                dcm->dev_id_match_count++;
            }
        } else {
            // This iteration found a different result, so reset counter.
            dcm->dev_id_match_count = 0;
            dcm->prev_type_id = dcm->type_id;
        }

        #if DEBUG
        // If definitive result found, set the device info.
        if (dcm->dev_id_match_count == AFFIRMATIVE_MATCH_COUNT) {
            // Log changes in detected ID. Guess port ID for debugging only.
            for (uint8_t c = 0; c < PBIO_CONFIG_PORT_DCM_NUM_DEV; c++) {
                if (&dcm_state[c] == dcm && device_info->type_id != dcm->type_id) {
                    debug_pr("Port %c: Detected ID: %d\n", c + 'A', dcm->type_id);
                }
            }
        }
        #endif
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Gets device connection manager state.
 *
 * @param [in]  index       The index of the DC motor.
 * @return                  The dcmotor instance.
 */
pbio_port_dcm_t *pbio_port_dcm_init_instance(uint8_t index) {
    if (index >= PBIO_CONFIG_PORT_DCM_NUM_DEV) {
        return NULL;
    }
    pbio_port_dcm_t *dcm = &dcm_state[index];
    return dcm;
}

pbio_error_t pbio_port_dcm_set_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t type_id) {
    // This platform can detect everything in LEGO mode, so no need to manually
    // set it. Instead, when custom devices are used we switch the port mode.
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbio_port_dcm_assert_type_id(pbio_port_dcm_t *dcm, lego_device_type_id_t *type_id) {

    // Require definitive detection before returning the device type.
    if (!dcm || dcm->dev_id_match_count < AFFIRMATIVE_MATCH_COUNT) {
        return PBIO_ERROR_NO_DEV;
    }

    // Need use prev_type_id as it has always gone through a full cycle. The
    // type_id may be a partial match and constantly changes.
    switch (dcm->prev_type_id) {
        case LEGO_DEVICE_TYPE_ID_LPF2_MMOTOR:
        case LEGO_DEVICE_TYPE_ID_LPF2_TRAIN:
        case LEGO_DEVICE_TYPE_ID_LPF2_LIGHT:
            // On Powered Up, the only known existing passive devices are DC
            // devices. Pass if requesting a specific match or any DC device.
            if (*type_id == LEGO_DEVICE_TYPE_ID_ANY_DC_MOTOR || *type_id == dcm->prev_type_id) {
                *type_id = dcm->prev_type_id;
                return PBIO_SUCCESS;
            }
            return PBIO_ERROR_NO_DEV;
        case LEGO_DEVICE_TYPE_ID_LPF2_UNKNOWN_UART:
            if (*type_id == LEGO_DEVICE_TYPE_ID_ANY_LUMP_UART) {
                return PBIO_SUCCESS;
            }
            return PBIO_ERROR_NO_DEV;
        default:
            return PBIO_ERROR_NO_DEV;
    }
}

uint32_t pbio_port_dcm_get_analog_value(pbio_port_dcm_t *dcm, const pbdrv_ioport_pins_t *pins, bool active) {
    return 0;
}

pbio_port_dcm_analog_rgba_t *pbio_port_dcm_get_analog_rgba(pbio_port_dcm_t *dcm) {
    return NULL;
}

#endif // PBIO_CONFIG_PORT_DCM_PUP
