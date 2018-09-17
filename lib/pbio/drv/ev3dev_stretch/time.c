#include <pbdrv/time.h>
#include <time.h>

uint32_t pbdrv_get_time_usec(){
    struct timespec time_val;
    clock_gettime(CLOCK_MONOTONIC, &time_val);
    return (time_val.tv_sec*1000000 + time_val.tv_nsec / 1000);
}
