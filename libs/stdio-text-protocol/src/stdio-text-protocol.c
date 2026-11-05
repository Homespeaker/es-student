#include "stdio-text-protocol/stdio-text-protocol.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
static char buffer[64];
static unsigned used;
static bool overflow, ready;
static command_t command;
void stdio_text_protocol_init(void) {
    used = 0;
    overflow = false;
    ready = false;
}
const command_t *stdio_text_protocol_handle(void) {
    if (ready) {
        stdio_text_protocol_init();
    }
    int c = getchar_timeout_us(0);
    if (c == PICO_ERROR_TIMEOUT)
        return NULL;
    if (c == 8 || c == 127) {
        if (used) {
            --used;
            overflow = false;
            printf("\b \b");
        }
        return NULL;
    }
    if (c != '\r' && c != '\n') {
        if (c >= 32 && c <= 126) {
            if (used < sizeof(buffer) - 1) {
                buffer[used++] = (char)c;
                putchar(c);
            } else
                overflow = true;
        }
        return NULL;
    }
    if (!used && !overflow)
        return NULL;
    printf("\n");
    buffer[used] = 0;
    memset(&command, 0, sizeof(command));
    command.truncated = overflow;
    char *word = strtok(buffer, " ");
    command.name = word;
    while ((word = strtok(NULL, " "))) {
        if (command.argc == COMMAND_MAX_ARGS) {
            command.truncated = true;
            break;
        }
        command.argv[command.argc++] = word;
    }
    ready = true;
    return command.name || command.truncated ? &command : NULL;
}
