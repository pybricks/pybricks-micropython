// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <pbdrv/clock.h>
#include <math.h>

#include <pbdrv/imu.h>

#include <pbio/angle.h>
#include <pbio/config.h>
#include <pbio/dcmotor.h>
#include <pbio/error.h>
#include <pbio/geometry.h>
#include <pbio/imu.h>
#include <pbio/int_math.h>
#include <pbio/util.h>

#include <pbsys/storage.h>
#include <pbsys/storage_settings.h>

#if PBIO_CONFIG_IMU

/**
 * Driver device.
 */
static pbdrv_imu_dev_t *imu_dev;

/**
 * Driver configuration. Contains settings like gyro and accelerometer scale
 * from raw units to (uncalibrated) physical units and thresholds that say
 * when the hub is stationary to be considered ready for calibration.
 */
static pbdrv_imu_config_t *imu_config;

/**
 * Uncalibrated angular velocity in the hub frame.
 *
 * These are scaled from raw units to degrees per second using only the
 * datasheet/hal conversion constant, but otherwise not further adjusted.
 */
static pbio_geometry_xyz_t angular_velocity_uncalibrated;

/**
 * Estimated gyro bias value in degrees per second.
 *
 * This is a measure for the uncalibrated angular velocity above, averaged over
 * time. If specified, the value starts at the last saved user value, then
 * updates over time.
 */
static pbio_geometry_xyz_t gyro_bias;

/**
 * Calibrated angular velocity in the hub frame degrees per second.
 *
 * This takes the uncalibrated value above, subtracts the bias estimate, and
 * rescales by a user calibration factor to ensure that integrating over one
 * full rotation adds up to 360 degrees.
 */
static pbio_geometry_xyz_t angular_velocity_calibrated;

/**
 * Uncalibrated acceleration in the hub frame in mm/s^2.
 *
 * These are scaled from raw units to mm/s^2 using only the
 * datasheet/hal conversion constant, but otherwise not further adjusted.
 */
static pbio_geometry_xyz_t acceleration_uncalibrated;

/**
 * Calibrated acceleration in the hub frame mm/s^2.
 *
 * This takes the uncalibrated value above, and subtracts a constant user offset
 * and scales by a previously determined user factor to normalize to gravity magnitude.
 */
static pbio_geometry_xyz_t acceleration_calibrated; // mm/s^2, in hub frame

/**
 * 1D integrated angular velocity for each body axis.
 *
 * This is based on integrating the calibrated angular velocity over time, so
 * including its bias and adjustments to achieve 360 degrees per rotation.
 *
 * This is not used for 3D attitude estimation, but serves as a useful way to
 * estimate 1D rotations without being effected by accelerometer fusion which
 * may leads to unwanted adjustments in applications like balancing robots.
 */
static pbio_geometry_xyz_t single_axis_rotation; // deg, in hub frame

/**
 * Rotation of the hub with respect to the inertial frame, see R(q) below.
 *
 * Initialized as the identity quaternion. Updated on first gravity sample.
 */
static pbio_geometry_quaternion_t quaternion = {
    .q1 = 0.0f,
    .q2 = 0.0f,
    .q3 = 0.0f,
    .q4 = 1.0f,
};

/**
 * Flag to indicate if the quaternion has been initialized to the very first
 * gravity sample.
 */
static bool quaternion_initialized = false;

/**
 * Rotation of the hub with respect to the inertial frame.
 *
 * Does *not* use the user application frame.
 *
 * The matrix R(q) is defined such that it transforms hub body frame vectors to
 * vectors in the inertial frame as:
 *
 *    v_inertial = R(q) * v_body
 *
 * Initialized as the identity matrix. Must match initial value of quaternion.
 */
static pbio_geometry_matrix_3x3_t pbio_imu_rotation = {
    .m11 = 1.0f, .m12 = 0.0f, .m13 = 0.0f,
    .m21 = 0.0f, .m22 = 1.0f, .m23 = 0.0f,
    .m31 = 0.0f, .m32 = 0.0f, .m33 = 1.0f,
};


/**
 * The "neutral" base orientation of the hub, describing how it is mounted
 * in the robot. All getters (tilt, acceleration, rotation, etc) give results
 * relative to this base orientation:
 *
 * vector_reported = R_base * vector_in_hub_body_frame
 *
 * Default orientation is identity, hub flat.
 */
