#ifndef __MAIN_H
#define __MAIN_H
#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>
typedef enum {
 STATE_IDLE, STATE_SELF_CHECK, STATE_INFLATE, STATE_SETTLE,
 STATE_DEFLATE_SAMPLE, STATE_CALCULATE, STATE_RESULT_DISPLAY, STATE_ERROR
} SystemState_t;
typedef struct {
 float sbp, dbp, map;
 uint16_t hr;
 uint8_t err_code;
 bool valid;
} BP_Result_t;
enum {
 BP_OK=0, BP_ERR_PRESSURE=1, BP_ERR_INFLATE=2, BP_ERR_SIGNAL=3,
 BP_ERR_SENSOR=4, BP_ERR_ZERO=5, BP_ERR_TIMEOUT=6, BP_ERR_BLEED=7,
 BP_ERR_COVERAGE=8, BP_ERR_TIMING=9
};
#define MAX_SAFETY_PRESSURE_MMHG 260.0f
#define DEFAULT_INFLATE_MMHG 165.0f
#define STOP_DEFLATE_MMHG 45.0f
#define INFLATE_TIMEOUT_MS 40000UL
#define MEASURE_TOTAL_TIMEOUT_MS 95000UL
/* Nominal 80Hz conversion starts; every sample carries its actual timestamp. */
#define SAMPLE_PERIOD_US 12500UL
#define SENSOR_TIMEOUT_MS 100UL
#define KEY_DEBOUNCE_MS 200UL
#define STEP_PLATEAU_HOLD_MS 240UL
/* Experimental values retained for the existing valve; validate on the bench. */
#define STEP_WATER_HAMMER_BLANK_MS 38UL
void App_RequestToggleStartStop(void);
void App_RequestStart(void);
void App_RequestStop(void);
void App_Init(void);
void App_Process(void);
SystemState_t App_GetState(void);
#endif
