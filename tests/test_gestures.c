#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "button-task/button-task.h"
static uint64_t now;
static bool pins[2];
static unsigned short_count[2], long_count;
uint64_t systime_us(void) {
    return now;
}
bool platform_button_read(unsigned i) {
    return pins[i];
}
static void short0(void) {
    ++short_count[0];
}
static void short1(void) {
    ++short_count[1];
}
static void long0(void) {
    ++long_count;
}
static void set(unsigned i, bool value) {
    pins[i] = value;
    button_task_handle();
    now += 20000;
    button_task_handle();
}
int main(void) {
    button_task_init(0, short0, long0);
    button_task_init(1, short1, 0);
    set(0, true);
    assert(short_count[0] == 0);
    now += 100000;
    set(0, false);
    assert(short_count[0] == 1 && long_count == 0);
    set(0, true);
    now += 1000000;
    button_task_handle();
    assert(long_count == 1 && short_count[0] == 1);
    set(1, true);
    assert(short_count[1] == 1);
    set(1, false);
    set(1, true);
    assert(short_count[1] == 2);
    now += 2000000;
    button_task_handle();
    assert(long_count == 1);
    set(0, false);
    assert(short_count[0] == 1);
    assert(button_task_get_press_count(0) == 2 && button_task_get_press_count(1) == 2);
    puts("Independent buttons and long press passed");
}
