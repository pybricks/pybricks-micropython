/**
*  \file digi_sensors.c
*    
*  \brief Function definitions to talk to the digital sensors (I2C sensors)
* 
*  \author Tobias Schießl, Christian Soward
*/

/* Include statements */
#include "digi_sensors.h"
#include "i2c.h"
#include "stdio.h"
#include "string.h"

/* Macro definitions */
/**
* \brief The default slave address used by all I2C sensors so far
**/ 
#define I2C_DEFAULT_SLAVE_ADDRESS       0x01
/**
* \brief The default internal register address which contains the values meassured by the I2C sensors (or the start address in case multiple registers are required)
**/ 
#define I2C_DEFAULT_INTERNAL_ADDRESS    0x42

/* Global variable definitions */
/**
* \brief Array storing information about the current mode of Lego ultrasonic sensors (0x02 = CONTINOUS MODE is the default mode)
**/ 
unsigned char ultrasonic_modes[4] = {2, 2, 2, 2};
/**
* \brief Array storing calibration information for the HiTechnic acceleration sensor
*
* This information is used to calculate return values for the acceleration sensor which are relativ to 0.
**/ 
short accel_calibration[4][4] = {0};

/**
* \brief Enable a digital sensor by configuring the GPIO pins required for I2C communication
*
* \param port - The port the sensor is connected to
*
* \return none
**/
void sensor_digi_enable(int port) {
    i2c_enable(port);
}

/**
* \brief Get the range of the Lego ultrasonic sensor connected at the specified port
*
* The sensor meassures distances from 0 to 255 in cm. If nothing is located in front of the sensor, the value will be 255. This function will consider the sensor's current mode. 
* If it is in CONTINOUS MODE (default), only the first value of the array will be valid. The others will be set to 0.
* If it is in SINGLE SHOT MODE, all 8 entries of the array will be values returned by the sensor. If less than 8 objects are detected, some entries will be set to 255.
* If it is in OFF MODE, all 8 entries of the array will be set to 0.
* Note: If the sensor is in SINGLE SHOT MODE, no additional SINGLE SHOT command will be sent in this function - call sensor_ultrasonic_set_mode therefore.
*
* \param port - The port the sensor is connected to
* \param data_buffer - Buffer to store the result in (depending on the sensor's current mode, not all entries might be filled with values returned by the sensor)
*
* \return none
**/
void sensor_ultrasonic_get_range(int port, unsigned char data_buffer[8]) {
    unsigned char length;
    switch (ultrasonic_modes[port]) {
        case 0:
            for (int i = 0; i < 8; ++i)
                data_buffer[i] = 0;
            return;
        case 1:
            length = 8;
            break;
        case 2:
            for (int i = 1; i < 8; ++i)
                data_buffer[i] = 0;
            length = 1;
            break;
    }
    int ret = i2c_start_transaction(port, I2C_DEFAULT_SLAVE_ADDRESS, I2C_DEFAULT_INTERNAL_ADDRESS, 1, data_buffer, length, 0);
    if (ret != 0) {
        if (ret == -1)
            printf("No device ready at port %i\n", port);
        else
            printf("No ACK for byte %i\n\r", ret);
    }
}

/**
* \brief Set the mode of the Lego ultrasonic sensor at the specified port
*
* \param port - The port the sensor is connected to
* \param mode - The mode to set: ULTRASONIC_MODE_OFF (0x00), ULTRASONIC_MODE_SINGLE_SHOT (0x01) or ULTRASONIC_MODE_CONTINOUS (0x02)
*
* \return none
**/
void sensor_ultrasonic_set_mode(int port, unsigned char mode) {
    int ret = i2c_start_transaction(port, I2C_DEFAULT_SLAVE_ADDRESS, 0x41, 1, &mode, 1, 1);
    if (ret != 0) {
        if (ret == -1)
            printf("No device ready at port %i\n", port);
        else
            printf("No ACK for byte %i\n\r", ret);
        return;
    }
    ultrasonic_modes[port] = mode;
}

