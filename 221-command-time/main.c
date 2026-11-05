#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "command.h"
#include "led.h"
#include "device.h"
#include "log.h"
#include "memory.h"
#include "clock.h"
#include "profiling.h"
/* Software double: about 600 cycles/term, roughly 4.8 s at 125 MHz. */
static const unsigned CALC_PI_TERMS = 1000000;
static const uint64_t BLINK_HALF_PERIOD_US = 500000;
static volatile double pi_result;
static double calc_pi(void) {
    double sum = 0;
    for (unsigned i = 0; i < CALC_PI_TERMS; ++i)
        sum += (i & 1 ? -1.0 : 1.0) / (2.0 * i + 1.0);
    return 4 * sum;
}
static void cmd_calc_pi(void) {
    uint64_t start = time_us_64();
    pi_result = calc_pi();
    uint64_t elapsed = time_us_64() - start;
    printf("pi: %.8f\ntime: %llu ms\n", pi_result, (unsigned long long)(elapsed / 1000));
}
static void cmd_ping(void) {
    printf("pong\n");
}
static void blink(void) {
    static uint64_t last;
    uint64_t now = time_us_64();
    if (now - last >= BLINK_HALF_PERIOD_US) {
        last += BLINK_HALF_PERIOD_US;
        if (now - last >= BLINK_HALF_PERIOD_US)
            last = now;
        led_toggle();
    }
}
const struct command_t commands[] = {{"info", device_info},
                                     {"version", log_version},
                                     {"ping", cmd_ping},
                                     {"mem_info", mem_info},
                                     {"fw_info", fw_info},
                                     {"dev_info", dev_info},
                                     {"boot_info", boot_info},
                                     {"clk_info", clk_info},
                                     {"uptime", uptime},
                                     {"calc_pi", cmd_calc_pi},
                                     {"main_time_exec", main_time_exec},
                                     {"main_time_reset", main_time_reset},
                                     {"clk_sys_low", clk_sys_low},
                                     {"clk_sys_default", clk_sys_default}};
const uint command_count = sizeof(commands) / sizeof(commands[0]);
static void handle_command(const char *command) {
    for (unsigned i = 0; i < command_count; ++i) {
        if (!strcmp(command, commands[i].name)) {
            if (commands[i].handler)
                commands[i].handler();
            return;
        }
    }
    LOG_ERR("unknown command: %s\n", command);
}
int main(void) {
    stdio_init_all();
    led_init();
    profiling_init();
    char line[64];
    unsigned line_length = 0;
    bool overflow = false;
    while (1) {
        blink();
        int c = getchar_timeout_us(0);
        if (c == '\r' || c == '\n') {
            if (line_length || overflow) {
                putchar('\n');
                line[line_length] = '\0';
                if (overflow)
                    printf("error: command too long\n");
                else
                    handle_command(line);
            }
            line_length = 0;
            overflow = false;
        } else if (c == 8 || c == 127) {
            if (line_length) {
                --line_length;
                overflow = false;
                printf("\b \b");
            }
        } else if (c >= 32 && c <= 126) {
            if (line_length < sizeof(line) - 1) {
                line[line_length++] = c;
                putchar(c);
            } else
                overflow = true;
        }
        profiling_iteration();
    }
}
