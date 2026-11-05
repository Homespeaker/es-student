#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "stream-task/stream-task.h"
#include "platform/platform.h"
static unsigned bits, received;
static bool available;
bool platform_stream_start(unsigned b) {
    bits = b;
    available = true;
    return true;
}
bool platform_stream_take(void *data, uint16_t *sequence, uint16_t *count) {
    if (!available)
        return false;
    available = false;
    memset(data, 0x0a, 1024);
    *sequence = 65535;
    *count = bits == 8 ? 1024 : 512;
    return true;
}
void platform_stream_stop(void) {}
uint32_t platform_stream_overruns(void) {
    return 0;
}
bool platform_timer_blink_start(uint32_t ms) {
    assert(ms == 250);
    return true;
}
uint32_t platform_timer_blink_stop(void) {
    return 0;
}
static void frame(const void *data, unsigned bytes) {
    const unsigned char *b = data;
    assert(bytes == 1030 && b[0] == 0xa5 && b[1] == 0x5a && b[2] == 0xff && b[3] == 0xff);
    assert(b[4] == 0 && b[5] == (bits == 8 ? 4 : 2));
    for (unsigned i = 6; i < bytes; ++i)
        assert(b[i] == 0x0a);
    ++received;
}
int main(void) {
    stream_task_init(frame);
    assert(stream_task_start(16));
    assert(!stream_task_start(16));
    stream_task_handle();
    stream_task_handle();
    stream_result_t r = stream_task_stop();
    assert(received == 1 && r.frames == 1 && r.samples == 512 && r.overruns == 0);
    assert(stream_task_start(8));
    stream_task_handle();
    r = stream_task_stop();
    assert(received == 2 && r.samples == 1024);
    puts("Binary frame layout and 8/16-bit stream passed");
}