static pbio_geometry_matrix_3x3_t pbio_imu_base_orientation = {
    .m11 = 1.0f, .m12 = 0.0f, .m13 = 0.0f,
    .m21 = 0.0f, .m22 = 1.0f, .m23 = 0.0f,
    .m31 = 0.0f, .m32 = 0.0f, .m33 = 1.0f,
};

/**
 * The heading is defined as follows.
 *
 * Take the x-axis (after transformation to application frame) and project
 * into the inertial frame. Then project onto the horizontal (X-Y) plane. Then
 * take the angle between the projection and the x-axis, counterclockwise
 * positive.
 *
 * In practice, this means that when you look at a robot from the top, it is
 * the angle that its "forward direction vector" makes with respect to the
 * x-axis, even when the robot isn't perfectly flat.
 *
 */
static float heading_projection;

/**
 * When the heading_projection flips from 180 to -180 or vice versa, we
 * increment or decrement the overal rotation counter to maintain a continuous
 * heading.
 */
static int32_t heading_rotations;


/**
 * Hub calibration settings. Cannot be used until loaded.
 */
static pbio_imu_persistent_settings_t *persistent_settings = NULL;

/**
 * Standard gravity in mm/s^2.
 */
const float standard_gravity = 9806.65f;

/**
 * Applies (newly set) settings to the driver.
 */
static void pbio_imu_apply_pbdrv_settings(pbio_imu_persistent_settings_t *settings) {

    // First occurence of this being called is when pbsys loads or resets
    // the settings, so this should never happen.
    if (!imu_config) {
        return;
    }

    imu_config->gyro_stationary_threshold = pbio_int_math_bind(settings->gyro_stationary_threshold / imu_config->gyro_scale, 1, INT16_MAX);
    imu_config->accel_stationary_threshold = pbio_int_math_bind(settings->accel_stationary_threshold / imu_config->accel_scale, 1, INT16_MAX);
}

/**
 * Sets default settings. This is called by the storage module if it has to
 * erase the settings and reinitialize them, including when a different
 * firmware version is detected.
 *
 * @param [in]  settings  The loaded settings to apply.
 */
void pbio_imu_set_default_settings(pbio_imu_persistent_settings_t *settings) {
    settings->flags = 0;
    settings->gyro_stationary_threshold = 2.0f;
    settings->accel_stationary_threshold = 2500.0f;
    settings->gravity_pos.x = settings->gravity_pos.y = settings->gravity_pos.z = standard_gravity;
    settings->gravity_neg.x = settings->gravity_neg.y = settings->gravity_neg.z = -standard_gravity;
    settings->angular_velocity_bias_start.x = settings->angular_velocity_bias_start.y = settings->angular_velocity_bias_start.z = 0.0f;
    settings->angular_velocity_scale.x = settings->angular_velocity_scale.y = settings->angular_velocity_scale.z = 360.0f;
    settings->heading_correction_1d = 360.0f;
    pbio_imu_apply_pbdrv_settings(settings);
}

/**
 * Applies settings loaded from storage to this imu module.
 *
 * @param [in]  settings  The loaded settings to apply.
 */
void pbio_imu_apply_loaded_settings(pbio_imu_persistent_settings_t *settings) {
    // This is called on load, so we can now access the settings directly.
    persistent_settings = settings;

    // The saved angular velocity bias only sets the initial value. We still
    // update the bias continuously from stationary data.
    gyro_bias.x = settings->angular_velocity_bias_start.x;
    gyro_bias.y = settings->angular_velocity_bias_start.y;
    gyro_bias.z = settings->angular_velocity_bias_start.z;

    pbio_imu_apply_pbdrv_settings(settings);
}

/**
 * Given current orientation matrix, update the heading projection.
 *
 * This is called from the update loop so we can catch the projection jumping
 * across the 180/-180 boundary, and increment or decrement the rotation to
 * have a continuous heading.
 *
 * This is also called when the orientation frame is changed because this sets
 * the application x-axis used for the heading projection.
 */
