#pragma once
#include <stddef.h>
#include "stdio-text-protocol/stdio-text-protocol.h"
typedef void (*api_callback_t)(const command_t *command);
typedef struct {
    const char *name, *help;
    api_callback_t callback;
} api_command_t;
extern const api_command_t api_commands[];
extern const size_t api_command_count;
