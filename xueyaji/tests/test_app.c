#include "main.h"
#include "xgzp6847d.h"
#include "oled_ssd1306.h"
#include "pump_valve.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
static uint32_t now,seq,service_delay; static bool online=true,pump,valve_closed;
static float raw,zero; static char log_buffer[2048];
uint32_t HAL_GetTick(void) { return now; }
void BSP_IO_Service(void) { }
void BSP_WatchdogFeed(void) { }
void BSP_SetMeasuringFlag(bool b) { (void)b; }
void BSP_UART1_SendString(const char *s) {
 if (strlen(log_buffer)+strlen(s)<sizeof(log_buffer)) strcat(log_buffer,s);
}
void XGZP6847D_Service(void) { if (service_delay) { now+=service_delay;seq++; } }
bool XGZP6847D_IsOnline(void) { return online; }
bool XGZP6847D_GetSample(PressureSample_t *p) {
 if (!online) return false;
 p->raw_mmhg=raw; p->pressure_mmhg=raw-zero;
 p->sequence=seq; p->timestamp_ms=now-now%13U; return true;
}
bool XGZP6847D_SetZero(float p) { zero=p; return online; }
void Pump_SetSpeed(uint8_t p) { pump=p!=0; }
void Pump_StartSoftRamp(uint8_t p) { pump=p!=0; }
void Pump_UpdateSoftRamp(void) { }
void Valve_Open(void) { valve_closed=false; }
void Valve_Close(void) { valve_closed=true; }
void Emergency_Exhaust(void) { pump=false; valve_closed=false; }
void Valve_ResetSmoothBleed(float p) { (void)p; }
uint32_t Valve_StepBleedAdaptive(float p) { (void)p; return 600; }
void OLED_Clear(void) { }
void OLED_Refresh(void) { }
void OLED_Service(void) { }
bool OLED_IsBusy(void) { return false; }
void OLED_ShowString(uint8_t x,uint8_t y,const char*s,uint8_t z) { (void)x;(void)y;(void)s;(void)z; }
void OLED_DrawPressureBar(float p,float t,uint16_t n) { (void)p;(void)t;(void)n; }
void OLED_ShowResult(float a,float b,uint16_t c) { (void)a;(void)b;(void)c; }
void OLED_ShowError(uint8_t e) { (void)e; }
static void advance(unsigned ms) {
 while (ms--) { now++; if (now%13U==0) seq++; App_Process(); }
}
static void reset(void) {
 now=0;seq=1;service_delay=0;online=true;raw=zero=0;log_buffer[0]=0;App_Init();advance(50);
}
static void inflate(void) {
 App_RequestStart();advance(3000);
 assert(App_GetState()==STATE_INFLATE && pump && valve_closed);
}
static void deflate(void) {
 inflate();raw=166;advance(30);assert(!pump);advance(800);
 assert(App_GetState()==STATE_DEFLATE_SAMPLE);
}
static void safe_error(const char *code) {
 assert(App_GetState()==STATE_ERROR); assert(!pump && !valve_closed);
 assert(strstr(log_buffer,code)); assert(!strstr(log_buffer,"RESULT:"));
}
int main(void) {
 reset();service_delay=2;App_RequestStart();advance(100);
 assert(App_GetState()==STATE_SELF_CHECK); /* Tick advances inside I2C service. */
 reset();inflate();online=false;advance(120);safe_error("E04");
 reset();inflate();raw=270;advance(20);safe_error("E01");
 reset();inflate();advance(40000);safe_error("E02");
 reset();App_RequestStart();raw=15;advance(8100);safe_error("E05");
 reset();deflate();advance(8100);safe_error("E07");
 reset();inflate();App_RequestStop();advance(1);
 assert(App_GetState()==STATE_IDLE && !pump && !valve_closed);
 /* Idempotent start must not stop an active measurement. */
 reset();inflate();App_RequestStart();advance(1);assert(pump);
 /* Total limit survives transitions; use a slow but allowed 1mmHg/s deflation. */
 reset();inflate();advance(30000);raw=166;advance(850);
 assert(App_GetState()==STATE_DEFLATE_SAMPLE);
 for (unsigned t=0;t<65000 && App_GetState()!=STATE_ERROR;t++) { raw=166.0f-t*0.001f;advance(1); }
 safe_error("E06");
 /* Real algorithm, simulated sensor: complete pressure sweep and result packet. */
 reset();deflate();
 for (unsigned t=0;t<32000 && App_GetState()==STATE_DEFLATE_SAMPLE;t++) {
  float p=166.0f-t*0.004f;
  float a=3.0f*expf(-((p-100)*(p-100))/(2*22.0f*22.0f));
  raw=p+a*sinf(2*3.14159265f*1.2f*t*0.001f);advance(1);
 }
 advance(50);
 if (!strstr(log_buffer,"RESULT:")) puts(log_buffer);
 assert(strstr(log_buffer,"RESULT:")); assert(!pump && !valve_closed);
 puts("PASS app: sensor loss, overpressure, timeouts, calibration, stalled valve, stop/start, measurement");
}
