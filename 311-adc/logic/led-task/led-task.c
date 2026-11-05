#include "led-task/led-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
static led_state_t state;
static uint32_t period_ms;
static uint64_t last_us;
static bool output;
void led_task_init(void) {
    period_ms = 1000;
    led_task_set_state(LED_STATE_BLINK);
}
void led_task_set_state(led_state_t s) {
    state = s;
    output = s == LED_STATE_ON || s == LED_STATE_BLINK;
    last_us = systime_us();
    platform_led_set(output);
}
void led_task_next_state(void) {
    led_task_set_state((state + 1) % 3);
}
void led_task_set_period_ms(uint32_t ms) {
    if (ms) {
        period_ms = ms;
        last_us = systime_us();
    }
}
uint32_t led_task_get_period_ms(void) {
    return period_ms;
}
led_state_t led_task_get_state(void) {
    return state;
}
void led_task_handle(void) {
    switch (state) {
    case LED_STATE_BLINK:
        if (systime_period_elapsed(&last_us, (uint64_t)period_ms * 500)) {
            output = !output;
            platform_led_set(output);
        }
        break;
    case LED_STATE_OFF:
    case LED_STATE_ON:
        break;
    }
}
