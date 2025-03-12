/*
 * linux/<file location within the kernel tree>
 *
 * Copyright (C) 2010 Texas Instruments Incorporated
 * Author: Ganeshan N
 *
 * Based on <Give reference of old kernel file from which this file is derived from>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as  published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any kind,
 * whether express or implied; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef _SUART_UTILS_H_
#define _SUART_UTILS_H_



/*
 *====================
 * Includes
 *====================
 */

/* ************ Serializers ***************** */
#define PRU_SUART_SERIALIZER_0          (0u)
/** Serializer */
#define PRU_SUART_SERIALIZER_1          (1u)
/** Serializer */
#define PRU_SUART_SERIALIZER_2          (2u)
/** Serializer */
#define PRU_SUART_SERIALIZER_3          (3u)
/** Serializer */
#define PRU_SUART_SERIALIZER_4          (4u)
/** Serializer */
#define PRU_SUART_SERIALIZER_5          (5u)
/** Serializer */
#define PRU_SUART_SERIALIZER_6          (6u)
/** Serializer */
#define PRU_SUART_SERIALIZER_7          (7u)
/** Serializer */
#define PRU_SUART_SERIALIZER_8          (8u)
/** Serializer */
#define PRU_SUART_SERIALIZER_9          (9u)
/** Serializer */
#define PRU_SUART_SERIALIZER_10         (10u)
/** Serializer */
#define PRU_SUART_SERIALIZER_11         (11u)
/** Serializer */
#define PRU_SUART_SERIALIZER_12         (12u)
/** Serializer */
#define PRU_SUART_SERIALIZER_13         (13u)
/** Serializer */
#define PRU_SUART_SERIALIZER_14         (14u)
/** Serializer */
#define PRU_SUART_SERIALIZER_15         (15u)
/** Serializer */
#define PRU_SUART_SERIALIZER_NONE       (16u)

/* Total number of baud rates supported */
#define SUART_NUM_OF_BAUDS_SUPPORTED    13

#define MCASP_PDIR_VAL ( \
    CSL_MCASP_PDIR_AFSR_OUTPUT << CSL_MCASP_PDIR_AFSR_SHIFT | \
        CSL_MCASP_PDIR_AHCLKR_OUTPUT << CSL_MCASP_PDIR_AHCLKR_SHIFT | \
        CSL_MCASP_PDIR_ACLKR_OUTPUT << CSL_MCASP_PDIR_ACLKR_SHIFT | \
        CSL_MCASP_PDIR_AFSX_OUTPUT << CSL_MCASP_PDIR_AFSX_SHIFT | \
        CSL_MCASP_PDIR_AHCLKX_OUTPUT << CSL_MCASP_PDIR_AHCLKX_SHIFT | \
        CSL_MCASP_PDIR_ACLKX_OUTPUT << CSL_MCASP_PDIR_ACLKX_SHIFT)

extern void suart_mcasp_reset(arm_pru_iomap *pru_arm_iomap);
extern void suart_mcasp_config(uint32_t mcasp_addr,
    uint32_t txBaudValue,
    uint32_t rxBaudValue,
    uint32_t oversampling,
    arm_pru_iomap *pru_arm_iomap);

extern int16_t suart_asp_baud_set(uint32_t txBaudValue,
    uint32_t rxBaudValue,
    uint32_t oversampling,
    arm_pru_iomap *pru_arm_iomap);

extern void suart_mcasp_psc_disable(uint32_t psc1_addr);
extern void suart_mcasp_psc_enable(uint32_t psc1_addr);
extern int16_t suart_asp_serializer_deactivate(uint16_t u16srNum,
    arm_pru_iomap *pru_arm_iomap);

extern void suart_mcasp_tx_serialzier_set(uint32_t serializerNum,
    arm_pru_iomap *pru_arm_iomap);

#endif
