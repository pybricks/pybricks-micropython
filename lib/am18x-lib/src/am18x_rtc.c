// tary, 20:03 2016/6/4
#include "am18x_rtc.h"

am18x_rt rtc_conf(RTC_con_t* rtc, const rtc_tm_t* tm) {
	uint32_t reg;

	assert(rtc);
	assert(tm);

	rtc->KICKnR[0] = RTC_KICK0R_VAL;
	rtc->KICKnR[1] = RTC_KICK1R_VAL;

	if (tm->cflags & RTC_DISABLE) {
		reg = rtc->CTRL;
		reg = FIELD_SET(reg, RTCTL_RUN_MASK, RTCTL_RUN_no);
		rtc->CTRL = reg;
		rtc->CTRL = FIELD_SET(reg, RTCTL_DISABLE_MASK, RTCTL_DISABLE_yes);
	}
	if (tm->cflags & RTC_ENABLE) {
		reg = rtc->CTRL;
		reg = FIELD_SET(reg, RTCTL_DISABLE_MASK, RTCTL_DISABLE_no);
		rtc->CTRL = reg;
		rtc->CTRL = FIELD_SET(reg, RTCTL_RUN_MASK, RTCTL_RUN_yes);
	}

	if (tm->cflags & RTC_TICKINT_DISABLE) {
		reg = rtc->INTERRUPT;
		rtc->INTERRUPT = FIELD_SET(reg, RTCINT_TIMER_MASK, RTCINT_TIMER_disabled);
	}
	if (tm->cflags & RTC_TICKINT_ENABLE) {
		reg = rtc->INTERRUPT;
		reg = FIELD_SET(reg, RTCINT_TIMER_MASK, RTCINT_TIMER_enabled);
		rtc->INTERRUPT = FIELD_SET(reg, RTCINT_EVERY_MASK, RTCINT_EVERY_Second);
	}

	if (tm->cflags & RTC_SET_TIME) {
		rtc->SECOND = BIN2BCD(tm->second);
		rtc->MINUTE = BIN2BCD(tm->minute);
		rtc->HOUR = BIN2BCD(tm->hour);
		rtc->DAY = BIN2BCD(tm->day);
		rtc->MONTH = BIN2BCD(tm->month);
		rtc->YEAR = BIN2BCD(tm->year - 2000);
	}

	return AM18X_OK;
}

am18x_rt rtc_alarm(RTC_con_t* rtc, const rtc_tm_t* tm) {
	uint32_t reg, v;

	assert(rtc);

	reg = rtc->INTERRUPT;
	v = (tm == NULL)? RTCINT_ALARM_disabled: RTCINT_ALARM_enabled;
	rtc->INTERRUPT = reg = FIELD_SET(reg, RTCINT_ALARM_MASK, v);
	if (tm == NULL) {
		return AM18X_ERR;
	}

	rtc->INTERRUPT = FIELD_SET(reg, RTCINT_EVERY_MASK, RTCINT_EVERY_Second);
	rtc->ALARMSECOND = BIN2BCD(tm->second);
	rtc->ALARMMINUTE = BIN2BCD(tm->minute);
	rtc->ALARMHOUR = BIN2BCD(tm->hour);
	rtc->ALARMDAY = BIN2BCD(tm->day);
	rtc->ALARMMONTH = BIN2BCD(tm->month);
	rtc->ALARMYEAR = BIN2BCD(tm->year - 2000);

	return AM18X_OK;
}

am18x_rt rtc_state(const RTC_con_t* rtc, rtc_tm_t* tm) {
	uint8_t reg;

	assert(rtc);
	assert(tm);

	reg = rtc->STATUS;
	if (FIELD_GET(reg, RTCSTAT_RUN_MASK) != RTCSTAT_RUN_yes) {
		return AM18X_ERR;
	}

	tm->second = BCD2BIN(rtc->SECOND);
	tm->minute = BCD2BIN(rtc->MINUTE);
	tm->hour = BCD2BIN(rtc->HOUR);
	tm->weekday = rtc->DOTW;
	tm->day = BCD2BIN(rtc->DAY);
	tm->month = BCD2BIN(rtc->MONTH);
	tm->year = BCD2BIN(rtc->YEAR) + 2000;

	reg = rtc->INTERRUPT;
	if (FIELD_GET(reg, RTCINT_TIMER_MASK) == RTCINT_TIMER_enabled) {
		tm->cflags = RTC_TICKINT_ENABLE;
		// TODO: 
		tm->tickcnt = 1;
	} else {
		tm->cflags = RTC_TICKINT_DISABLE;
	}

	return AM18X_OK;
}
