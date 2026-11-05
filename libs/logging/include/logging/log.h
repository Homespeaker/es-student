#pragma once
#include <stdio.h>
#ifndef LOG_LEVEL
#define LOG_LEVEL 3
#endif
void log_write(const char *level, const char *function, int line, const char *format, ...);
#if LOG_LEVEL >= 1
#define LOG_ERR(...) log_write("err", __func__, __LINE__, __VA_ARGS__)
#else
#define LOG_ERR(...) ((void)0)
#endif
#if LOG_LEVEL >= 2
#define LOG_INF(...) log_write("inf", __func__, __LINE__, __VA_ARGS__)
#else
#define LOG_INF(...) ((void)0)
#endif
#if LOG_LEVEL >= 3
#define LOG_DBG(...) log_write("dbg", __func__, __LINE__, __VA_ARGS__)
#else
#define LOG_DBG(...) ((void)0)
#endif
