// qstrs specific to all Pybricks MicroPython ports.
// *FORMAT-OFF*

#include "py/mpconfig.h"

#if !PYBRICKS_HUB_MOVEHUB
Q(Li-ion)
#endif

#if PYBRICKS_PY_EV3DEVICES
Q(pybricks.ev3devices)
#endif

#if PYBRICKS_PY_EXPERIMENTAL
Q(pybricks.experimental)
#endif

#if PYBRICKS_PY_COMMON_BTC
Q(pybricks.experimental.btc)
#endif

#if MICROPY_PY_BUILTINS_FLOAT // backwards compatibility with Pybricks V3.2, maps to pybricks.tools
Q(pybricks.geometry)
#endif

#if PYBRICKS_PY_HUBS
Q(pybricks.hubs)
#endif

#if PYBRICKS_PY_IODEVICES
Q(pybricks.iodevices)
#endif

#if PYBRICKS_PY_NXTDEVICES
Q(pybricks.nxtdevices)
#endif

#if PYBRICKS_PY_PARAMETERS
Q(pybricks.parameters)
#endif

#if PYBRICKS_PY_PUPDEVICES || PYBRICKS_PY_EV3_PUP_ALIAS
Q(pybricks.pupdevices)
#endif

#if PYBRICKS_PY_ROBOTICS
Q(pybricks.robotics)
#endif

#if PYBRICKS_PY_TOOLS
Q(pybricks.tools)
#endif
