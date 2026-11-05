#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef enum { TELEMETRY_STATE_IDLE, TELEMETRY_STATE_RUN } telemetry_state_t;
typedef struct {
    uint64_t time_us;
    float voltage_v, temp_c;
} telemetry_record_t;
typedef void (*telemetry_task_callback_t)(const telemetry_record_t *record);
void telemetry_task_init(telemetry_task_callback_t callback);
void telemetry_task_handle(void);
bool telemetry_task_start(uint32_t period_ms);
void telemetry_task_stop(void);
telemetry_state_t telemetry_task_get_state(void);
uint32_t telemetry_task_get_period_ms(void);
