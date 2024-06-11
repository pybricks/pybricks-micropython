// tary, 17:10 2013/3/9
#include "am18x_lib.h"
#include "auxlib.h"
#include "systick.h"

#define TIMER_NR		TIMER0
#define TIMER_INTR_NR		AINTC_T64P0_TINT12

static volatile systick_handler_t systick_handler = NULL;
static volatile unsigned long ticks = 0;
static unsigned inner_period = 1000U;
static void systick_isr(void);
static int dummy_handler(int ticks) { return 0;}

int systick_init(unsigned period) {
	am18x_rt r;
	timer_conf_t tconf[1] = {{
		TIMER_MODE_32_BIT_UNCHANINED,
		TIMER_RESULT_INTERRUPT,
		TIMER_SOURCE_INTERNAL,
		TIMER_OPER_CONTINUOUS,
		240000UL,
	}};

	tconf->period = timer_input_freq(TIMER_NR) * period / 1000;
	inner_period = period;

	r = timer_conf(TIMER_NR, tconf);
	if (AM18X_OK != r) {
		printk("error timer_conf()\n");
	}

	systick_set_handler(NULL);

	isr_set_handler(TIMER_INTR_NR, systick_isr);

	aintc_sys_enable(TIMER_INTR_NR);

	return 0;
}

int systick_start(void) {
	timer_cmd(TIMER_NR, TIMER_CMD_START, 0);
	return 0;
}

int systick_stop(void) {
	timer_cmd(TIMER_NR, TIMER_CMD_PAUSE, 0);
	return 0;
}

int systick_set_handler(systick_handler_t handle) {
	systick_handler_t l_handle = systick_handler;

	if (handle == NULL) {
		handle = dummy_handler;
	}
	systick_handler = handle;
	return (int)l_handle;
}

unsigned systick_elapsed(void) {
	return ticks * inner_period;
}

unsigned systick_period(void) {
	return inner_period;
}

int systick_timeout(int msecs) {
	static volatile unsigned long l_ticks;

	if (msecs == 0) {
		l_ticks = ticks;
		return 0;
	}
	if (ticks * inner_period >= msecs + l_ticks * inner_period) {
		l_ticks = ticks;
		return 1;
	}
	return 0;
}

int systick_sleep(int msecs) {
	unsigned long goal;

	goal = msecs + ticks * inner_period;
	while (goal > ticks * inner_period) {
		;
	}
	return 0;
}

static void systick_isr(void) {

	if (systick_handler != NULL) {
		systick_handler(ticks++);
	}

	timer_cmd(TIMER_NR, TIMER_CMD_INTR_CLEAR, 0);

	return;
}
