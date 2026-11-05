#include "block-task/block-task.h"
#include "platform/platform.h"
static block_task_callback_t on_block;
static bool running;
void block_task_init(void) {
    running = false;
}
bool block_task_start(uint32_t count, block_task_callback_t callback) {
    if (running || !platform_block_start(count))
        return false;
    on_block = callback;
    running = true;
    return true;
}
void block_task_handle(void) {
    if (!running)
        return;
    const uint16_t *samples;
    uint32_t count, elapsed;
    if (platform_block_take(&samples, &count, &elapsed)) {
        running = false;
        if (on_block)
            on_block(samples, count, elapsed);
    }
}
