#include "pico/stdlib.h"
#include "led.h"
#include "device.h"
#include "log.h"
static const uint BUTTON_PIN = 15, DEBOUNCE_MS = 20;
static bool get_button_debounce(uint pin) {
    bool v = gpio_get(pin);
    sleep_ms(DEBOUNCE_MS);
    return v && gpio_get(pin);
}
static void report_led(void) {
    LOG_INF("led %s\n", led_is_on() ? "on" : "off");
}
static void handle_command(int c) {
    if (c == 'e') {
        led_set(true);
        report_led();
    } else if (c == 'd') {
        led_set(false);
        report_led();
    } else if (c == 'v')
        log_version();
    else if (c == 'i')
        device_info();
    else
        LOG_ERR("unknown command: %c\n", c);
}
int main(void) {
    stdio_init_all();
    led_init();
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    bool previous = true;
    while (1) {
        bool current = get_button_debounce(BUTTON_PIN);
        if (previous && !current) {
            led_toggle();
            report_led();
        }
        previous = current;
        int command = getchar_timeout_us(0);
        if (command != PICO_ERROR_TIMEOUT) {
            LOG_DBG("got %c\n", command);
            handle_command(command);
        }
    }
}
