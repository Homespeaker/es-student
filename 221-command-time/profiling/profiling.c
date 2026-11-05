#include "profiling.h"
#include <stdio.h>
#include <stdint.h>
#include "pico/time.h"
static uint32_t previous, maximum;
static int64_t mean_scaled;
/* About 2^16 iterations/s gives a one-second EMA time constant; Q16 keeps fractions. */
#define EMA_SHIFT 16
void profiling_init(void) {
    previous = time_us_32();
    mean_scaled = 0;
    maximum = 0;
}
void profiling_iteration(void) {
    uint32_t now = time_us_32(), elapsed = now - previous;
    previous = now;
    if (elapsed > maximum)
        maximum = elapsed;
    mean_scaled += (((int64_t)elapsed << EMA_SHIFT) - mean_scaled) >> EMA_SHIFT;
}
void main_time_exec(void) {
    printf("iteration avg %llu.%02llu us, max %lu us\n",
           (unsigned long long)(mean_scaled >> EMA_SHIFT),
           (unsigned long long)(((mean_scaled & ((1 << EMA_SHIFT) - 1)) * 100) >> EMA_SHIFT),
           (unsigned long)maximum);
}
void main_time_reset(void) {
    maximum = 0;
    printf("main: max reset\n");
}
uint32_t profiling_avg_us(void) {
    return (uint32_t)(mean_scaled >> EMA_SHIFT);
}
uint32_t profiling_max_us(void) {
    return maximum;
}
void profiling_reset_max(void) {
    maximum = 0;
}
