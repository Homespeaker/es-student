#include "stream-task/stream-task.h"
#include "platform/platform.h"
static stream_task_callback_t on_frame;
static bool running;
static stream_result_t result;
static uint16_t frame[515];
void stream_task_init(stream_task_callback_t callback) {
    on_frame = callback;
    running = false;
}
bool stream_task_start(unsigned bits) {
    if (running || !platform_stream_start(bits))
        return false;
    result = (stream_result_t){0};
    running = true;
    platform_timer_blink_start(250);
    return true;
}
void stream_task_handle(void) {
    if (!running)
        return;
    uint16_t sequence, count;
    if (platform_stream_take(frame + 3, &sequence, &count)) {
        frame[0] = STREAM_SYNC_WORD;
        frame[1] = sequence;
        frame[2] = count;
        if (on_frame)
            on_frame(frame, sizeof(frame));
        ++result.frames;
        result.samples += count;
    }
}
stream_result_t stream_task_stop(void) {
    platform_stream_stop();
    result.overruns = platform_stream_overruns();
    running = false;
    platform_timer_blink_stop();
    return result;
}
