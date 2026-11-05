#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "api/api.h"
#include "led-task/led-task.h"
#include "pi-task/pi-task.h"
#include "device/device.h"
uint64_t time_us_64(void) { return 1000000; }
void platform_led_set(bool on) { (void)on; }
bool button_task_is_pressed(void) { return false; }
uint32_t button_task_get_press_count(void) { return 0; }
void device_get_info(device_info_t *info) {
    *info = (device_info_t){.board = "test", .sdk_version = "test"};
}
static void call(const char *name, unsigned argc, const char *arg) {
    command_t command = {.name = name, .argc = argc, .argv = {arg}};
    api_handle(&command);
}
int main(void) {
    led_task_init(); pi_task_init();
    const char *invalid[] = {"-1", "+1", "1x", "0x10", "4294967296", "999999999999999999999", ""};
    for (unsigned i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        call("led_period", 1, invalid[i]);
        assert(led_task_get_period_ms() == 1000);
    }
    call("led_period", 1, "0");
    assert(led_task_get_period_ms() == 1000);
    call("led_period", 0, NULL);
    call("led_period", 1, "4294967295");
    assert(led_task_get_period_ms() == UINT32_MAX);
    call("pi", 0, NULL);
    call("pi_start", 1, "0");
    call("pi_start", 1, "1");
    call("pi_start", 1, "1");
    call("pi", 0, NULL);
    pi_task_handle();
    call("pi", 0, NULL);
    call("profiling", 0, NULL);
    call("not_a_command", 0, NULL);
    command_t overflow = {.truncated = true};
    api_handle(&overflow);
    puts("API tests passed");
}
