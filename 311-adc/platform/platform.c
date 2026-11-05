#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
static const unsigned LED_PIN = 25, BUTTON_PIN = 15;
void platform_init(void) {
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    adc_init();
    adc_gpio_init(26);
    adc_set_temp_sensor_enabled(true);
    gpio_init(22);
    gpio_set_dir(22, GPIO_OUT);
    gpio_put(22, false);
}
void platform_led_set(bool on) {
    gpio_put(LED_PIN, on);
}
bool platform_button_read(void) {
    return !gpio_get(BUTTON_PIN);
}
bool platform_sampling_busy(void) {
    return false;
}
float platform_voltage(void) {
    adc_select_input(0);
    return adc_read() * (3.3f / 4096);
}
float platform_temperature(void) {
    adc_select_input(4);
    float v = adc_read() * (3.3f / 4096);
    return 27 - (v - 0.706f) / 0.001721f;
}
void platform_charge_set(bool on) {
    gpio_put(22, on);
}