/**
* \brief Read the current direction meassured by the HiTechnic compass sensor at the specified port
*
* \param port - The port the sensor is connected to
*
* \return The current direction in degrees meassured by the sensor, ranging from 0 to 360
**/
unsigned short sensor_compass_get(int port) {
    U8 data[2];
    int ret = i2c_start_transaction(port, I2C_DEFAULT_SLAVE_ADDRESS, I2C_DEFAULT_INTERNAL_ADDRESS, 1, data, 2, 0);
    if (ret != 0) {
        if (ret == -1)
            printf("No device ready at port %i\n", port);
        else
            printf("No ACK for byte %i\n\r", ret);
        return 0;
    }
    unsigned short degree = 2 * data[0] + data[1];
    return degree;
}

/**
* \brief Start the HiTechnic compass sensor calibration
*
* You should calibrate a compass sensor when it is mounted on your robot in the way you want to use it. So the sensor will be calibrated for your specific environment/robot. The calibration adjustment is stored persitent on the sensor itself. For more information see the HiTechnic documentation.
* Calibrating the compass sensor takes 3 steps. (1) Call this function. (2) Move the sensor/robot in a circle (540 degrees - 720 degrees within 20 seconds). (3) Call the appropriate end_calibration function.
*
* \param port - The port the sensor is connected to
*
* \return none
**/
void sensor_compass_start_calibration(int port){
	U8 data;
	data = 0x43;
	i2c_start_transaction(port, 1, 0x41, 1, &data, 1, 1);
}

/**
* \brief Stop the HiTechnic compass sensor calibration
*
* Read the information in the appropriate start_calibration function first.
* This function is the third step and finishs the calibration process.
*
* \param port - The port the sensor is connected to
*
* \return 1 if the calibration was successful, 0 otherwise
**/
unsigned short sensor_compass_end_calibration(int port){
	U8 data;
	data = 0x00;
	i2c_start_transaction(port, 1, 0x41, 1, &data, 1, 1);
	i2c_start_transaction(port, 1, 0x41, 1, &data, 1, 0);

	if(data == 0x00) {
		return 1; //cal success
	} else if (data == 0x46) {
		return 0; //cal failed. HiTechnic doc says fail == 0x02, but that never happend. cal fail seems to be 0x46...
	} else {
		return 0; //cal failed. Unknown issue.
	}
	return 0;
}

/**
* \brief Calibrate the HiTechnic acceleration sensor at the specified port
*
* The sensor has be at rest when this function is called. Calibrating the sensor means to get the values returned while at rest in order to return values realtive to 0
* in calls to sensor_accel_get. If this function is not called directly, it will be triggered on the first call to sensor_accel_get.
*
* \param port - The port the sensor is connected to
*
* \return none
**/
void sensor_accel_calibrate(int port) {
    S8 data[6];
    int ret = i2c_start_transaction(port, I2C_DEFAULT_SLAVE_ADDRESS, I2C_DEFAULT_INTERNAL_ADDRESS, 1, (U8*) data, 6, 0);
    
    if (ret != 0) {
        return;
    }
 
    for (int i = 0; i < 3; ++i) {
        accel_calibration[port][i] = ((((short)data[i]) << 2) | (data [i + 3] & 0x0003));
    }
    
    accel_calibration[port][3] = 1;
}

/**
* \brief Get the 0-relativ acceleration meassured by the HiTechnic acceleration sensor at the specified port
*
* The sensor meassures the acceleration on all 3 axis (X, Y and Z).
* This function will call sensor_accel_calibrate when called for the first time. The values returned will be relativ to 0.
*
* \param port - The port the sensor is connected to
* \param data_buffer - Buffer to store the meassured values in (the values will be stored in the buffer in the following order: X, Y, Z)
*
* \return none
**/
void sensor_accel_get(int port, short data_buffer[3]) {
    if (accel_calibration[port][3] == 0)
        sensor_accel_calibrate(port);

    S8 data[6];
    int ret = i2c_start_transaction(port, I2C_DEFAULT_SLAVE_ADDRESS, I2C_DEFAULT_INTERNAL_ADDRESS, 1, (U8*) data, 6, 0);
    if (ret != 0) {
        if (ret == -1)
            printf("No device ready at port %i\n", port);
        else
            printf("No ACK for byte %i\n\r", ret);
        for (int i = 0; i < 3; ++i)
            data_buffer[i] = 0;
        return;
    }
    for (int i = 0; i < 3; ++i) {
        data_buffer[i] = ((((short)data[i]) << 2) | (data [i + 3] & 0x0003)) - accel_calibration[port][i];
    }
}

