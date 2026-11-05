#include "led.h"
#include "pico/stdlib.h"
static const unsigned LED_PIN = 25;
static bool state;
void led_init(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    led_set(false);
}
void led_set(bool on) {
    state = on;
    gpio_put(LED_PIN, state);
}
void led_toggle(void) {
    led_set(!state);
}
bool led_is_on(void) {
    return state;
}
unsigned led_pin(void) {
    return LED_PIN;
}
