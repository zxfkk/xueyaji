#ifndef __XGZP6847D_H
#define __XGZP6847D_H
#include "bsp_sys.h"
#define XGZP6847D_ADDR_6D (0x6D << 1)
#define XGZP6847D_ADDR_58 (0x58 << 1)
#define XGZP_REG_CMD 0x30
#define XGZP_REG_PRESS_MSB 0x06
#define XGZP_CMD_SINGLE_CONV 0x0A
/* Verify the purchased 040KPG sensor and calibrate against a reference. */
#define XGZP6847D_SCALE_K 128.0f
#define PA_PER_MMHG 133.322368f
typedef struct {
 float raw_mmhg; /* signed gauge value before zero correction */
 float pressure_mmhg;
 uint32_t timestamp_ms;
 uint32_t sequence;
} PressureSample_t;
void XGZP6847D_Init(void);
void XGZP6847D_Service(void);
bool XGZP6847D_GetSample(PressureSample_t *sample);
bool XGZP6847D_IsOnline(void);
bool XGZP6847D_SetZero(float raw_mmhg);
#endif
