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
typedef struct {
    uint32_t count, period_us, min_interval_us, max_interval_us;
    float handler_percent;
    const uint16_t *samples;
} platform_capture_t;
bool platform_timer_blink_start(uint32_t period_ms);
uint32_t platform_timer_blink_stop(void);
bool platform_capture_start(uint32_t count, uint32_t period_us);
bool platform_capture_get(platform_capture_t *capture);
bool platform_block_start(uint32_t count);
bool platform_block_take(const uint16_t **samples, uint32_t *count, uint32_t *elapsed_us);
bool platform_stream_start(unsigned bits);
bool platform_stream_take(void *data, uint16_t *sequence, uint16_t *count);
void platform_stream_stop(void);
uint32_t platform_stream_overruns(void);
void platform_write_raw(const void *data, unsigned bytes);
bool platform_tone_set(uint32_t hz);
void platform_watchdog_enable(uint32_t timeout_ms);
void platform_watchdog_update(void);
bool platform_reset_watchdog(void);
uint32_t platform_reset_count(void);
void platform_hang(void);
bool platform_is_hung(void);
