#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "command.h"
#include "led.h"
#include "device.h"
#include "log.h"
#include "memory.h"
static void cmd_enable(void) {
    led_set(true);
    printf("led on\n");
}
static void cmd_disable(void) {
    led_set(false);
    printf("led off\n");
}
static void cmd_ping(void) {
    printf("pong\n");
}
static void cmd_info(void) {
    device_info();
}
static void cmd_version(void) {
    log_version();
}
static void cmd_memory(void) {
    mem_info();
}
static void cmd_firmware(void) {
    fw_info();
}
static void cmd_device(void) {
    dev_info();
}
static void cmd_boot(void) {
    boot_info();
}
const struct command_t commands[] = {
    {"enable", cmd_enable},    {"disable", cmd_disable}, {"info", cmd_info},
    {"version", cmd_version},  {"ping", cmd_ping},       {"mem_info", cmd_memory},
    {"fw_info", cmd_firmware}, {"dev_info", cmd_device}, {"boot_info", cmd_boot}};
const uint command_count = sizeof(commands) / sizeof(commands[0]);
static void handle_command(const char *command) {
    for (unsigned i = 0; i < command_count; ++i) {
        if (!strcmp(command, commands[i].name)) {
            if (commands[i].handler != NULL)
                commands[i].handler();
            return;
        }
    }
    LOG_ERR("unknown command: %s\n", command);
}
int main(void) {
    stdio_init_all();
    led_init();
    gpio_init(15);
    gpio_set_dir(15, GPIO_IN);
    gpio_pull_up(15);
    char line[64];
    unsigned line_length = 0;
    bool overflow = false, previous = true;
    while (1) {
        bool current = gpio_get(15);
        sleep_ms(20);
        current = current && gpio_get(15);
        if (previous && !current) {
            led_toggle();
            printf("led %s\n", led_is_on() ? "on" : "off");
        }
        previous = current;
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT)
            continue;
        if (c == '\r' || c == '\n') {
            if (line_length || overflow) {
                printf("\n");
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
    }
}
