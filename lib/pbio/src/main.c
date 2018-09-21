/**
 * \addtogroup Main Library initialization and events
 * @{
 */

#include <stdbool.h>

#include <pbdrv/light.h>
#include <pbdrv/ioport.h>
#include <pbdrv/motor.h>

#include <pbio/light.h>

/**
 * Initialize the Pybricks I/O Library. This function must be called once,
 * usually at the beginning of a program, before using any other functions in
 * the library.
 */
void pbio_init(void) {
    pbdrv_light_init();
    pbdrv_ioport_init();
    pbdrv_motor_init();
}

/**
 * Checks for and performs pending background tasks. This function is meant to
 * be called once every millisecond in an event loop.
 */
void pbio_poll(void) {
    pbdrv_ioport_poll();
}

/**
 * Performs a "soft" reset. This will stop all motors, turn off lights, etc.
 * It is meant to be called at the end of a user program when the Pybricks
 * I/O library is used at the O/S level.
 */
void pbio_reset(void) {
    pbio_light_set_user_mode(false);

    // TODO: this should probably call the higher-level pbio_dcmotor_coast() function
    for (pbio_port_t p = PBDRV_CONFIG_FIRST_MOTOR_PORT; p <= PBDRV_CONFIG_LAST_MOTOR_PORT; p++) {
        pbdrv_motor_coast(p);
    }

    // TODO: this should stop sound playback on systems that support it
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
/**
 * Releases all resources used by the library. Calling this function is
 * optional. It should be called once at the end of a program. No other
 * functions may be called after this.
 */
void pbio_deinit(void) {
    pbdrv_motor_deinit();
    pbdrv_light_deinit();
}
#endif

/** @}*/
