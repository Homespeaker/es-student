#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
typedef enum { RELEASED, HELD, LONG_SENT } gesture_t;
typedef struct {
    bool candidate, stable;
    uint64_t changed_us, down_us;
    uint32_t presses;
    gesture_t gesture;
    button_task_callback_t short_action, long_action;
} button_t;
static button_t buttons[2];
void button_task_init(unsigned i, button_task_callback_t short_action,
                      button_task_callback_t long_action) {
    if (i >= 2)
        return;
    buttons[i] = (button_t){.candidate = platform_button_read(i),
                            .changed_us = systime_us(),
                            .short_action = short_action,
                            .long_action = long_action};
}
void button_task_handle(void) {
    uint64_t now = systime_us();
    for (unsigned i = 0; i < 2; ++i) {
        button_t *b = &buttons[i];
        bool raw = platform_button_read(i);
        if (raw != b->candidate) {
            b->candidate = raw;
            b->changed_us = now;
        }
        if (b->stable != b->candidate && now - b->changed_us >= 20000) {
            b->stable = b->candidate;
            if (b->stable) {
                ++b->presses;
                b->down_us = now;
                b->gesture = HELD;
                if (!b->long_action && b->short_action)
                    b->short_action();
            } else {
                if (b->gesture == HELD && b->long_action && b->short_action)
                    b->short_action();
                b->gesture = RELEASED;
            }
        }
        switch (b->gesture) {
        case HELD:
            if (b->stable && b->long_action && now - b->down_us >= 1000000) {
                b->gesture = LONG_SENT;
                b->long_action();
            }
            break;
        case RELEASED:
        case LONG_SENT:
            break;
        }
    }
}
bool button_task_is_pressed(unsigned i) {
    return i < 2 && buttons[i].stable;
}
uint32_t button_task_get_press_count(unsigned i) {
    return i < 2 ? buttons[i].presses : 0;
}
