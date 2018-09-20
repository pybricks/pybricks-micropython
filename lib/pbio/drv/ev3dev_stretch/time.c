#include <time.h>
#include <stdint.h>

uint32_t pbdrv_time_get_msec() {
    struct timespec time_val;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_val);
    return (time_val.tv_sec*1000 + time_val.tv_nsec / 1000000);
}

uint32_t pbdrv_time_get_usec() {
    struct timespec time_val;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_val);
    return (time_val.tv_sec*1000000 + time_val.tv_nsec / 1000);
}
