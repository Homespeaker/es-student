#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
const uint LED_PIN = 25, BUTTON_PIN = 15, DEBOUNCE_MS = 20;
static bool led;
bool get_button_debounce(uint pin) {
    bool value = gpio_get(pin);
    sleep_ms(DEBOUNCE_MS);
    return value && gpio_get(pin);
}
void set_led(bool on) {
    led = on;
    gpio_put(LED_PIN, on);
    printf("led %s\n", on ? "on" : "off");
}
void handle_command(int command) {
    if (command == 'e') {
        set_led(true);
        return;
    }
    if (command == 'd') {
        set_led(false);
        return;
    }
    printf("unknown command: %c\n", command);
}
int main(void) {
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    bool previous = true;
    while (1) {
        bool current = get_button_debounce(BUTTON_PIN);
        if (previous && !current)
            set_led(!led);
        previous = current;
        int command = getchar_timeout_us(0);
        if (command != PICO_ERROR_TIMEOUT)
            handle_command(command);
    }
}
