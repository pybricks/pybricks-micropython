// tary, 23:50 2013/10/27

#ifndef __AM18X_DDR_H__
#define __AM18X_DDR_H__

#include "am18x_map.h"

typedef struct {
	uint8_t		ddr2_not_mddr;
	uint8_t		page_size;	// 8..11
	uint8_t		row_size;	// 9..16
	uint8_t		bank_cnt;	// 1,2,4,8

	uint32_t	freq_ck;
	uint32_t	trefi;		// ns

	uint8_t		cl;		// 2..3
	uint8_t		trfc;		// ns
	uint8_t		trp;		// ...
	uint8_t		trcd;

	uint8_t		twr;
	uint8_t		tras;
	uint8_t		trc;
	uint8_t		trrd;

	uint8_t		twtr;
	uint8_t		todt;
	uint8_t		txsnr;
	uint8_t		trtp;

	uint8_t		txp;		// tck
	uint8_t		txsrd;		// tck
	uint8_t		tcke;		// tck

	uint32_t	trasmax;

	uint8_t		pasr;
} ddr_conf_t;

am18x_rt ddr_initialize(DDR_con_t* dcon, const ddr_conf_t* conf);
am18x_rt ddr_clock_off(DDR_con_t* dcon);
am18x_rt ddr_clock_on(DDR_con_t* dcon);

#endif//__AM18X_DDR_H__
