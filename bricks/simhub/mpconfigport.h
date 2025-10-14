#include <stdint.h>

#define MICROPY_HW_BOARD_NAME                   "Desktop"
#define MICROPY_HW_MCU_NAME                     "Desktop"

#define PYBRICKS_HUB_CLASS_NAME                 (MP_QSTR_VirtualHub)

#define PYBRICKS_HUB_NAME                       "virtualhub"
#define PYBRICKS_HUB_VIRTUALHUB                 (1)

// Pybricks modules
#define PYBRICKS_PY_COMMON                      (1)
#define PYBRICKS_PY_COMMON_BLE                  (0)
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
#define PYBRICKS_PY_EV3DEVICES                  (0)
#define PYBRICKS_PY_EXPERIMENTAL                (1)
#define PYBRICKS_PY_HUBS                        (1)
#define PYBRICKS_PY_IODEVICES                   (0)
#define PYBRICKS_PY_NXTDEVICES                  (0)
#define PYBRICKS_PY_PARAMETERS                  (1)
#define PYBRICKS_PY_PARAMETERS_BUTTON           (1)
#define PYBRICKS_PY_PARAMETERS_ICON             (0)
#define PYBRICKS_PY_PARAMETERS_IMAGE            (0)
#define PYBRICKS_PY_PUPDEVICES                  (1)
#define PYBRICKS_PY_PUPDEVICES_REMOTE           (0)
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
#define PYBRICKS_OPT_FLOAT                      (0)
#define PYBRICKS_OPT_TERSE_ERR                  (0)
#define PYBRICKS_OPT_EXTRA_LEVEL1               (0)
#define PYBRICKS_OPT_EXTRA_LEVEL2               (0)
#define PYBRICKS_OPT_CUSTOM_IMPORT              (0)
#define PYBRICKS_OPT_NATIVE_MOD                 (0)

#include "../_common/mpconfigport.h"
