#include "profiling/profiling.h"
#if PROFILING_ENABLED
#include "pico/time.h"
static profiling_result_t watches[8];
static uint32_t starts[8];
void profiling_stopwatch_init(unsigned i, const char *name) {
    if (i < 8)
        watches[i].name = name;
}
void profiling_start(unsigned i) {
    if (i < 8)
        starts[i] = time_us_32();
}
void profiling_stop(unsigned i) {
    if (i >= 8)
        return;
    uint32_t dt = time_us_32() - starts[i];
    ++watches[i].count;
    watches[i].total_us += dt;
    if (dt > watches[i].max_us)
        watches[i].max_us = dt;
}
bool profiling_get(unsigned i, profiling_result_t *r) {
    if (i >= 8 || !r || !watches[i].name)
        return false;
    *r = watches[i];
    return true;
}
void profiling_reset(void) {
    for (unsigned i = 0; i < 8; ++i) {
        watches[i].count = 0;
        watches[i].total_us = 0;
        watches[i].max_us = 0;
    }
}
#endif