static void update_heading_projection(void) {

    // Transform application x axis back into the hub frame (R_base^T * x_unit).
    pbio_geometry_xyz_t x_application = {
        .x = pbio_imu_base_orientation.m11,
        .y = pbio_imu_base_orientation.m12,
        .z = pbio_imu_base_orientation.m13
    };

    // Transform application x axis into the inertial frame via quaternion matrix.
    pbio_geometry_xyz_t x_inertial;
    pbio_geometry_vector_map(&pbio_imu_rotation, &x_application, &x_inertial);

    // Project onto the horizontal plane and use atan2 to get the angle.
    float heading_now = pbio_geometry_radians_to_degrees(atan2f(-x_inertial.y, x_inertial.x));

    // Update full rotation counter if the projection jumps across the 180/-180 boundary.
    if (heading_now < -90 && heading_projection > 90) {
        heading_rotations++;
    } else if (heading_now > 90 && heading_projection < -90) {
        heading_rotations--;
    }
    heading_projection = heading_now;
}

// Called by driver to process one frame of unfiltered gyro and accelerometer data.
static void pbio_imu_handle_frame_data_func(int16_t *data) {

    // Initialize quaternion from first gravity sample as a best-effort estimate.
    // From here, fusion will gradually converge the quaternion to the true value.
    if (!quaternion_initialized) {
        pbio_geometry_xyz_t g = { .x = data[3], .y = data[4], .z = data[5]};
        pbio_error_t err = pbio_geometry_vector_normalize(&g, &g);
        if (err != PBIO_SUCCESS) {
            // First sample not suited, try again on next sample.
            return;
        }
        pbio_geometry_quaternion_from_gravity_unit_vector(&g, &quaternion);
        quaternion_initialized = true;
    }

    // Compute current orientation matrix to obtain the current heading.
    pbio_geometry_quaternion_to_rotation_matrix(&quaternion, &pbio_imu_rotation);

    // Projects application x-axis into the inertial frame to compute the heading.
    update_heading_projection();

    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(angular_velocity_calibrated.values); i++) {
        // Update angular velocity and acceleration cache so user can read them.
        angular_velocity_uncalibrated.values[i] = data[i] * imu_config->gyro_scale;
        acceleration_uncalibrated.values[i] = data[i + 3] * imu_config->accel_scale;

        // Once settings loaded, maintain calibrated cached values.
        if (persistent_settings) {
            float acceleration_offset = (persistent_settings->gravity_pos.values[i] + persistent_settings->gravity_neg.values[i]) / 2;
            float acceleration_scale = (persistent_settings->gravity_pos.values[i] - persistent_settings->gravity_neg.values[i]) / 2;
            acceleration_calibrated.values[i] = (acceleration_uncalibrated.values[i] - acceleration_offset) * standard_gravity / acceleration_scale;
            angular_velocity_calibrated.values[i] = (angular_velocity_uncalibrated.values[i] - gyro_bias.values[i]) * 360.0f / persistent_settings->angular_velocity_scale.values[i];
        } else {
            acceleration_calibrated.values[i] = acceleration_uncalibrated.values[i];
            angular_velocity_calibrated.values[i] = angular_velocity_uncalibrated.values[i];
        }

        // Update "heading" on all axes. This is not useful for 3D attitude
        // estimation, but it allows the user to get a 1D heading even with
        // the hub mounted at an arbitrary orientation. Such a 1D heading
        // is numerically more accurate, which is useful in drive base
        // applications so long as the vehicle drives on a flat surface.
        single_axis_rotation.values[i] += angular_velocity_calibrated.values[i] * imu_config->sample_time;
    }

    // Estimate for gravity vector based on orientation estimate.
    pbio_geometry_xyz_t s = {
        .x = pbio_imu_rotation.m31,
        .y = pbio_imu_rotation.m32,
        .z = pbio_imu_rotation.m33,
    };

    // We would like to adjust the attitude such that the gravity estimate
    // converges to the gravity value in the stationary case. If we subtract
    // both vectors we get the required direction of changes. This can be
    // thought of as a virtual spring between both vectors. This produces a
    // moment about the origin, which ultimately simplies to the following,
    // which we inject to the attitude integration.
    pbio_geometry_xyz_t correction;
    pbio_geometry_vector_cross_product(&s, &acceleration_calibrated, &correction);

    // Qualitative measures for how far the current state is from being stationary.
    float accl_stationary_error = pbio_geometry_absf(pbio_geometry_vector_norm(&acceleration_calibrated) - standard_gravity);
    float gyro_stationary_error = pbio_geometry_absf(pbio_geometry_vector_norm(&angular_velocity_calibrated));

    // Cut off value below which value is considered stationary enough for fusion.
    const float gyro_stationary_min = 10;
    const float accl_stationary_min = 150;

    // Measure for being statinonary ranging from 0 (moving) to 1 (moving less than above thresholds).
    float stationary_measure = accl_stationary_min / pbio_geometry_maxf(accl_stationary_error, accl_stationary_min) *
        gyro_stationary_min / pbio_geometry_maxf(gyro_stationary_error, gyro_stationary_min);

    // The virtual moment would produce motion in that direction, so we can
    // simulate that effect by injecting it into the attitude integration, the
    // strength of which is based on the stationary measure. It is scaled down
    // by the gravity amount since one of the two vectors to produce this has
    // units of gravity. Hence if the hub is stationary (measure = 1), and the
    // error is 90 degrees (which is unlikely), the correction is at
    // most 200 deg/s, but usually much less.
    float fusion = -stationary_measure / standard_gravity * 200;
    pbio_geometry_xyz_t adjusted_angular_velocity;
    adjusted_angular_velocity.x = angular_velocity_calibrated.x + correction.x * fusion;
    adjusted_angular_velocity.y = angular_velocity_calibrated.y + correction.y * fusion;
    adjusted_angular_velocity.z = angular_velocity_calibrated.z + correction.z * fusion;

    // Update 3D attitude, basic forward integration.
    pbio_geometry_quaternion_t dq;
    pbio_geometry_quaternion_get_rate_of_change(&quaternion, &adjusted_angular_velocity, &dq);
    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(dq.values); i++) {
        quaternion.values[i] += dq.values[i] * imu_config->sample_time;
    }
    pbio_geometry_quaternion_normalize(&quaternion);
}

