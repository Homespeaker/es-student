#include "api/api-commands.h"
#include "firmware.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "device/device.h"
#include "systime/systime.h"
#include "profiling/profiling.h"
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
static bool number(const char *s, uint32_t *n) {
    if (!s || !*s)
        return false;
    for (const char *p = s; *p; ++p)
        if (*p < '0' || *p > '9')
            return false;
    errno = 0;
    char *end;
    unsigned long value = strtoul(s, &end, 10);
    if (errno == ERANGE || *end || value > UINT32_MAX)
        return false;
    *n = (uint32_t)value;
    return true;
}
static void info(const command_t *c) {
    (void)c;
    device_info_t d;
    device_get_info(&d);
    printf("name: %s\nversion: %s\nproject: %s\nrepo: %s\nboard: %s\nserial: %s\n", FIRMWARE_NAME,
           FIRMWARE_VERSION, FIRMWARE_PROJECT, FIRMWARE_REPO, d.board, d.serial);
    printf("chip: manufacturer 0x%03lx, part 0x%04lx, revision %lu\npico-sdk: %s\n",
           (unsigned long)d.manufacturer, (unsigned long)d.part, (unsigned long)d.revision,
           d.sdk_version);
}
static void uptime(const command_t *c) {
    (void)c;
    printf("uptime: %llu ms\n", (unsigned long long)systime_ms());
}
static void led_report(void) {
    static const char *states[] = {"off", "on", "blink"};
    printf("led: %s, period %lu ms\n", states[led_task_get_state()],
           (unsigned long)led_task_get_period_ms());
}
static void led_enable(const command_t *c) {
    (void)c;
    led_task_set_state(LED_STATE_ON);
    led_report();
}
static void led_disable(const command_t *c) {
    (void)c;
    led_task_set_state(LED_STATE_OFF);
    led_report();
}
static void led_blink(const command_t *c) {
    (void)c;
    led_task_set_state(LED_STATE_BLINK);
    led_report();
}
static void led_period(const command_t *c) {
    uint32_t n;
    if (c->argc != 1 || !number(c->argv[0], &n)) {
        printf("error: usage led_period <period_ms>\n");
        return;
    }
    if (!n) {
        printf("error: period_ms must be greater than 0\n");
        return;
    }
    led_task_set_period_ms(n);
    led_report();
}
static void button(const command_t *c) {
    (void)c;
    printf("button: %s, presses %lu\n", button_task_is_pressed() ? "pressed" : "released",
           (unsigned long)button_task_get_press_count());
}
static void pi_start(const command_t *c) {
    uint32_t n = 1000000;
    if (c->argc > 1 || (c->argc && !number(c->argv[0], &n))) {
        printf("error: usage pi_start [<terms>]\n");
        return;
    }
    if (!n) {
        printf("error: terms must be greater than 0\n");
        return;
    }
    if (!pi_task_start(n))
        printf("error: pi is busy\n");
    else
        printf("pi: started, %lu terms\n", (unsigned long)n);
}
static void pi(const command_t *c) {
    (void)c;
    pi_result_t r;
    pi_task_get_result(&r);
    if (!r.started)
        printf("pi: not started\n");
    else if (r.running)
        printf("pi: running\n");
    else
        printf("pi: %.9f in %llu ms\n", r.value, (unsigned long long)r.elapsed_ms);
}
static void profiling(const command_t *c) {
    (void)c;
#if PROFILING_ENABLED
    for (unsigned i = 0; i < 5; ++i) {
        profiling_result_t r;
        profiling_get(i, &r);
        printf("%-10s count %llu mean %.2f us max %lu us\n", r.name ? r.name : "unused",
               (unsigned long long)r.count, r.count ? (double)r.total_us / r.count : 0,
               (unsigned long)r.max_us);
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}
static void profiling_reset_command(const command_t *c) {
    (void)c;
    profiling_reset();
    printf("profiling reset\n");
}
const api_command_t api_commands[] = {
    {"info", "device passport", info},
    {"uptime", "time since reset", uptime},
    {"led_enable", "LED on", led_enable},
    {"led_disable", "LED off", led_disable},
    {"led_blink", "LED blink", led_blink},
    {"led_period", "set period in ms", led_period},
    {"button", "button state and press count", button},
    {"pi_start", "start calculation", pi_start},
    {"pi", "calculation result", pi},
    {"profiling", "stopwatch results", profiling},
    {"profiling_reset", "reset stopwatches", profiling_reset_command}};
const size_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);
