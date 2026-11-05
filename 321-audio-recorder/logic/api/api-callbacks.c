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
#include "block-task/block-task.h"
#include "stream-task/stream-task.h"
#include <math.h>
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
void api_send_frame(const void *frame, unsigned bytes) {
    platform_write_raw(frame, bytes);
}
static bool one_number(const command_t *c, uint32_t *value, const char *syntax) {
    if (c->argc != 1 || !number(c->argv[0], value)) {
        printf("error: usage %s\n", syntax);
        return false;
    }
    return true;
}
static void timer_blink_start(const command_t *c) {
    uint32_t n;
    if (!one_number(c, &n, "timer_blink_start <period_ms>"))
        return;
    if (n < 10 || n > 60000) {
        printf("error: period_ms must be 10..60000\n");
        return;
    }
    if (platform_timer_blink_start(n))
        printf("timer_blink: started, period %lu ms\n", (unsigned long)n);
    else
        printf("error: timer unavailable\n");
}
static void timer_blink_stop(const command_t *c) {
    (void)c;
    printf("timer_blink: stopped, %lu toggles\n", (unsigned long)platform_timer_blink_stop());
}
static void cap_start(const command_t *c) {
    uint32_t n, period;
    if (c->argc != 2 || !number(c->argv[0], &n) || !number(c->argv[1], &period)) {
        printf("error: usage cap_start <N> <period_us>\n");
        return;
    }
    if (!n || n > 2000 || period < 10 || period > INT32_MAX) {
        printf("error: N must be 1..2000, period_us at least 10\n");
        return;
    }
    if (platform_sampling_busy()) {
        printf("error: adc busy\n");
        return;
    }
    telemetry_task_stop();
    if (platform_capture_start(n, period))
        printf("capture: started, %lu samples, period %lu us\n", (unsigned long)n,
               (unsigned long)period);
    else
        printf("error: capture unavailable\n");
}
static void cap_dump(const command_t *c) {
    (void)c;
    platform_capture_t r;
    if (!platform_capture_get(&r)) {
        printf("error: capture not ready\n");
        return;
    }
    printf("capture: %lu samples, period %lu us, interval %lu..%lu us, handler %.1f %%\n",
           (unsigned long)r.count, (unsigned long)r.period_us, (unsigned long)r.min_interval_us,
           (unsigned long)r.max_interval_us, r.handler_percent);
    for (uint32_t i = 0; i < r.count; ++i) {
        if (i % 16 == 0)
            printf("cap:");
        printf(" %u", r.samples[i]);
        if (i % 16 == 15 || i + 1 == r.count)
            printf("\n");
    }
}
static void block_stats(const uint16_t *samples, uint32_t count, uint32_t elapsed) {
    uint16_t low = 4095, high = 0;
    uint64_t sum = 0;
    for (uint32_t i = 0; i < count; ++i) {
        if (samples[i] < low)
            low = samples[i];
        if (samples[i] > high)
            high = samples[i];
        sum += samples[i];
    }
    printf("dma_cap: %lu samples in %lu us, expected %lu us, min %u max %u mean %.1f\n",
           (unsigned long)count, (unsigned long)elapsed,
           (unsigned long)((uint64_t)count * 1000000 / 44100), low, high, (double)sum / count);
}
static void cmd_dma_cap(const command_t *c) {
    uint32_t n;
    if (!one_number(c, &n, "dma_cap <N>"))
        return;
    if (!n || n > 8192) {
        printf("error: N must be 1..8192\n");
        return;
    }
    telemetry_task_stop();
    if (!block_task_start(n, block_stats))
        printf("error: adc busy\n");
}
static void generate_tone(const command_t *c) {
    uint32_t hz;
    if (!one_number(c, &hz, "generate_tone <Hz>"))
        return;
    if (!platform_tone_set(hz)) {
        printf("error: Hz must be 0 or 20..20000\n");
        return;
    }
    if (hz)
        printf("tone: %lu Hz\n", (unsigned long)hz);
    else
        printf("tone: off\n");
}
static void mic_bias_done(const uint16_t *samples, uint32_t count, uint32_t elapsed) {
    (void)elapsed;
    uint64_t sum = 0;
    for (uint32_t i = 0; i < count; ++i)
        sum += samples[i];
    printf("mic: bias %.3f V\n", (double)sum / count * 3.3 / 4096);
}
static void mic_level_done(const uint16_t *samples, uint32_t count, uint32_t elapsed) {
    (void)elapsed;
    double sum = 0, squares = 0, peak = 0;
    for (uint32_t i = 0; i < count; ++i)
        sum += samples[i];
    double mean = sum / count;
    for (uint32_t i = 0; i < count; ++i) {
        double d = samples[i] - mean;
        squares += d * d;
        if (fabs(d) > peak)
            peak = fabs(d);
    }
    printf("mic: level %.1f mV rms, peak %.1f mV, %lu ms\n", sqrt(squares / count) * 3300 / 4096,
           peak * 3300 / 4096, (unsigned long)((uint64_t)count * 1000 / 44100));
}
static void mic_bias(const command_t *c) {
    (void)c;
    telemetry_task_stop();
    if (!block_task_start(4410, mic_bias_done))
        printf("error: adc busy\n");
}
static void mic_level(const command_t *c) {
    uint32_t ms = 100;
    if (c->argc > 1 || (c->argc && !number(c->argv[0], &ms))) {
        printf("error: usage mic_level [<ms>]\n");
        return;
    }
    if (!ms || ms > 185) {
        printf("error: ms must be 1..185\n");
        return;
    }
    telemetry_task_stop();
    if (!block_task_start(ms * 44100 / 1000, mic_level_done))
        printf("error: adc busy\n");
}
static void stream_start(const command_t *c) {
    uint32_t bits = 16;
    if (c->argc > 1 || (c->argc && !number(c->argv[0], &bits)) || (bits != 8 && bits != 16)) {
        printf("error: usage stream_start [8|16]\n");
        return;
    }
    telemetry_task_stop();
    if (!stream_task_start(bits)) {
        printf("error: adc busy\n");
        return;
    }
    printf("stream: started, 44100 Hz, %lu bit\n", (unsigned long)bits);
}
static void stream_stop(const command_t *c) {
    (void)c;
    stream_result_t r = stream_task_stop();
    printf("stream: stopped, %lu frames, %lu samples, %lu overruns\n", (unsigned long)r.frames,
           (unsigned long)r.samples, (unsigned long)r.overruns);
}
static void wdt_enable(const command_t *c) {
    uint32_t ms;
    if (!one_number(c, &ms, "wdt_enable <timeout_ms>"))
        return;
    if (!ms || ms > 8388) {
        printf("error: timeout_ms must be 1..8388\n");
        return;
    }
    printf("wdt: enabled, timeout %lu ms\n", (unsigned long)ms);
    platform_watchdog_enable(ms);
}
static void hang(const command_t *c) {
    (void)c;
    printf("hang: superloop stopped\n");
    platform_hang();
}
static void reset_reason(const command_t *c) {
    (void)c;
    printf("reset: %s, restarts %lu\n", platform_reset_watchdog() ? "watchdog" : "other",
           (unsigned long)platform_reset_count());
}
const api_command_t api_commands[] = {
    {"timer_blink_start", "start interrupt LED", timer_blink_start},
    {"timer_blink_stop", "stop interrupt LED", timer_blink_stop},
    {"cap_start", "capture samples using timer", cap_start},
    {"cap_dump", "print timer capture", cap_dump},
    {"dma_cap", "capture block using DMA", cmd_dma_cap},
    {"generate_tone", "PWM test tone", generate_tone},
    {"mic_bias", "microphone bias", mic_bias},
    {"mic_level", "microphone RMS and peak", mic_level},
    {"stream_start", "start binary stream", stream_start},
    {"stream_stop", "stop binary stream", stream_stop},
    {"wdt_enable", "enable watchdog", wdt_enable},
    {"hang", "stop superloop", hang},
    {"reset_reason", "last reset", reset_reason},
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