// This counter is a measure for calibration accuracy, roughly equivalent
// to the accumulative number of seconds it has been stationary in total.
static uint32_t stationary_counter = 0;
static uint32_t stationary_time_last;

/*
 * Tests if the imu is ready for use in a user program.
 *
 * @return    True if it has been stationary at least once in the last 10 minutes.
*/
bool pbio_imu_is_ready(void) {
    return stationary_counter > 0 && pbdrv_clock_get_ms() - stationary_time_last < 10 * 60 * 1000;
}

// Called by driver to process unfiltered gyro and accelerometer data recorded while stationary.
static void pbio_imu_handle_stationary_data_func(const int32_t *gyro_data_sum, const int32_t *accel_data_sum, uint32_t num_samples) {

    // Don't update if not stationary
    if (!pbio_imu_is_stationary()) {
        return;
    }

    // If the IMU calibration hasn't been updated in a long time, reset the
    // stationary counter so that the calibration values get a large weight.
    if (!pbio_imu_is_ready()) {
        stationary_counter = 0;
    }

    stationary_time_last = pbdrv_clock_get_ms();
    stationary_counter++;

    // The relative weight of the new data in order to build a long term
    // average of the data without maintaining a data buffer.
    float weight = stationary_counter >= 20 ? 0.05f : 1.0f / stationary_counter;

    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(gyro_bias.values); i++) {
        // Average gyro rate while stationary, indicating current bias.
        float average_now = gyro_data_sum[i] * imu_config->gyro_scale / num_samples;

        // Update bias at decreasing rate.
        gyro_bias.values[i] = gyro_bias.values[i] * (1.0f - weight) + weight * average_now;
    }

    // If persistent gyro bias has never been set, do so now and request saving.
    // This ensures a better starting point for the next boot. We do this only
    // once to avoid unnecessary writes on every shutdown. It can be further
    // refined with a calibration routine performed by the user.
    if (persistent_settings && !(persistent_settings->flags & PBIO_IMU_SETTINGS_FLAGS_GYRO_BIAS_INITIAL_SET) && stationary_counter > 2) {
        persistent_settings->angular_velocity_bias_start = gyro_bias;
        persistent_settings->flags |= PBIO_IMU_SETTINGS_FLAGS_GYRO_BIAS_INITIAL_SET;
        pbsys_storage_request_write();
    }
}

/**
 * Initializes global imu module.
 */
void pbio_imu_init(void) {
    pbio_error_t err = pbdrv_imu_get_imu(&imu_dev, &imu_config);
    if (err != PBIO_SUCCESS) {
        return;
    }
    pbdrv_imu_set_data_handlers(imu_dev, pbio_imu_handle_frame_data_func, pbio_imu_handle_stationary_data_func);
}

