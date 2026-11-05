#pragma once
#include <stdbool.h>
#include <stdint.h>
#define STREAM_SYNC_WORD 0x5aa5
typedef void (*stream_task_callback_t)(const void *frame, unsigned bytes);
typedef struct {
    uint32_t frames, samples, overruns;
} stream_result_t;
void stream_task_init(stream_task_callback_t callback);
bool stream_task_start(unsigned bits);
void stream_task_handle(void);
stream_result_t stream_task_stop(void);
