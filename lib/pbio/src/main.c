/**
 * \addtogroup Main Library initialization and events
 * @{
 */

#include <stdbool.h>

#include <pbdrv/button.h>
#include <pbdrv/light.h>
#include <pbdrv/ioport.h>
#include <pbdrv/motor.h>
#include <pbsys/sys.h>

/**
 * Initialize the Pybricks I/O Library. This function must be called once,
 * usually at the beginning of a program, before using any other functions in
 * the library.
 */
void pbio_init(void) {
    _pbdrv_button_init();
    _pbdrv_light_init();
    _pbdrv_ioport_init();
    _pbdrv_motor_init();
}

/**
 * Checks for and performs pending background tasks. This function is meant to
 * be called once every millisecond in an event loop.
 */
void pbio_poll(void) {
    // TODO: if it has been < 1ms since last call return early for efficiency
    _pbdrv_ioport_poll();
    _pbio_light_poll();
    _pbsys_poll();
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
/**
 * Releases all resources used by the library. Calling this function is
 * optional. It should be called once at the end of a program. No other
 * functions may be called after this.
 */
void pbio_deinit(void) {
    _pbdrv_motor_deinit();
    _pbdrv_light_deinit();
    _pbdrv_button_deinit();
}
#endif

/** @}*/
