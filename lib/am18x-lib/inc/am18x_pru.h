// tary, 22:45 2013/5/10

#ifndef __AM18X_PRU_H__
#define __AM18X_PRU_H__

#include "am18x_map.h"

typedef enum {
	PRU_CMD_DISABLE,
	PRU_CMD_ENABLE,
	PRU_CMD_RUN,
	PRU_CMD_IS_HALT,
} pru_cmd_t;

am18x_rt pru_load(PRU_con_t* pcon, const uint32_t* inst, uint32_t count);
am18x_rt pru_cmd(PRU_con_t* pcon, pru_cmd_t cmd, uint32_t arg);
am18x_rt pru_dump_regs(PRU_con_t* pcon);

#endif//__AM18X_PRU_H__
