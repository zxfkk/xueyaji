#ifndef __BP_ALGORITHM_H
#define __BP_ALGORITHM_H
#include "main.h"
#define MAX_PULSE_PEAKS 192U
#define KSBP_RATIO 0.55f
#define KDBP_RATIO 0.72f
typedef struct { float cuff_pressure,pulse_amp; uint32_t timestamp_ms; } PulsePeak_t;
void BP_Algorithm_Reset(void);
void BP_Algorithm_NotifyValvePulse(uint32_t tick);
void BP_Algorithm_PushSample(float pressure,uint32_t tick);
float BP_Algorithm_GetRecentPulseAmp(void);
uint16_t BP_Algorithm_GetPeakCount(void);
bool BP_Algorithm_Calculate(BP_Result_t *result);
#endif
