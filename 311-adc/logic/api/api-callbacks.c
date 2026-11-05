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
#include "platform/platform.h"
#include "telemetry-task/telemetry-task.h"
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
void api_print_telemetry(const telemetry_record_t *r) {
    printf("tm: %llu %.3f %.1f\n", (unsigned long long)r->time_us, r->voltage_v, r->temp_c);
}
static void get_voltage(const command_t *c) {
    (void)c;
    if (platform_sampling_busy())
        printf("error: adc busy\n");
    else
        printf("voltage: %.3f V\n", platform_voltage());
}
static void get_temp(const command_t *c) {
    (void)c;
    if (platform_sampling_busy())
        printf("error: adc busy\n");
    else
        printf("temp: %.1f C\n", platform_temperature());
}
static void tm_start(const command_t *c) {
    uint32_t n = 100;
    if (c->argc > 1 || (c->argc && !number(c->argv[0], &n))) {
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }
    if (platform_sampling_busy()) {
        printf("error: adc busy\n");
        return;
    }
    if (!telemetry_task_start(n))
        printf("error: period_ms must be at least 10\n");
    else
        printf("telemetry: started, period %lu ms\n", (unsigned long)n);
}
static void tm_stop(const command_t *c) {
    (void)c;
    telemetry_task_stop();
    printf("telemetry: stopped\n");
}
static void charge_on(const command_t *c) {
    (void)c;
    platform_charge_set(true);
    printf("charge: on\n");
}
static void charge_off(const command_t *c) {
    (void)c;
    platform_charge_set(false);
    printf("charge: off\n");
}
const api_command_t api_commands[] = {
    {"get_voltage", "GP26 voltage", get_voltage},
    {"get_temp", "die temperature", get_temp},
    {"tm_start", "start telemetry", tm_start},
    {"tm_stop", "stop telemetry", tm_stop},
    {"charge_on", "charge RC circuit", charge_on},
    {"charge_off", "discharge RC circuit", charge_off},
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
