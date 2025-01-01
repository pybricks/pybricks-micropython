#include <stdint.h>
#include <lego/device.h>

/**
 * Gets the minimum time needed before stale data is discarded.
 *
 * This is empirically determined based on sensor experiments.
 *
 * @param [in]  id          The device type ID.
 * @param [in]  mode        The device mode.
 * @return                  Required delay in milliseconds.
 */
uint32_t lego_device_stale_data_delay(lego_device_type_id_t id, uint8_t mode) {
    switch (id) {
        case LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR:
            return mode == LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX ? 0 : 30;
        case LEGO_DEVICE_TYPE_ID_SPIKE_COLOR_SENSOR:
            return mode == LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__LIGHT ? 0 : 30;
        case LEGO_DEVICE_TYPE_ID_SPIKE_ULTRASONIC_SENSOR:
            return mode == LEGO_DEVICE_MODE_PUP_ULTRASONIC_SENSOR__LIGHT ? 0 : 50;
        case LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR:
            return 30;
        case LEGO_DEVICE_TYPE_ID_EV3_IR_SENSOR:
            return 1100;
        case LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR:
            return 20;
        case LEGO_DEVICE_TYPE_ID_NXT_SOUND_SENSOR:
            return 300;
        case LEGO_DEVICE_TYPE_ID_NXT_ENERGY_METER:
            return 200;
        default:
            // Default delay for other sensors and modes.
            return 0;
    }
}

/**
 * Gets the minimum time needed for the device to handle written data.
 *
 * This is empirically determined based on sensor experiments.
 *
 * @param [in]  id          The device type ID.
 * @param [in]  mode        The device mode.
 * @return                  Required delay in milliseconds.
 */
uint32_t lego_device_data_set_delay(lego_device_type_id_t id, uint8_t mode) {
    // The Boost Color Distance Sensor requires a long delay or successive
    // writes are ignored.
    if (id == LEGO_DEVICE_TYPE_ID_COLOR_DIST_SENSOR && mode == LEGO_DEVICE_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX) {
        return 250;
    }

    // Default delay for setting data. In practice, this is the delay for setting
    // the light on the color sensor and ultrasonic sensor.
    return 10;
}