/**
 * Sets the hub base orientation.
 *
 * @param [in]  front_side_axis  Which way the hub front side points when it is
 *                               in the base orientation.
 * @param [in]  top_side_axis    Which way the hub top side points when it is
 *                               in the base orientation.
 * @return                       ::PBIO_SUCCESS on success, ::PBIO_ERROR_INVALID_ARG for incorrect axis values.
 */
pbio_error_t pbio_imu_set_base_orientation(pbio_geometry_xyz_t *front_side_axis, pbio_geometry_xyz_t *top_side_axis) {

    pbio_error_t err = pbio_geometry_map_from_base_axes(front_side_axis, top_side_axis, &pbio_imu_base_orientation);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Need to update heading projection since the application axes were changed.
    update_heading_projection();

    // Reset offsets such that the new frame starts with zero heading.
    pbio_imu_set_heading(0.0f);
    return PBIO_SUCCESS;
}

/**
 * Checks if the IMU is currently stationary and motors are not moving.
 *
 * @return    True if it has been stationary for about a second, false if moving.
 */
bool pbio_imu_is_stationary(void) {
    return pbdrv_imu_is_stationary(imu_dev) && pbio_dcmotor_all_coasting();
}

/**
 * Tests if the acceleration value is within a reasonable range for a stationary hub.
 *
 * @param [in]  value  The acceleration value to test.
 * @return             True if the value is within 10% off from standard gravity.
 */
static bool pbio_imu_stationary_acceleration_out_of_range(float value, bool expect_positive) {
    const float expected_value = expect_positive ? standard_gravity : -standard_gravity;
    const float absolute_error = value > expected_value ? value - expected_value : expected_value - value;
    return absolute_error > standard_gravity / 15;
}

/**
 * Sets the IMU settings. This includes the thresholds that define when the hub
 * is stationary. When the measurements are steadily below these levels, the
 * orientation module automatically recalibrates. Also includes the hub-specific
 * correction value to get a more accurate heading value.
 *
 * Note: the flags in this setter are not used to reset the flags value in the
 * persistent settings but to select which settings are being updated here.
 *
 * @param [in]  new_settings        Incomplete set of new settings to apply according to the flags..
 * @returns ::PBIO_ERROR_INVALID_ARG if a value is out of range, otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbio_imu_set_settings(pbio_imu_persistent_settings_t *new_settings) {

    // Can't set settings if storage not loaded.
    if (!persistent_settings) {
        return PBIO_ERROR_FAILED;
    }

    if (new_settings->flags & PBIO_IMU_SETTINGS_FLAGS_ACCEL_STATIONARY_THRESHOLD_SET) {
        persistent_settings->accel_stationary_threshold = new_settings->accel_stationary_threshold;
    }

    if (new_settings->flags & PBIO_IMU_SETTINGS_FLAGS_GYRO_STATIONARY_THRESHOLD_SET) {
        persistent_settings->gyro_stationary_threshold = new_settings->gyro_stationary_threshold;
    }

    for (uint8_t i = 0; i < 3; i++) {
        if (new_settings->flags & PBIO_IMU_SETTINGS_FLAGS_GYRO_BIAS_INITIAL_SET) {
            persistent_settings->angular_velocity_bias_start.values[i] = new_settings->angular_velocity_bias_start.values[i];
        }

        if (new_settings->flags & PBIO_IMU_SETTINGS_FLAGS_GYRO_SCALE_SET) {
            if (new_settings->angular_velocity_scale.values[i] < 350 || new_settings->angular_velocity_scale.values[i] > 370) {
                return PBIO_ERROR_INVALID_ARG;
            }
            persistent_settings->angular_velocity_scale.values[i] = new_settings->angular_velocity_scale.values[i];
        }
    }

    if (new_settings->flags & PBIO_IMU_SETTINGS_FLAGS_ACCEL_CALIBRATED) {
        if (pbio_imu_stationary_acceleration_out_of_range(new_settings->gravity_pos.x, true) ||
            pbio_imu_stationary_acceleration_out_of_range(new_settings->gravity_neg.x, false) ||
            pbio_imu_stationary_acceleration_out_of_range(new_settings->gravity_pos.y, true) ||
            pbio_imu_stationary_acceleration_out_of_range(new_settings->gravity_neg.y, false) ||
            pbio_imu_stationary_acceleration_out_of_range(new_settings->gravity_pos.z, true) ||
            pbio_imu_stationary_acceleration_out_of_range(new_settings->gravity_neg.z, false)) {
            return PBIO_ERROR_INVALID_ARG;
        }
        persistent_settings->gravity_pos = new_settings->gravity_pos;
        persistent_settings->gravity_neg = new_settings->gravity_neg;
    }

    if (new_settings->flags & PBIO_IMU_SETTINGS_FLAGS_HEADING_CORRECTION_1D_SET) {
        persistent_settings->heading_correction_1d = new_settings->heading_correction_1d;
    }

    // If any settings were changed, request saving.
    if (new_settings->flags) {
        persistent_settings->flags |= new_settings->flags;
        pbsys_storage_request_write();
    }

    // The persistent settings have now been updated as applicable. Use the
    // complete set of settings and apply them to the driver.
    pbio_imu_apply_pbdrv_settings(persistent_settings);

    return PBIO_SUCCESS;
}

/**
 * Gets the IMU settings
 *
 * @param [out]  settings        Complete set of new settings.
 * @returns                      ::PBIO_ERROR_FAILED if settings not available, otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbio_imu_get_settings(pbio_imu_persistent_settings_t **settings) {
    // Can't set settings if storage not loaded.
    if (!persistent_settings) {
        return PBIO_ERROR_FAILED;
    }
    *settings = persistent_settings;
    return PBIO_SUCCESS;
}

/**
 * Gets the cached IMU angular velocity in deg/s, compensated for gyro bias.
 *
 * @param [out] values      The angular velocity vector.
 * @param [in]  calibrated  Whether to get calibrated or uncalibrated data.
 */
