#include "telemetry-task/telemetry-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
static telemetry_state_t state;
static uint64_t last_us;
static uint32_t period_ms;
static telemetry_task_callback_t on_record;
void telemetry_task_init(telemetry_task_callback_t callback) {
    on_record = callback;
    state = TELEMETRY_STATE_IDLE;
}
bool telemetry_task_start(uint32_t ms) {
    if (ms < 10 || platform_sampling_busy())
        return false;
    period_ms = ms;
    last_us = systime_us() - (uint64_t)ms * 1000;
    state = TELEMETRY_STATE_RUN;
    return true;
}
void telemetry_task_stop(void) {
    state = TELEMETRY_STATE_IDLE;
}
telemetry_state_t telemetry_task_get_state(void) {
    return state;
}
uint32_t telemetry_task_get_period_ms(void) {
    return period_ms;
}
void telemetry_task_handle(void) {
    if (state == TELEMETRY_STATE_RUN && !platform_sampling_busy() &&
        systime_period_elapsed(&last_us, (uint64_t)period_ms * 1000)) {
        telemetry_record_t r = {.time_us = systime_us(),
                                .voltage_v = platform_voltage(),
                                .temp_c = platform_temperature()};
        if (on_record)
            on_record(&r);
    }
}
