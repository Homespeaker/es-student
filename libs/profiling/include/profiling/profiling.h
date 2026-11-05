#pragma once
#include <stdint.h>
#ifndef PROFILING_ENABLED
#define PROFILING_ENABLED 1
#endif
typedef struct {
    const char *name;
    uint64_t count, total_us;
    uint32_t max_us;
} profiling_result_t;
#if PROFILING_ENABLED
void profiling_stopwatch_init(unsigned index, const char *name);
void profiling_start(unsigned index);
void profiling_stop(unsigned index);
void profiling_get(unsigned index, profiling_result_t *result);
void profiling_reset(void);
#else
#define profiling_stopwatch_init(...) ((void)0)
#define profiling_start(...) ((void)0)
#define profiling_stop(...) ((void)0)
#define profiling_get(index, result) ((void)(index), (void)(result))
#define profiling_reset() ((void)0)
#endif