void pbio_imu_get_angular_velocity(pbio_geometry_xyz_t *values, bool calibrated) {
    pbio_geometry_xyz_t *angular_velocity = calibrated ? &angular_velocity_calibrated : &angular_velocity_uncalibrated;
    pbio_geometry_vector_map(&pbio_imu_base_orientation, angular_velocity, values);
}

/**
 * Gets the cached IMU acceleration in mm/s^2.
 *
 * @param [in]  calibrated  Whether to use calibrated or uncalibrated data.
 *
 * @param [out] values      The acceleration vector.
 */
void pbio_imu_get_acceleration(pbio_geometry_xyz_t *values, bool calibrated) {
    pbio_geometry_xyz_t *acceleration = calibrated ? &acceleration_calibrated : &acceleration_uncalibrated;
    pbio_geometry_vector_map(&pbio_imu_base_orientation, acceleration, values);
}

/**
 * Gets the vector that is parallel to the acceleration measurement of the stationary case.
 *
 * @param [out] values      The acceleration vector.
 */
void pbio_imu_get_tilt_vector(pbio_geometry_xyz_t *values) {
    pbio_geometry_xyz_t direction = {
        .x = pbio_imu_rotation.m31,
        .y = pbio_imu_rotation.m32,
        .z = pbio_imu_rotation.m33,
    };
    pbio_geometry_vector_map(&pbio_imu_base_orientation, &direction, values);
}

/**
 * Gets the rotation along a particular axis of the robot frame.
 *
 * The resulting value makes sense only for one-dimensional rotations.
 *
 * @param [in]  axis        The axis to project the rotation onto.
 * @param [out] angle       The angle of rotation in degrees.
 * @return                  ::PBIO_SUCCESS on success, ::PBIO_ERROR_INVALID_ARG if axis has zero length.
 */
pbio_error_t pbio_imu_get_single_axis_rotation(pbio_geometry_xyz_t *axis, float *angle) {

    // Transform the single axis rotations to the robot frame.
    pbio_geometry_xyz_t rotation;
    pbio_geometry_vector_map(&pbio_imu_base_orientation, &single_axis_rotation, &rotation);

    // Get the requested scalar rotation along the given axis.
    return pbio_geometry_vector_project(axis, &rotation, angle);
}

/**
 * Gets which side of a hub points upwards.
 *
 * @param [in]  calibrated  Whether to use calibrated or uncalibrated data.
 *
 * @return                  Which side is up.
 */
