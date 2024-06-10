// tary, 20:48 2016/4/1

#ifndef __AM18X_RTC_H__
#define __AM18X_RTC_H__

#include "am18x_map.h"

#define BCD2BIN(val)	(((val) & 0x0F) + ((val) >> 4) * 10)
#define BIN2BCD(val)	((((val) / 10) << 4) + (val) % 10)

typedef enum {
	RTC_DISABLE = BIT(0),
	RTC_ENABLE = BIT(1),
	RTC_TICKINT_DISABLE = BIT(2),
	RTC_TICKINT_ENABLE = BIT(3),
	RTC_SET_TIME = BIT(4),
} rtc_cflag_t;

#define RTC_INVALID_VAL		(0xFF)

typedef struct {
	uint16_t	year;
	uint8_t 	month;
	uint8_t 	day;
	uint8_t 	weekday;
	uint8_t		hour;
	uint8_t 	minute;
	uint8_t		second;
	uint8_t		tickcnt;
	uint8_t		cflags;
} rtc_tm_t;

am18x_rt rtc_conf(RTC_con_t* rtc, const rtc_tm_t* tm);
am18x_rt rtc_alarm(RTC_con_t* rtc, const rtc_tm_t* tm);
am18x_rt rtc_state(const RTC_con_t* rtc, rtc_tm_t* tm);

#endif//__AM18X_RTC_H__
