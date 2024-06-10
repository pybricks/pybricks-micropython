// tary, 20:42 2017/6/20

#ifndef __AM18X_ECAP_H__
#define __AM18X_ECAP_H__

#include "am18x_map.h"

typedef enum {
	ECAP_DISABLE = 0,
	ECAP_ENABLE = BIT(0),
	ECAP_ACTIVE_LOW = 0,
	ECAP_ACTIVE_HIGH = BIT(1),
} ecap_cflag_t;

typedef struct {
	uint32_t	freq;
	uint32_t	duty;
	uint8_t		cflags;
} ecap_conf_t;

am18x_rt ecap_init(ECAP_con_t* econ);
am18x_rt ecap_get_conf(ECAP_con_t* econ, ecap_conf_t* conf);
am18x_rt ecap_set_conf(ECAP_con_t* econ, const ecap_conf_t* conf);

#endif//__AM18X_ECAP_H__
