#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
static bool candidate, stable;
static uint64_t changed_us;
static uint32_t presses;
static button_task_callback_t on_press;
typedef enum { BUTTON_RELEASED, BUTTON_PRESSED } button_state_t;
static button_state_t state;
void button_task_init(button_task_callback_t callback) {
    on_press = callback;
    candidate = platform_button_read();
    stable = false;
    changed_us = systime_us();
    presses = 0;
    state = BUTTON_RELEASED;
}
void button_task_handle(void) {
    bool raw = platform_button_read();
    uint64_t now = systime_us();
    if (raw != candidate) {
        candidate = raw;
        changed_us = now;
    }
    if (candidate != stable && now - changed_us >= 20000) {
        stable = candidate;
        switch (state) {
        case BUTTON_RELEASED:
            if (stable) {
                state = BUTTON_PRESSED;
                ++presses;
                if (on_press)
                    on_press();
            }
            break;
        case BUTTON_PRESSED:
            if (!stable)
                state = BUTTON_RELEASED;
            break;
        }
    }
}
bool button_task_is_pressed(void) {
    return stable;
}
uint32_t button_task_get_press_count(void) {
    return presses;
}
