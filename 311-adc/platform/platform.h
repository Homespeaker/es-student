#pragma once
#include <stdbool.h>
#include <stdint.h>
void platform_init(void);
void platform_led_set(bool on);
bool platform_button_read(void);
bool platform_sampling_busy(void);
float platform_voltage(void);
float platform_temperature(void);
void platform_charge_set(bool on);
