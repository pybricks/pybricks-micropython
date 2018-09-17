#include <pbdrv/time.h>
#include <time.h>

pbio_error_t pbdrv_get_time_usec(uint32_t *time_now){
    struct timespec time_val;
    clock_gettime(CLOCK_MONOTONIC, &time_val);
    *time_now = time_val.tv_sec*1000000 + time_val.tv_nsec / 1000;
    return PBIO_SUCCESS;
}
