// tary, 21:57 2013/3/6

#ifndef __AM18X_TIMER_H__
#define __AM18X_TIMER_H__

#include "am18x_map.h"

typedef enum {
	TIMER_MODE_64_BIT,
	TIMER_MODE_32_BIT_UNCHANINED,
	TIMER_MODE_32_BIT_CHAINED,
	TIMER_MODE_WATCHDOG,
} timer_mode_t;

typedef enum {
	TIMER_RESULT_NONE,
	TIMER_RESULT_INTERRUPT,
	TIMER_RESULT_DMA_SYNC,
	TIMER_RESULT_OUTPUT,
	TIMER_RESULT_CAPTURE,
} timer_result_t;

typedef enum {
	TIMER_SOURCE_INTERNAL,
	TIMER_SOURCE_EXTERNAL,
} timer_source_t;

typedef enum {
	TIMER_OPER_ONE_TIME,
	TIMER_OPER_CONTINUOUS,
	TIMER_OPER_RELOAD,
} timer_operation_t;

typedef enum {
	TIMER_CMD_PAUSE,
	TIMER_CMD_RESTART,
	TIMER_CMD_RESET,
	TIMER_CMD_START,
	TIMER_CMD_RELOAD,
	TIMER_CMD_INTR_CLEAR,
} timer_cmd_t;

typedef struct {
	uint32_t	mode;
	uint32_t	result;
	uint32_t	source;
	uint32_t	operation;
	uint32_t	period;
} timer_conf_t;

uint32_t timer_input_freq(TIMER_con_t* tcon);
am18x_rt timer_conf(TIMER_con_t* tcon, const timer_conf_t* conf);
am18x_rt timer_cmd(TIMER_con_t* tcon, timer_cmd_t cmd, uint32_t arg);
uint32_t timer_get_count(TIMER_con_t* tcon);

#endif//__AM18X_TIMER_H__
