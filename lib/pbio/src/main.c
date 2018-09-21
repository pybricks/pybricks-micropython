/**
 * \addtogroup Main Library initialization and events
 * @{
 */

#include <stdbool.h>

#include <pbdrv/light.h>
#include <pbdrv/ioport.h>
#include <pbdrv/motor.h>

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
