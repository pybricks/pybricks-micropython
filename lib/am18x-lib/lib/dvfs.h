// tary, 14:17 2015/5/9
#ifndef __DVFS_H__
#define __DVFS_H__

enum {
	OPP_OSC		= 0,
	OPP_100M,
	OPP_200M,
	OPP_375M,
	OPP_456M,
	OPP_CNT,
};

int dvfs_get_opp(void);
int dvfs_get_volt(int opp);
int dvfs_get_freq(int opp);
int dvfs_set_opp(int opp);

#endif//__DVFS_H__
