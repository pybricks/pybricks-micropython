/**
*  \file button.c
*    
*  \brief This file contains function definitions to use the hardware buttons of the EV3
* 
*  \author Tobias Schießl, ev3ninja
*/

/* Include statements */
#include "button.h"
#include "gpio.h"

/* Global variable definitions */
/**
* \brief Array storing the required GPIO pins to read the state of the buttons
**/ 
button_info buttons[] = {
    { GPIO_PIN(6,  6) },
    { GPIO_PIN(7, 12) },
    { GPIO_PIN(7, 15) },
    { GPIO_PIN(7, 14) },
    { GPIO_PIN(1, 13) },
    { GPIO_PIN(6, 10) }
};

/**
* \brief Check if the given button is pressed
* 
* \param button - The ID of the button to check (ranging from 0x00 to 0x05)
*
* \return The state of the button (BUTTON_DOWN = 0x01 if the button is pressed, BUTTON_UP = 0x00 otherwise)
**/
button_state button_get_state (button_id button) {
    if (button >= BUTTON_LEFT && button <= BUTTON_BACK)
        return gpio_get(buttons[button].pin);
    else
        return BUTTON_UP;
}

/**
* \brief Check if any button is pressed
* 
* This function will check if any button is pressed. As soon as one button that is pressed is found, it will be returned and the remaining buttons will not be checked.
* The buttons are checked in the order they are defined in the enumeration button_id, i.e. LEFT, RIGHT, TOP, BOTTOM, CENTER, BACK.
*
* \return The ID of the button that was pressed (ranging from 0x00 to 0x05) or BUTTON_NONE (0xFF) if none was pressed
**/
button_id button_get_pressed(void) {
    for (int i = BUTTON_LEFT; i <= BUTTON_BACK; ++i)
        if (button_get_state(i) == BUTTON_DOWN)
            return i;
    return BUTTON_NONE;
}

/**
* \brief Initialize the buttons be configuring the required GPIO pins as input pins.
*
* \return none
**/
void button_init (void) {
    gpio_init_inpin(buttons[0].pin);
    gpio_init_inpin(buttons[1].pin);
    gpio_init_inpin(buttons[2].pin);
    gpio_init_inpin(buttons[3].pin);
    gpio_init_inpin(buttons[4].pin);
    gpio_init_inpin(buttons[5].pin);
}