#include <stdint.h>
#include <pbdrv/config.h>

#define MICROPY_HW_BOARD_NAME                   "Desktop"
#define MICROPY_HW_MCU_NAME                     "Desktop"

#define PYBRICKS_HUB_CLASS_NAME                 (MP_QSTR_VirtualHub)

#define PYBRICKS_HUB_NAME                       "virtualhub"
#define PYBRICKS_HUB_VIRTUALHUB                 (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON                      (1)
#define PYBRICKS_PY_COMMON_BLE                  (1)
#define PYBRICKS_PY_COMMON_CHARGER              (1)
#define PYBRICKS_PY_COMMON_COLOR_LIGHT          (1)
#define PYBRICKS_PY_COMMON_CONTROL              (1)
#define PYBRICKS_PY_COMMON_IMU                  (0)
#define PYBRICKS_PY_COMMON_KEYPAD               (1)
#define PYBRICKS_PY_COMMON_KEYPAD_HUB_BUTTONS   (1)
#define PYBRICKS_PY_COMMON_LIGHT_ARRAY          (1)
#define PYBRICKS_PY_COMMON_LIGHT_MATRIX         (0)
#define PYBRICKS_PY_COMMON_LOGGER               (1)
#define PYBRICKS_PY_COMMON_LOGGER_REAL_FILE     (1)
#define PYBRICKS_PY_COMMON_MOTORS               (1)
#define PYBRICKS_PY_COMMON_SPEAKER              (0)
#define PYBRICKS_PY_COMMON_SYSTEM               (1)
#define PYBRICKS_PY_COMMON_SYSTEM_UMM_INFO      (1)
#define PYBRICKS_PY_EV3DEVICES                  (0)
#define PYBRICKS_PY_EXPERIMENTAL                (1)
#define PYBRICKS_PY_HUBS                        (1)
#define PYBRICKS_PY_IODEVICES                   (1)
#define PYBRICKS_PY_IODEVICES_ANALOG_SENSOR     (0)
#define PYBRICKS_PY_IODEVICES_DC_MOTOR          (0)
#define PYBRICKS_PY_IODEVICES_I2C_DEVICE        (0)
#define PYBRICKS_PY_IODEVICES_LUMP_DEVICE       (0)
#define PYBRICKS_PY_IODEVICES_LWP3_DEVICE       (1)
#define PYBRICKS_PY_IODEVICES_PUP_DEVICE        (0)
#define PYBRICKS_PY_IODEVICES_UART_DEVICE       (0)
#define PYBRICKS_PY_IODEVICES_XBOX_CONTROLLER   (1)
#define PYBRICKS_PY_MESSAGING                   (1)
#define PYBRICKS_PY_MESSAGING_RFCOMM            (1)
#define PYBRICKS_PY_NXTDEVICES                  (0)
#define PYBRICKS_PY_PARAMETERS                  (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON           (1)
#define PYBRICKS_PY_PARAMETERS_ICON             (0)
#define PYBRICKS_PY_PARAMETERS_IMAGE            (1)
#define PYBRICKS_PY_PUPDEVICES                  (1)
#define PYBRICKS_PY_PUPDEVICES_REMOTE           (1)
#define PYBRICKS_PY_DEVICES                     (1)
#define PYBRICKS_PY_ROBOTICS                    (1)
#define PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE    (0)
#define PYBRICKS_PY_TOOLS                       (1)
#define PYBRICKS_PY_TOOLS_HUB_MENU              (0)
#define PYBRICKS_PY_TOOLS_APP_DATA              (1)

// Pybricks options
#define PYBRICKS_OPT_COMPILER                   (1)
#define PYBRICKS_OPT_USE_STACK_END_AS_TOP       (1)
#define PYBRICKS_OPT_RAW_REPL                   (0)
#define PYBRICKS_OPT_FLOAT                      (1)
#define PYBRICKS_OPT_TERSE_ERR                  (0)
#define PYBRICKS_OPT_EXTRA_LEVEL1               (1)
#define PYBRICKS_OPT_EXTRA_LEVEL2               (1)
#define PYBRICKS_OPT_CUSTOM_IMPORT              (0)
#define PYBRICKS_OPT_NATIVE_MOD                 (0)

// The Virtual Hub has no hardware interrupt that requests polling every 1ms.
// We solve this by polling manually as appropriate for the simulation, as
// indicated below.
#if PBDRV_CONFIG_CLOCK_TEST
// In the CI variant ("counting clock"), the clock is advanced on every
// pbio_os_hook_wait_for_interrupt, which is called from mp_hal_delay_ms. But
// the user could be running a tight loop without any waits. We still want to
// advance the clock in those cases, which we mimic here by advancing the clock
// every couple of MicroPython byte codes. This also polls to the event loop.
#define PYBRICKS_VM_HOOK_LOOP_EXTRA \
    do { \
        extern void pbio_clock_test_advance_eventually(void); \
        pbio_clock_test_advance_eventually(); \
    } while (0);
#else
// When using the wall clock, time advances automatically but we still need to
// request polling. This is done at the end of pbio_os_hook_wait_for_interrupt.
// As above, we also need something to move it along with blocking user loops.
// Instead of guessing with a number of instructions, here we can just poll
// whenever the wall clock changes.
#define PYBRICKS_VM_HOOK_LOOP_EXTRA \
    do { \
        static uint32_t clock_last; \
        extern uint32_t pbdrv_clock_get_ms(void); \
        uint32_t clock_now = pbdrv_clock_get_ms(); \
        if (clock_last != clock_now) { \
            extern void pbio_os_request_poll(void); \
            pbio_os_request_poll(); \
            clock_last = clock_now; \
        } \
    } while (0);
#endif

// Allow printf for conventional purposes on native host, such as printing
// the help info for the executable. This will not go through the simulated
// i/o drivers, but just to stdout.
#define MICROPY_USE_INTERNAL_PRINTF             (0)

#include "../_common/mpconfigport.h"
