

#ifndef _PBIO_CONFIG_H_
#define _PBIO_CONFIG_H_

// this is defined per-platform
#include "pbioconfig.h"

#ifndef PBIO_CONFIG_HAS_PORT_A
#define PBIO_CONFIG_HAS_PORT_A (0)
#endif
#ifndef PBIO_CONFIG_HAS_PORT_B
#define PBIO_CONFIG_HAS_PORT_B (0)
#endif
#ifndef PBIO_CONFIG_HAS_PORT_C
#define PBIO_CONFIG_HAS_PORT_C (0)
#endif
#ifndef PBIO_CONFIG_HAS_PORT_D
#define PBIO_CONFIG_HAS_PORT_D (0)
#endif
#ifndef PBIO_CONFIG_HAS_PORT_1
#define PBIO_CONFIG_HAS_PORT_1 (0)
#endif
#ifndef PBIO_CONFIG_HAS_PORT_2
#define PBIO_CONFIG_HAS_PORT_2 (0)
#endif
#ifndef PBIO_CONFIG_HAS_PORT_3
#define PBIO_CONFIG_HAS_PORT_3 (0)
#endif
#ifndef PBIO_CONFIG_HAS_PORT_4
#define PBIO_CONFIG_HAS_PORT_4 (0)
#endif

#ifndef PBIO_MOTOR_COUNT_PER_ROT
/**
 * The number of motor tacho counts per one rotation. Use this value to convert
 * counts to degrees or rate to RPM, for example.
 */
#define PBIO_MOTOR_COUNT_PER_ROT 360
#endif

#endif // _PBIO_CONFIG_H_
