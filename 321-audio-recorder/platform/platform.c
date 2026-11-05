#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include "hardware/sync.h"
#include <string.h>
typedef enum { ADC_IDLE, ADC_TIMER, ADC_BLOCK, ADC_STREAM } adc_mode_t;
static volatile adc_mode_t adc_mode;
static struct repeating_timer blink_timer, capture_timer;
static bool blink_active, watchdog_active, hung, watchdog_reset;
static volatile bool indicator;
static volatile uint32_t toggles;
static uint32_t restarts;
static uint16_t capture_samples[2000], block_samples[8192];
static uint64_t capture_times[2000];
static volatile uint32_t capture_count;
static uint32_t capture_target, capture_period;
static volatile uint64_t handler_total_us;
static uint64_t block_start_us;
static volatile uint32_t block_elapsed_us;
static uint32_t block_count;
static volatile bool block_ready;
static int dma_channels[2];
static uint16_t stream_buffers[2][512];
static volatile unsigned ready_mask;
static volatile uint32_t overruns;
static volatile uint16_t buffer_sequence[2];
static uint16_t next_sequence;
static unsigned stream_bits;
static bool blink_tick(struct repeating_timer *timer) {
    (void)timer;
    indicator = !indicator;
    gpio_put(25, indicator);
    ++toggles;
    return true;
}
static bool capture_tick(struct repeating_timer *timer) {
    (void)timer;
    uint64_t start = time_us_64();
    uint32_t i = capture_count;
    capture_times[i] = start;
    capture_samples[i] = adc_read();
    handler_total_us += time_us_64() - start;
    capture_count = i + 1;
    if (capture_count == capture_target) {
        adc_mode = ADC_IDLE;
        return false;
    }
    return true;
}
static void dma_irq_handler(void) {
    uint32_t pending = dma_hw->ints0;
    dma_hw->ints0 = pending & ((1u << dma_channels[0]) | (1u << dma_channels[1]));
    if (adc_mode == ADC_BLOCK && (pending & (1u << dma_channels[0]))) {
        adc_run(false);
        block_elapsed_us = (uint32_t)(time_us_64() - block_start_us);
        block_ready = true;
        adc_mode = ADC_IDLE;
    } else if (adc_mode == ADC_STREAM) {
        for (unsigned i = 0; i < 2; ++i)
            if (pending & (1u << dma_channels[i])) {
                unsigned other = i ^ 1u;
                /* The chained channel now owns its buffer: drop any stale ready copy. */
                if (ready_mask & (1u << other)) {
                    ready_mask &= ~(1u << other);
                    ++overruns;
                }
                if (ready_mask & (1u << i))
                    ++overruns;
                buffer_sequence[i] = next_sequence++;
                ready_mask |= 1u << i;
                dma_channel_set_write_addr(dma_channels[i], stream_buffers[i], false);
                dma_channel_set_trans_count(dma_channels[i], stream_bits == 8 ? 1024 : 512, false);
            }
    }
}
static void prepare_adc(unsigned bits) {
    adc_run(false);
    adc_select_input(0);
    adc_fifo_drain();
    adc_fifo_setup(true, true, 1, false, bits == 8);
    adc_set_clkdiv(48000000.0f / 44100.0f - 1.0f);
}
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
    dma_channels[0] = dma_claim_unused_channel(true);
    dma_channels[1] = dma_claim_unused_channel(true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    gpio_set_function(20, GPIO_FUNC_PWM);
    unsigned slice = pwm_gpio_to_slice_num(20);
    pwm_set_clkdiv(slice, (float)clock_get_hz(clk_sys) / 1000000.0f);
    pwm_set_gpio_level(20, 0);
    watchdog_reset = watchdog_enable_caused_reboot();
    restarts =
        watchdog_reset && watchdog_hw->scratch[0] == 0x45535243 ? watchdog_hw->scratch[1] + 1 : 0;
    watchdog_hw->scratch[0] = 0x45535243;
    watchdog_hw->scratch[1] = restarts;
}
void platform_led_set(bool on) {
    if (!blink_active)
        gpio_put(LED_PIN, on);
}
bool platform_button_read(void) {
    return !gpio_get(BUTTON_PIN);
}
bool platform_sampling_busy(void) {
    return adc_mode != ADC_IDLE;
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
bool platform_timer_blink_start(uint32_t ms) {
    if (ms < 10 || ms > 60000)
        return false;
    if (blink_active)
        cancel_repeating_timer(&blink_timer);
    toggles = 0;
    indicator = false;
    blink_active = add_repeating_timer_ms(-(int32_t)(ms / 2), blink_tick, NULL, &blink_timer);
    return blink_active;
}
uint32_t platform_timer_blink_stop(void) {
    if (blink_active)
        cancel_repeating_timer(&blink_timer);
    blink_active = false;
    gpio_put(25, false);
    return toggles;
}
bool platform_capture_start(uint32_t n, uint32_t us) {
    if (platform_sampling_busy() || !n || n > 2000 || us < 10 || us > INT32_MAX)
        return false;
    adc_select_input(0);
    adc_set_clkdiv(0);
    adc_fifo_setup(false, false, 1, false, false);
    capture_target = n;
    capture_count = 0;
    capture_period = us;
    handler_total_us = 0;
    adc_mode = ADC_TIMER;
    if (!add_repeating_timer_us(-(int32_t)us, capture_tick, NULL, &capture_timer)) {
        adc_mode = ADC_IDLE;
        return false;
    }
    return true;
}
bool platform_capture_get(platform_capture_t *r) {
    if (adc_mode == ADC_TIMER || !capture_count)
        return false;
    r->count = capture_count;
    r->period_us = capture_period;
    r->samples = capture_samples;
    r->min_interval_us = UINT32_MAX;
    r->max_interval_us = 0;
    for (unsigned i = 1; i < r->count; ++i) {
        uint32_t dt = capture_times[i] - capture_times[i - 1];
        if (dt < r->min_interval_us)
            r->min_interval_us = dt;
        if (dt > r->max_interval_us)
            r->max_interval_us = dt;
    }
    if (r->count < 2)
        r->min_interval_us = 0;
    r->handler_percent = 100.0f * handler_total_us / ((uint64_t)r->count * r->period_us);
    return true;
}
bool platform_block_start(uint32_t n) {
    if (platform_sampling_busy() || !n || n > 8192)
        return false;
    prepare_adc(16);
    block_count = n;
    block_ready = false;
    adc_mode = ADC_BLOCK;
    dma_channel_config cfg = dma_channel_get_default_config(dma_channels[0]);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);
    dma_channel_configure(dma_channels[0], &cfg, block_samples, &adc_hw->fifo, n, false);
    dma_channel_set_irq0_enabled(dma_channels[0], true);
    dma_hw->ints0 = 1u << dma_channels[0];
    block_start_us = time_us_64();
    dma_start_channel_mask(1u << dma_channels[0]);
    adc_run(true);
    return true;
}
bool platform_block_take(const uint16_t **samples, uint32_t *count, uint32_t *elapsed_us) {
    if (!block_ready)
        return false;
    block_ready = false;
    *samples = block_samples;
    *count = block_count;
    *elapsed_us = block_elapsed_us;
    return true;
}
bool platform_stream_start(unsigned bits) {
    if (platform_sampling_busy() || (bits != 8 && bits != 16))
        return false;
    prepare_adc(bits);
    stream_bits = bits;
    ready_mask = 0;
    overruns = 0;
    next_sequence = 0;
    adc_mode = ADC_STREAM;
    for (unsigned i = 0; i < 2; ++i) {
        dma_channel_config cfg = dma_channel_get_default_config(dma_channels[i]);
        channel_config_set_transfer_data_size(&cfg, bits == 8 ? DMA_SIZE_8 : DMA_SIZE_16);
        channel_config_set_read_increment(&cfg, false);
        channel_config_set_write_increment(&cfg, true);
        channel_config_set_dreq(&cfg, DREQ_ADC);
        channel_config_set_chain_to(&cfg, dma_channels[i ^ 1u]);
        dma_channel_configure(dma_channels[i], &cfg, stream_buffers[i], &adc_hw->fifo,
                              bits == 8 ? 1024 : 512, false);
        dma_channel_set_irq0_enabled(dma_channels[i], true);
    }
    dma_hw->ints0 = (1u << dma_channels[0]) | (1u << dma_channels[1]);
    dma_start_channel_mask(1u << dma_channels[0]);
    adc_run(true);
    return true;
}
bool platform_stream_take(void *data, uint16_t *sequence, uint16_t *count) {
    uint32_t saved = save_and_disable_interrupts();
    unsigned mask = ready_mask;
    if (!mask) {
        restore_interrupts(saved);
        return false;
    }
    unsigned i = (mask & 1u) ? 0 : 1;
    if (mask == 3 && (int16_t)(buffer_sequence[1] - buffer_sequence[0]) < 0)
        i = 1;
    if (dma_channel_is_busy(dma_channels[i])) {
        ready_mask &= ~(1u << i);
        ++overruns;
        restore_interrupts(saved);
        return false;
    }
    memcpy(data, stream_buffers[i], 1024);
    *sequence = buffer_sequence[i];
    *count = stream_bits == 8 ? 1024 : 512;
    ready_mask &= ~(1u << i);
    restore_interrupts(saved);
    return true;
}
void platform_stream_stop(void) {
    if (adc_mode == ADC_STREAM) {
        adc_run(false);
        adc_mode = ADC_IDLE;
        for (unsigned i = 0; i < 2; ++i) {
            dma_channel_config cfg = dma_get_channel_config(dma_channels[i]);
            channel_config_set_chain_to(&cfg, dma_channels[i]);
            dma_channel_set_config(dma_channels[i], &cfg, false);
        }
        for (unsigned i = 0; i < 2; ++i) {
            dma_channel_set_irq0_enabled(dma_channels[i], false);
            dma_channel_abort(dma_channels[i]);
        }
        ready_mask = 0;
        adc_fifo_drain();
    }
}
uint32_t platform_stream_overruns(void) {
    return overruns;
}
void platform_write_raw(const void *data, unsigned bytes) {
    stdio_put_string(data, (int)bytes, false, false);
}
bool platform_tone_set(uint32_t hz) {
    if (hz && (hz < 20 || hz > 20000))
        return false;
    unsigned slice = pwm_gpio_to_slice_num(20);
    if (!hz) {
        pwm_set_enabled(slice, false);
        pwm_set_gpio_level(20, 0);
        return true;
    }
    uint32_t top = (1000000u + hz / 2) / hz;
    pwm_set_wrap(slice, top - 1);
    pwm_set_gpio_level(20, top / 2);
    pwm_set_enabled(slice, true);
    return true;
}
void platform_watchdog_enable(uint32_t ms) {
    watchdog_enable(ms, true);
    watchdog_active = true;
}
void platform_watchdog_update(void) {
    if (watchdog_active)
        watchdog_update();
}
bool platform_reset_watchdog(void) {
    return watchdog_reset;
}
uint32_t platform_reset_count(void) {
    return restarts;
}
void platform_hang(void) {
    hung = true;
}
bool platform_is_hung(void) {
    return hung;
}