/**
* \brief Get the raw acceleration meassured by the HiTechnic acceleration sensor at the specified port
*
* The sensor meassures the acceleration on all 3 axis (X, Y and Z).
* This function will not call sensor_accel_calibrate and return the raw values (not relativ to 0) returned by the sensor.
*
* \param port - The port the sensor is connected to
* \param data_buffer - Buffer to store the meassured values in (the values will be stored in the buffer in the following order: X, Y, Z)
*
* \return none
**/
void sensor_accel_get_raw(int port, short data_buffer[3]) {
    S8 data[6];
    int ret = i2c_start_transaction(port, I2C_DEFAULT_SLAVE_ADDRESS, I2C_DEFAULT_INTERNAL_ADDRESS, 1, (U8*) data, 6, 0);
    if (ret != 0) {
        if (ret == -1)
            printf("No device ready at port %i\n", port);
        else
            printf("No ACK for byte %i\n\r", ret);
        for (int i = 0; i < 3; ++i)
            data_buffer[i] = 0;
    }
}

/**
* \brief Calibrate the HiTechnic color sensor at the specified port
*
* To calibrate the sensor properly, this function has to be called two times. Once with mode set to CAL_WHITE (0x043) and once with mode set to CAL_BLACK (0x42).
* Calibration information is stored directly on the sensor and will be persistent, even if the sensor is no longer provided with power.
* When called with mode set to CAL_WHITE, the sensor should be located in front of a diffuse white surface at a distance of 1.5 cm.
* When called with mode set to CAL_BLACK, the sensor should have nothing in front of it within a distance of about 2 m.
* If the calibration command was received successfully, the sensors LED will blink for confirmation.
*
* \param port - The port the sensor is connected to
* \param mode - The mode to calibrate the sensor with: CAL_WHITE (0x43) or CAL_BLACK (0x42)
*
* \return 1 if the calibration was successful, 0 otherwise
**/
unsigned char sensor_hitechnic_color_calibrate(int port, int mode) {
    int ret = i2c_start_transaction(port, I2C_DEFAULT_SLAVE_ADDRESS, 0x41, 1, (U8*) &mode, 1, 1);
    if (ret != 0) {
        if (ret == -1)
            printf("No device ready at port %i\n", port);
        else
            printf("No ACK for byte %i\n\r", ret);
        return 0;
    }
    return 1;
}

/**
* \brief Get the red, green and blue values (RGB) meassured by the HiTechnic color sensor at the specified port
*
* For best results, the sensor should be calibrated before calling this function.
*
* \param port - The port the sensor is connected to
* \param data_buffer - Buffer to store the received data in (the values will be stored in the following order: red, green, blue)
*
* \return none
**/
void sensor_hitechnic_color_get(int port, signed short data_buffer[3]) {
    U8 data[3];
    int ret = i2c_start_transaction(port, I2C_DEFAULT_SLAVE_ADDRESS, 0x43, 1, data, 3, 0);
    if (ret != 0) {
        if (ret == -1)
            printf("No device ready at port %i\n", port);
        else
            printf("No ACK for byte %i\n\r", ret);
        return;
    }
    for (int i = 0; i < 3; ++i)
        data_buffer[i] = (signed short) data[i];
}

/**
* \brief Get the ID of the color meassured by the HiTechnic color sensor at the specified port
*
* For best results, the sensor should be calibrated before calling this function.
* See digi_sensors.h to see which color is represented by which ID.
*
* \param port - The port the sensor is connected to
*
* \return The ID of the color meassured by the sensor
**/
unsigned char sensor_hitechnic_color_get_color_id(int port) {
    U8 data;
    int ret = i2c_start_transaction(port, I2C_DEFAULT_SLAVE_ADDRESS, I2C_DEFAULT_INTERNAL_ADDRESS, 1, &data, 1, 0);
    if (ret != 0) {
        if (ret == -1)
            printf("No device ready at port %i\n", port);
        else
            printf("No ACK for byte %i\n\r", ret);
        return 0;
    }
    return data;
}