pbio_geometry_side_t pbio_imu_get_up_side(bool calibrated) {
    // Up is which side of a unit box intersects the +Z vector first.
    // So read +Z vector of the inertial frame, in the body frame.
    // For now, this is the gravity vector. In the future, we can make this
    // slightly more accurate by using the full IMU orientation.
    pbio_geometry_xyz_t *acceleration = calibrated ? &acceleration_calibrated : &acceleration_uncalibrated;
    return pbio_geometry_side_from_vector(acceleration);
}

static float heading_offset_1d = 0;
static float heading_offset_3d = 0;

/**
 * Reads the estimated IMU heading in degrees, accounting for user offset and
 * user-specified heading correction scaling constant.
 *
 * Heading is defined as clockwise positive.
 *
 * @param [in]  type        The type of heading to get.
 *
 * @return                  Heading angle in the base frame.
 */
float pbio_imu_get_heading(pbio_imu_heading_type_t type) {

    // 3D. Mapping into user frame is already accounted for in the projection.
    if (type == PBIO_IMU_HEADING_TYPE_3D) {
        return heading_rotations * 360.0f + heading_projection - heading_offset_3d;
    }

    // 1D. Map the per-axis integrated rotation to the user frame, then take
    // the negative z component as the heading for positive-clockwise convention.
    pbio_geometry_xyz_t heading_mapped;
    pbio_geometry_vector_map(&pbio_imu_base_orientation, &single_axis_rotation, &heading_mapped);

    float correction = (persistent_settings && (persistent_settings->flags & PBIO_IMU_SETTINGS_FLAGS_HEADING_CORRECTION_1D_SET)) ?
        // If set, adjust by the user-specified scaling constant.
        (360.0f / persistent_settings->heading_correction_1d):
        // No (additional) correction.
        1.0f;

    return -heading_mapped.z * correction - heading_offset_1d;
}

/**
 * Sets the IMU heading.
 *
 * This only adjusts the user offset without resetting anything in the
 * algorithm, so this can be called at any time.
 *
 * @param [in] desired_heading  The desired heading value.
 */
void pbio_imu_set_heading(float desired_heading) {
    heading_rotations = 0;
    heading_offset_3d = pbio_imu_get_heading(PBIO_IMU_HEADING_TYPE_3D) + heading_offset_3d - desired_heading;
    heading_offset_1d = pbio_imu_get_heading(PBIO_IMU_HEADING_TYPE_1D) + heading_offset_1d - desired_heading;
}

/**
 * Gets the estimated IMU heading in control units through a given scale.
 *
 * This is mainly used to convert the heading to the right format for a
 * drivebase, which measures heading as the half the difference of the two
 * motor positions in millidegrees.
 *
 * Heading is defined as clockwise positive.
 *
 * @param [in]   type                  Heading type to get.
 * @param [out]  heading               The heading angle in control units.
 * @param [out]  heading_rate          The heading rate in control units.
 * @param [in]   ctl_steps_per_degree  The number of control steps per heading degree.
 */
void pbio_imu_get_heading_scaled(pbio_imu_heading_type_t type, pbio_angle_t *heading, int32_t *heading_rate, int32_t ctl_steps_per_degree) {

    // Heading in degrees of the robot.
    float heading_degrees = pbio_imu_get_heading(type);

    // Number of whole rotations in control units (in terms of wheels, not robot).
    heading->rotations = (int32_t)(heading_degrees / (360000.0f / ctl_steps_per_degree));

    // The truncated part represents everything else. NB: The scaling factor
    // is a float here to ensure we don't lose precision while scaling.
    float truncated = heading_degrees - heading->rotations * (360000.0f / ctl_steps_per_degree);
    heading->millidegrees = (int32_t)(truncated * ctl_steps_per_degree);

    // The heading rate can be obtained by a simple scale because it always fits.
    pbio_geometry_xyz_t angular_rate;
    pbio_imu_get_angular_velocity(&angular_rate, true);
    *heading_rate = (int32_t)(-angular_rate.z * ctl_steps_per_degree);
}

/**
 * Reads the current rotation matrix.
 *
 * @param [out] rotation      The rotation matrix
 */
void pbio_orientation_imu_get_rotation(pbio_geometry_matrix_3x3_t *rotation) {
    *rotation = pbio_imu_rotation;
}

#endif // PBIO_CONFIG_IMU
