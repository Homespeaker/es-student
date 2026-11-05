#include "device.h"
#include <stddef.h>
#include <stdio.h>
#include "pico/unique_id.h"
#include "pico/version.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"
struct info_t device_card = {.version = 0x00010000, .name = "es-cmd-usb", .revision = 0};
static char serial[17];
static uint32_t manufacturer, part;
static void read_card(void) {
    pico_get_unique_board_id_string(serial, sizeof(serial));
    uint32_t id = *(volatile uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    manufacturer = (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    part = (id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    device_card.revision = (id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
}
void device_info(void) {
    read_card();
    printf("project: %s\nrepo: %s\nboard: %s\nserial: %s\n", DEVICE_PROJECT, DEVICE_REPO,
           DEVICE_BOARD, serial);
    printf("chip: manufacturer 0x%03lx, part 0x%04lx, revision %u\n", (unsigned long)manufacturer,
           (unsigned long)part, device_card.revision);
#ifdef EXTENDED_DEVICE
    printf("pico-sdk: %s\n", PICO_SDK_VERSION_STRING);
#endif
}
void dev_info(void) {
    read_card();
    printf("struct          address     size offset value\n");
    printf("device_card     0x%08x %5u\n", (unsigned)(uintptr_t)&device_card,
           (unsigned)sizeof(device_card));
    printf("- version       0x%08x %5u %6u 0x%08lx\n", (unsigned)(uintptr_t)&device_card.version,
           (unsigned)sizeof(device_card.version), (unsigned)offsetof(struct info_t, version),
           (unsigned long)device_card.version);
    printf("- name          0x%08x %5u %6u %s\n", (unsigned)(uintptr_t)&device_card.name,
           (unsigned)sizeof(device_card.name), (unsigned)offsetof(struct info_t, name),
           device_card.name);
    printf("- revision      0x%08x %5u %6u %u\n", (unsigned)(uintptr_t)&device_card.revision,
           (unsigned)sizeof(device_card.revision), (unsigned)offsetof(struct info_t, revision),
           device_card.revision);
    unsigned fields =
        sizeof(device_card.version) + sizeof(device_card.name) + sizeof(device_card.revision);
    printf("fields %u, sizeof %u, padding %u\n", fields, (unsigned)sizeof(device_card),
           (unsigned)sizeof(device_card) - fields);
}