/**
* \brief Transform a HiTechnic color sensor ID into a String
*
* This function was for debug purposes but it might be useful in the future. Therefore it remains part of the code.
*
* \param color_id - The ID of the color to represent as a String
* \param string_buffer - Buffer to store the String representation of the color in
*
* \return none
**/
void sensor_hitechnic_color_id_to_string(unsigned char color_id, char *string_buffer) {
    switch (color_id) {
        case HI_TECHNIC_COL_BLACK:
            memcpy(string_buffer, "BLACK", 6);
            return;
        case HI_TECHNIC_COL_PURPLE:
            memcpy(string_buffer, "PURPLE", 7);
            return;
        case HI_TECHNIC_COL_DARK_BLUE:
            memcpy(string_buffer, "DARK_BLUE", 10);
            return;
        case HI_TECHNIC_COL_LIGHT_BLUE:
            memcpy(string_buffer, "LIGHT_BLUE", 11);
            return;
        case HI_TECHNIC_COL_GREEN:
            memcpy(string_buffer, "GREEN", 6);
            return;
        case HI_TECHNIC_COL_YELLOW_GREEN:
            memcpy(string_buffer, "YELLOW_GREEN", 13);
            return;
        case HI_TECHNIC_COL_YELLOW:
            memcpy(string_buffer, "YELLOW", 7);
            return;
        case HI_TECHNIC_COL_ORANGE:
            memcpy(string_buffer, "ORGANGE", 8);
            return;
        case HI_TECHNIC_COL_RED:
            memcpy(string_buffer, "RED", 4);
            return;
        case HI_TECHNIC_COL_RED_PINK:
            memcpy(string_buffer, "RED_PINK", 9);
            return;
        case HI_TECHNIC_COL_PINK:
            memcpy(string_buffer, "PINK", 5);
            return;
        case HI_TECHNIC_COL_GREY1:
        case HI_TECHNIC_COL_GREY2:
        case HI_TECHNIC_COL_GREY3:
        case HI_TECHNIC_COL_GREY4:
        case HI_TECHNIC_COL_GREY5:
        case HI_TECHNIC_COL_GREY6:
            memcpy(string_buffer, "GREY", 6);
            return;
        case HI_TECHNIC_COL_WHITE:
            memcpy(string_buffer, "WHITE", 6);
            return;
        default:
            string_buffer[0] = '\0';
            return;
    }
}

/**
* \brief Set the resolution for the measured temperature values.
*
* The internal Texas Instruments tmp275 temperature sensor can operate in different resolution modes. Lower resolution is faster. See the TEMP9BIT ... TEMP12BIT macros for more information or the official documentation.
*
* \param port - The port the sensor is connected to
* \param resolution - The resolution. Use on of the macros TEMP9BIT ... TEMP12BIT
*
* \return none
**/
void sensor_temperature_nxt_set_resolution(int port, unsigned char resolution) {
	resolution = resolution << 5;
	i2c_start_transaction(port, 0x4C, 0x01, 1, &resolution, 1, 1);
}

/**
* \brief Returns the current resolution setting of the sensor.
*
* \param port - The port the sensor is connected to
*
* \return The current resolution. See the macros TEMP9BIT ... TEMP12BIT
**/
unsigned char sensor_temperature_nxt_get_resolution(int port) {
	unsigned char data;
	i2c_start_transaction(port, 0x4C, 0x01, 1, &data, 1, 0);
	data = ( data >> 5 ) & 0x03;
	return data;
}

/**
* \brief Measures and returns the current temperature value.
*
* \param port - The port the sensor is connected to
*
* \return The current temperature given in degrees Celsius
**/
float sensor_temperature_nxt_get_temperature(int port) {
	volatile unsigned char data[2];
	i2c_start_transaction(port, 0x4C, 0x00, 1, (U8*) data, 2, 0);

	signed short temp = 0x00;
	temp = (((data[0] << 4) | (data[1] >> 4) ) & 0xFFF);

	if(temp & 0x800) {// 1xxx xxxx xxxx -> negative value
		return ((((~temp) & 0xFFF) * -0.0625f) -0.0625f);
	} else {// 0xxx xxxx xxxx -> positive value
		return (temp * 0.0625f);
	}
}
