#include "pi-task/pi-task.h"
#include "systime/systime.h"
static uint32_t terms, index;
static uint64_t started_us;
static double sum;
static pi_result_t result;
typedef enum { PI_STATE_IDLE, PI_STATE_RUN, PI_STATE_DONE } pi_state_t;
static pi_state_t state;
#define PI_TERMS_PER_CALL 32
void pi_task_init(void) {
    result = (pi_result_t){0};
    state = PI_STATE_IDLE;
}
bool pi_task_start(uint32_t n) {
    if (result.running || !n)
        return false;
    terms = n;
    index = 0;
    sum = 0;
    started_us = systime_us();
    result = (pi_result_t){.started = true, .running = true};
    return true;
}
void pi_task_handle(void) {
    if (!result.running)
        return;
    /* 32 software-double divisions keep one slice below 1 ms on Cortex-M0+. */
    state = PI_STATE_RUN;
    for (unsigned slice = 0; slice < PI_TERMS_PER_CALL && index < terms; ++slice, ++index)
        sum += (index & 1 ? -1.0 : 1.0) / (2.0 * index + 1.0);
    if (index == terms) {
        result.value = 4.0 * sum;
        result.elapsed_ms = (systime_us() - started_us) / 1000;
        result.running = false;
        state = PI_STATE_DONE;
    }
}
void pi_task_get_result(pi_result_t *r) {
    *r = result;
    r->started = state != PI_STATE_IDLE || result.started;
}
