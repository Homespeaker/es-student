#pragma once
#include <stdbool.h>
#define COMMAND_MAX_ARGS 4
typedef struct {
    const char *name;
    unsigned argc;
    const char *argv[COMMAND_MAX_ARGS];
    bool truncated;
} command_t;
void stdio_text_protocol_init(void);
const command_t *stdio_text_protocol_handle(void);
