#include "device/device.h"
#include "pico/unique_id.h"
#include "pico/version.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"
#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif
void device_get_info(device_info_t *info) {
    pico_get_unique_board_id_string(info->serial, sizeof(info->serial));
    uint32_t id = *(volatile uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    info->board = DEVICE_BOARD;
    info->sdk_version = PICO_SDK_VERSION_STRING;
    info->manufacturer =
        (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    info->part = (id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    info->revision = (id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
}
