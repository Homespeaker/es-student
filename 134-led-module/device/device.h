#pragma once
#include <stdint.h>
#define DEVICE_NAME "es-led-module"
#define FIRMWARE_VERSION "1.0.0"
#ifndef DEVICE_PROJECT
#define DEVICE_PROJECT "134-led-module"
#endif
#define DEVICE_REPO "https://github.com/Homespeaker/es-student"
#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif
struct info_t {
    uint32_t manufacturer, part, revision;
    char serial[17];
    uint8_t version;
};
extern struct info_t device_card;
void device_info(void);
void dev_info(void);
