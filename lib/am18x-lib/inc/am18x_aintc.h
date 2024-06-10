// tary, 0:01 2013/3/9

#ifndef __AM18X_AINTC_H__
#define __AM18X_AINTC_H__

#include "am18x_map.h"

typedef enum {
	AINTC_GLOBAL,
	AINTC_HOST_FIQ,
	AINTC_HOST_IRQ,
} aintc_host_t;

typedef struct {
	uint32_t	isr_addr;
	uint32_t	isr_size;
} aintc_conf_t;

#define AINTC_INVALID_ACTIVE		(-1)

am18x_rt aintc_conf(const aintc_conf_t* conf);
am18x_rt aintc_enable(aintc_host_t host);
am18x_rt aintc_disable(aintc_host_t host);
am18x_rt aintc_sys_enable(AINTC_assign_t assign);
am18x_rt aintc_sys_disable(AINTC_assign_t assign);
am18x_rt aintc_trigger(AINTC_assign_t assign);
am18x_rt aintc_clear(AINTC_assign_t assign);
int32_t	 aintc_get_active(void);

#endif//__AM18X_AINTC_H__
