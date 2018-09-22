/**
 * \addtogroup Main Library initialization and events
 * @{
 */

#include <stdbool.h>

#include <pbdrv/button.h>
#include <pbdrv/light.h>
#include <pbdrv/ioport.h>
#include <pbdrv/motor.h>
#include <pbdrv/time.h>
#include <pbsys/sys.h>

static uint16_t prev_fast_poll_time;
static uint16_t prev_slow_poll_time;

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
    uint16_t now = pbdrv_time_get_msec();

    // pbio_poll() can be called quite frequently (e.g. in a tight loop) so we
    // don't want to call all of the subroutines unless enough time has
    // actually elapsed to do something useful.
    if (now - prev_fast_poll_time >= 2) {
        _pbdrv_ioport_poll(now);
        prev_fast_poll_time = now;
    }
    if (now - prev_slow_poll_time >= 32) {
        _pbio_light_poll(now);
        _pbsys_poll(now);
        prev_slow_poll_time = now;
    }
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
