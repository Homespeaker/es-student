#include "clock.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/regs/clocks.h"
#include "log.h"
#define CLK_SYS_LOW_KHZ 62500
void clk_info(void) {
    static const char *names[] = {"clk_ref", "clk_sys", "clk_peri", "clk_usb", "clk_adc"};
    const clock_handle_t indexes[] = {clk_ref, clk_sys, clk_peri, clk_usb, clk_adc};
    const unsigned sources[] = {CLOCKS_FC0_SRC_VALUE_CLK_REF, CLOCKS_FC0_SRC_VALUE_CLK_SYS,
                                CLOCKS_FC0_SRC_VALUE_CLK_PERI, CLOCKS_FC0_SRC_VALUE_CLK_USB,
                                CLOCKS_FC0_SRC_VALUE_CLK_ADC};
    for (unsigned i = 0; i < 5; ++i)
        printf("%-10s %lu %lu\n", names[i], (unsigned long)(clock_get_hz(indexes[i]) / 1000),
               (unsigned long)frequency_count_khz(sources[i]));
    printf("rosc %lu\n", (unsigned long)frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC));
}
void uptime(void) {
    printf("uptime: %llu ms\n", (unsigned long long)(time_us_64() / 1000));
}
/* set_sys_clock_pll() configures clk_peri from PLL_USB (48 MHz) unless
 * PICO_CLOCK_ADJUST_PERI_CLOCK_WITH_SYS_CLOCK is enabled in hardware_clocks. */
void clk_sys_low(void) {
    if (!set_sys_clock_khz(CLK_SYS_LOW_KHZ, false))
        LOG_ERR("cannot set system clock\n");
    else
        clk_info();
}
void clk_sys_default(void) {
    if (!set_sys_clock_khz(SYS_CLK_KHZ, false))
        LOG_ERR("cannot set system clock\n");
    else
        clk_info();
}
