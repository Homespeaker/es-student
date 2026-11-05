#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    bool started, running;
    double value;
    uint64_t elapsed_ms;
} pi_result_t;
void pi_task_init(void);
bool pi_task_start(uint32_t terms);
void pi_task_handle(void);
void pi_task_get_result(pi_result_t *result);
