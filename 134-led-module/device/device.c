#include "device.h"
#include <stddef.h>
#include <stdio.h>
#include "pico/unique_id.h"
#include "pico/version.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"
struct info_t device_card;
static void read_card(void) {
    pico_get_unique_board_id_string(device_card.serial, sizeof(device_card.serial));
    uint32_t id = *(volatile uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    device_card.manufacturer =
        (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    device_card.part = (id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    device_card.revision = (id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
    device_card.version = 1;
}
void device_info(void) {
    read_card();
    printf("project: %s\nrepo: %s\nboard: %s\nserial: %s\n", DEVICE_PROJECT, DEVICE_REPO,
           DEVICE_BOARD, device_card.serial);
    printf("chip: manufacturer 0x%03lx, part 0x%04lx, revision %lu\n",
           (unsigned long)device_card.manufacturer, (unsigned long)device_card.part,
           (unsigned long)device_card.revision);
#ifdef EXTENDED_DEVICE
    printf("pico-sdk: %s\n", PICO_SDK_VERSION_STRING);
#endif
}
void dev_info(void) {
    read_card();
    printf("device_card %p sizeof %u\n", (void *)&device_card, (unsigned)sizeof(device_card));
#define FIELD(f, fmt, value)                                                                       \
    printf(#f " %p size %u offset %u value " fmt "\n", (void *)&device_card.f,                     \
           (unsigned)sizeof(device_card.f), (unsigned)offsetof(struct info_t, f), value)
    FIELD(manufacturer, "%lu", (unsigned long)device_card.manufacturer);
    FIELD(part, "%lu", (unsigned long)device_card.part);
    FIELD(revision, "%lu", (unsigned long)device_card.revision);
    FIELD(serial, "%s", device_card.serial);
    FIELD(version, "%u", device_card.version);
#undef FIELD
    unsigned fields = 3 * sizeof(uint32_t) + sizeof(device_card.serial) + sizeof(uint8_t);
    printf("fields %u sizeof %u padding %u\n", fields, (unsigned)sizeof(device_card),
           (unsigned)sizeof(device_card) - fields);
}
