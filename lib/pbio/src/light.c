/**
 * \addtogroup Light Light control functions
 * @{
 */

#include <stdbool.h>

#include <pbdrv/light.h>

static bool pbio_light_user_mode;

/**
 * Enables or disables user mode. When user mode is enabled, all lights will
 * respond to user functions. When disabled, all lights will only respond to
 * system functions.
 * @param user_mode     *true* to enable user mode, otherwise *false*
 */
void pbio_light_set_user_mode(bool user_mode) {
    pbio_light_user_mode = user_mode;
    if (user_mode) {
        // TODO: This should be a nice breathe effect or something
        pbdrv_light_set_color(PBIO_PORT_SELF, 0, 255, 0);
        pbdrv_light_set_pattern(PBIO_PORT_SELF, PBDRV_LIGHT_PATTERN_ON);
    }
    else {
        // TODO: This needs to turn off all lights except for the brick status
        // light. The brick status light should be set to indicate the system
        // state.
        pbdrv_light_set_color(PBIO_PORT_SELF, 0, 0, 255);
        pbdrv_light_set_pattern(PBIO_PORT_SELF, PBDRV_LIGHT_PATTERN_ON);
    }
}

// TODO: need to add functions for setting the user state and system state for
// individual lights.

// TODO: need to add a poll function to update blinking lights

/** @}*/
