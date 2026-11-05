#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sio.h"
#include "command.h"
#include "device.h"
#include "led.h"
extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;
extern int main(void);
static uint32_t data_variable = 100;
static uint32_t bss_variable;
static void row(const char *name, uintptr_t start, uintptr_t end) {
    printf("%-12s 0x%08x 0x%08x %8u\n", name, (unsigned)start, (unsigned)end,
           (unsigned)(end - start));
}
void mem_info(void) {
    uintptr_t flash = (uintptr_t)&__flash_binary_start, end = (uintptr_t)&__flash_binary_end;
    uintptr_t data = (uintptr_t)&__data_start__, de = (uintptr_t)&__data_end__,
              bss = (uintptr_t)&__bss_start__, be = (uintptr_t)&__bss_end__;
    printf("region       start      end        size\n");
    row("flash", XIP_BASE, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("sram", SRAM_BASE, SRAM_BASE + 264 * 1024);
    row("rom", ROM_BASE, ROM_BASE + 16 * 1024);
    row("image", flash, end);
    row("free", end, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);
    row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);
    row("data flash", (uintptr_t)&__etext, (uintptr_t)&__etext + de - data);
    row("data ram", data, de);
    row("bss", bss, be);
    row("heap", be, (uintptr_t)&__HeapLimit);
    row("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);
    printf("total\nflash image %u = boot2 %u + text %u + data %u\n", (unsigned)(end - flash),
           (unsigned)(&__boot2_end__ - &__boot2_start__), (unsigned)(&__etext - &__boot2_end__),
           (unsigned)(de - data));
    printf("flash free %u of %u\nram used %u\nram free %u\nstack free %u\n",
           (unsigned)(XIP_BASE + PICO_FLASH_SIZE_BYTES - end), PICO_FLASH_SIZE_BYTES,
           (unsigned)(de - data + be - bss), (unsigned)((uintptr_t)&__HeapLimit - be),
           (unsigned)(&__StackTop - &__StackBottom));
}
static void function_row(const char *name, uintptr_t address) {
    printf("%-16s 0x%08x 0x%04x\n", name, (unsigned)address,
           *(const uint16_t *)(address & ~1u));
}
void fw_info(void) {
    int stack_variable = 0;
    void *heap_variable = malloc(16);
    function_row("main", (uintptr_t)main);
    function_row("fw_info", (uintptr_t)fw_info);
    printf("commands 0x%08x count %u\n", (unsigned)(uintptr_t)commands, command_count);
    for (unsigned i = 0; i < command_count; ++i)
        printf("- %-16s 0x%08x\n", commands[i].name, (unsigned)(uintptr_t)commands[i].handler);
    printf("DEVICE_NAME %p %s\nFIRMWARE_VERSION %p %s\n", (void *)DEVICE_NAME, DEVICE_NAME,
           (void *)FIRMWARE_VERSION, FIRMWARE_VERSION);
    printf("DEVICE_PROJECT 0x%08x %s\nDEVICE_BOARD 0x%08x %s\n",
           (unsigned)(uintptr_t)DEVICE_PROJECT, DEVICE_PROJECT, (unsigned)(uintptr_t)DEVICE_BOARD,
           DEVICE_BOARD);
    printf("data_variable 0x%08x %lu\nbss_variable 0x%08x %lu\nstack_variable 0x%08x\n",
           (unsigned)(uintptr_t)&data_variable, (unsigned long)++data_variable,
           (unsigned)(uintptr_t)&bss_variable, (unsigned long)++bss_variable,
           (unsigned)(uintptr_t)&stack_variable);
    if (heap_variable != NULL) {
        printf("heap_variable 0x%08x\n", (unsigned)(uintptr_t)heap_variable);
        free(heap_variable);
    } else
        printf("error: allocation failed\n");
}
void boot_info(void) {
    const uint32_t *vectors = (const uint32_t *)0x10000100;
    volatile uint32_t *gpio_in = (volatile uint32_t *)(SIO_BASE + SIO_GPIO_IN_OFFSET);
    unsigned pin = led_pin();
    printf("vector table 0x10000100\nstack top 0x%08lx\nreset 0x%08lx\nreset (even) 0x%08lx\n",
           (unsigned long)vectors[0], (unsigned long)vectors[1], (unsigned long)(vectors[1] & ~1u));
    printf("gpio in 0x%08x\nled pin %u\nled bit %lu\ngpio_get %d\n", (unsigned)(uintptr_t)gpio_in,
           pin, (unsigned long)((*gpio_in >> pin) & 1u), gpio_get(pin));
}
