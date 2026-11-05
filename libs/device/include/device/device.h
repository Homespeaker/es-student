#pragma once
#include <stdint.h>
typedef struct {
    char serial[17];
    const char *board;
    const char *sdk_version;
    uint32_t manufacturer, part, revision;
} device_info_t;
void device_get_info(device_info_t *info);
