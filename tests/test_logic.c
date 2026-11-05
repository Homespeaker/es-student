#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "systime/systime.h"
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
static uint64_t now;
static bool input, light;
static unsigned presses;
static int incoming = -1;
uint64_t time_us_64(void) {
    return now;
}
int getchar_timeout_us(unsigned timeout) {
    (void)timeout;
    int c = incoming;
    incoming = -1;
    return c;
}
void platform_led_set(bool on) {
    light = on;
}
bool platform_button_read(void) {
    return input;
}
static void pressed(void) {
    ++presses;
}
static const command_t *feed(const char *s) {
    const command_t *r = NULL;
    for (; *s; ++s) {
        incoming = (unsigned char)*s;
        const command_t *c = stdio_text_protocol_handle();
        if (c)
            r = c;
    }
    return r;
}
int main(void) {
    uint64_t last = 100;
    now = 199;
    assert(!systime_period_elapsed(&last, 100));
    now = 205;
    assert(systime_period_elapsed(&last, 100) && last == 200);
    now = 550;
    assert(systime_period_elapsed(&last, 100) && last == 550);
    led_task_init();
    assert(light && led_task_get_state() == LED_STATE_BLINK);
    now += 500000;
    led_task_handle();
    assert(!light);
    led_task_set_period_ms(UINT32_MAX);
    now += 1000000;
    led_task_handle();
    assert(!light);
    button_task_init(pressed);
    input = true;
    button_task_handle();
    now += 19000;
    button_task_handle();
    assert(!presses);
    input = false;
    button_task_handle();
    now += 1000;
    input = true;
    button_task_handle();
    now += 20000;
    button_task_handle();
    assert(presses == 1);
    now += 2000000;
    button_task_handle();
    assert(presses == 1);
    pi_task_init();
    assert(pi_task_start(1000000));
    assert(!pi_task_start(1));
    pi_result_t pi;
    do {
        pi_task_handle();
        pi_task_get_result(&pi);
    } while (pi.running);
    assert(fabs(pi.value - 3.141592653589793) < 1.01e-6);
    assert(pi_task_start(1));
    pi_task_handle();
    pi_task_get_result(&pi);
    assert(pi.value == 4);
    stdio_text_protocol_init();
    const command_t *c = feed("  led_period   500  \r");
    assert(c && !c->truncated && c->argc == 1 && !strcmp(c->name, "led_period") &&
           !strcmp(c->argv[0], "500"));
    assert(!feed("\n  \r"));
    c = feed("pi_starb\bt 10\r");
    assert(c && !strcmp(c->name, "pi_start"));
    c = feed("test 1 2 3 4 5\r");
    assert(c && c->truncated);
    char big[80];
    memset(big, 'x', sizeof(big));
    big[79] = 0;
    feed(big);
    c = feed("\r");
    assert(c && c->truncated);
    feed(big);
    c = feed("\b\r");
    assert(c && !c->truncated && strlen(c->name) == 62);
    puts("Native logic tests passed");
}
