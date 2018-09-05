#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <pbio/motorcontrol.h>

#define PERIOD_MS 10

struct periodic_info {
	int timer_fd;
	unsigned long long wakeups_missed;
};

void pybricks_init();

void pybricks_deinit();