

#ifndef _PBIO_PORT_H_
#define _PBIO_PORT_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

/**
 * \addtogroup Port I/O ports
 * @{
 */

/**
 * I/O port identifier. The meaning and availability of a port is device-specific.
 */
typedef enum {
    PBIO_PORT_SELF = -1, /**< Virtual port for the programmable brick itself */

#if PBDRV_CONFIG_HAS_PORT_A
    PBIO_PORT_A = 'A', /**< I/O port labeled as "A" */
#endif
#if PBDRV_CONFIG_HAS_PORT_B
    PBIO_PORT_B = 'B', /**< I/O port labeled as "B" */
#endif
#if PBDRV_CONFIG_HAS_PORT_C
    PBIO_PORT_C = 'C', /**< I/O port labeled as "C" */
#endif
#if PBDRV_CONFIG_HAS_PORT_D
    PBIO_PORT_D = 'D', /**< I/O port labeled as "D" */
#endif
#if PBDRV_CONFIG_HAS_PORT_1
    PBIO_PORT_1 = '1', /**< I/O port labeled as "1" */
#endif
#if PBDRV_CONFIG_HAS_PORT_2
    PBIO_PORT_2 = '2', /**< I/O port labeled as "2" */
#endif
#if PBDRV_CONFIG_HAS_PORT_3
    PBIO_PORT_3 = '3', /**< I/O port labeled as "3" */
#endif
#if PBDRV_CONFIG_HAS_PORT_4
    PBIO_PORT_4 = '4', /**< I/O port labeled as "4" */
#endif
} pbio_port_t;

/**
 * Gets the zero based index of a pbio_port_t motor port.
 * @param [in]  port    The motor port
 * @return              Zero based numeric index of the port
 */
#define motorindex(port)  (port-PBIO_PORT_A)

/** @}*/

#endif // _PBIO_PORT_H_
