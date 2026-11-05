#pragma once
#include <stddef.h>
#define PICO_ERROR_TIMEOUT (-1)
int getchar_timeout_us(unsigned timeout);
