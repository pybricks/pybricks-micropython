/**
*  \file button.h
*    
*  \brief This header contains function declarations to use the hardware buttons of the EV3 as well as enumerations to represent the buttons and their states
* 
*  \author Tobias Schießl, ev3ninja
*/

#pragma once

/**
* \brief This struct contains the information required to read a buttons state.
*
* Every buttons is red by reading the signal on 1 GPIO pin which is configured as input.
*/
typedef struct button_info {
    unsigned int 	pin;	///< The pin representing the buttons state
} button_info;

/**
* \brief This enumeration maps the buttons to unique IDs.
*/
typedef enum button_id {
    BUTTON_LEFT     = 0x00,	///< The left button of the EV3
    BUTTON_RIGHT    = 0x01,	///< The right button of the EV3
    BUTTON_TOP      = 0x02,	///< The top button of the EV3
    BUTTON_BOTTOM   = 0x03,	///< The bottom button of the EV3
    BUTTON_CENTER   = 0x04,	///< The center button of the EV3
    BUTTON_BACK     = 0x05,	///< The upper left button of the EV3
    BUTTON_NONE     = 0xFF 	///< ID representing no button
} button_id;

/**
* \brief This enumeration represents a button's state (pressed = DOWN or not pressed = UP)
*/
typedef enum button_state {
    BUTTON_UP       = 0x00, ///< State representing that the button was not pressed
    BUTTON_DOWN     = 0x01 	///< State representing that the button was pressed
} button_state;

button_state 	button_get_state	(button_id button);
button_id 		button_get_pressed	(void);
void 			button_init			(void);