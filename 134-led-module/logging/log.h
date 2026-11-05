#pragma once
#include <stdio.h>
#define LOG_LEVEL_ERR 1
#define LOG_LEVEL_INF 2
#define LOG_LEVEL_DBG 3
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif
void log_prefix(const char *level, const char *function, unsigned line);
#define LOG_MSG(level, text, ...)                                                                  \
    do {                                                                                           \
        if (LOG_LEVEL >= level) {                                                                  \
            log_prefix(text, __func__, __LINE__);                                                  \
            printf(__VA_ARGS__);                                                                   \
        }                                                                                          \
    } while (0)
#define LOG_ERR(...) LOG_MSG(LOG_LEVEL_ERR, "err", __VA_ARGS__)
#define LOG_INF(...) LOG_MSG(LOG_LEVEL_INF, "inf", __VA_ARGS__)
#define LOG_DBG(...) LOG_MSG(LOG_LEVEL_DBG, "dbg", __VA_ARGS__)
void log_version(void);
