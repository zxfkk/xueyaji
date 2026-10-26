/* Cooperative firmware: no blocking waits after initialization. */
#include "main.h"
#include "bsp_sys.h"
#include "xgzp6847d.h"
#include "pump_valve.h"
#include "oled_ssd1306.h"
#include "bp_algorithm.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static SystemState_t state;
static BP_Result_t result;
static uint32_t state_at, measure_at, inflate_at, valid_at, sequence;
static uint32_t phase_at,last_ramp,last_bleed,last_ui,last_slow,zero_at;
static uint8_t phase,retries,above,hist_n,hist_i;
static uint16_t zero_count;
static float pressure,raw_pressure,shown_pressure,slow_pressure,watch_pressure,bleed_ref;
static float zero_sum,zero_min,zero_max,hist[5];
static bool have_pressure,finished;
static uint8_t requests;
void App_RequestStart(void) { requests|=1U; }
void App_RequestStop(void) { requests|=2U; }
void App_RequestToggleStartStop(void) {
 if (state==STATE_IDLE || state==STATE_ERROR) App_RequestStart();
 else App_RequestStop();
}
SystemState_t App_GetState(void) { return state; }
static bool active(void) { return state!=STATE_IDLE && state!=STATE_ERROR; }
/* Median of the last 5 samples (~62ms at 80Hz): drops isolated I2C/EMI spikes
 * that used to stop the pump early or trip overpressure, but keeps the ~1Hz
 * oscillometric pulse. Every control decision uses this value, never raw. */
static float median5(float x) {
 float s[5]; uint8_t i,j;
 hist[hist_i]=x; hist_i=(uint8_t)((hist_i+1U)%5U); if (hist_n<5U) hist_n++;
 for (i=0;i<hist_n;i++) {
  float v=hist[i]; j=i;
  while (j && s[j-1U]>v) { s[j]=s[j-1U]; j--; }
  s[j]=v;
 }
 return s[hist_n/2U];
}
static void change(SystemState_t next) { state=next; state_at=HAL_GetTick(); }
static void fault(uint8_t code) {
 char msg[32];
 Emergency_Exhaust(); result.valid=false; result.err_code=code;
 change(STATE_ERROR); finished=false;
 snprintf(msg,sizeof(msg),"ERR:E%02u\r\n",code); BSP_UART1_SendString(msg);
}
void App_Init(void) {
 Emergency_Exhaust(); memset(&result,0,sizeof(result));
 requests=0; sequence=0; have_pressure=false; finished=false;
 valid_at=HAL_GetTick(); last_ui=valid_at-500UL; change(STATE_IDLE);
}
static void start(void) {
 Emergency_Exhaust(); memset(&result,0,sizeof(result));
 BP_Algorithm_Reset(); measure_at=HAL_GetTick(); phase=0; retries=0;
 zero_count=0; above=0; hist_n=0; hist_i=0; have_pressure=false;
 finished=false; BSP_SetMeasuringFlag(true);
 change(STATE_SELF_CHECK);
}
void App_Process(void) {
 uint32_t now=HAL_GetTick();
 PressureSample_t sample;
 bool fresh=false;
 XGZP6847D_Service(); BSP_IO_Service();
 if (requests&2U) {
  requests=0; Emergency_Exhaust(); result.valid=false;
  change(STATE_IDLE); finished=false;
 } else if (requests&1U) {
  requests=0; if (!active()) start();
 }
 if (XGZP6847D_GetSample(&sample) && sample.sequence!=sequence) {
  sequence=sample.sequence; valid_at=sample.timestamp_ms;
  raw_pressure=sample.pressure_mmhg; pressure=median5(raw_pressure); fresh=true;
  if (!have_pressure) { slow_pressure=shown_pressure=pressure; have_pressure=true; }
  else {
   slow_pressure+=0.015f*(pressure-slow_pressure);
   shown_pressure+=0.25f*(pressure-shown_pressure);
  }
 }
 /* I2C may cross a millisecond boundary. Refresh after service: subtracting
  * a newer sample/start timestamp from the old tick would underflow. */
 now=HAL_GetTick();
 if (active()) {
  if (now-measure_at>=MEASURE_TOTAL_TIMEOUT_MS) fault(BP_ERR_TIMEOUT);
  else if (now-valid_at>=SENSOR_TIMEOUT_MS) fault(BP_ERR_SENSOR);
  else if (fresh && pressure>=MAX_SAFETY_PRESSURE_MMHG) fault(BP_ERR_PRESSURE);
 }
 switch (state) {
 case STATE_IDLE:
 case STATE_ERROR:
  Emergency_Exhaust();
  if (now-state_at>=1500UL) BSP_SetMeasuringFlag(false);
  break;
 case STATE_SELF_CHECK:
  if (phase==0) {
   Emergency_Exhaust();
   if (now-state_at>=8000UL) { fault(BP_ERR_ZERO); break; }
   if (now-state_at<2000UL || !fresh) break;
   if (fabsf(sample.raw_mmhg)>10.0f) { zero_count=0; break; }
   if (!zero_count) { zero_sum=0; zero_min=sample.raw_mmhg; zero_max=zero_min; zero_at=now; }
   zero_sum+=sample.raw_mmhg; zero_count++;
   if (sample.raw_mmhg<zero_min) zero_min=sample.raw_mmhg;
   if (sample.raw_mmhg>zero_max) zero_max=sample.raw_mmhg;
   if (zero_count>=32U && now-zero_at>=400UL) {
    if (zero_max-zero_min>0.6f) { zero_count=0; break; }
    if (!XGZP6847D_SetZero(zero_sum/(float)zero_count)) { fault(BP_ERR_ZERO); break; }
    Valve_Close(); phase=1; phase_at=now;
   }
  } else if (now-phase_at>=200UL) {
   Pump_StartSoftRamp(52); inflate_at=now; last_ramp=now; above=0; change(STATE_INFLATE);
  }
  break;
 case STATE_INFLATE:
  if (now-inflate_at>=INFLATE_TIMEOUT_MS) { fault(BP_ERR_INFLATE); break; }
  if (now-last_ramp>=12UL) { Pump_UpdateSoftRamp(); last_ramp=now; }
  if (fresh) above=(pressure>=DEFAULT_INFLATE_MMHG)?(uint8_t)(above+1U):0U;
  if (above>=INFLATE_CONFIRM_SAMPLES) {
   Pump_SetSpeed(0); change(STATE_SETTLE);
  }
  break;
 case STATE_SETTLE:
  if (now-inflate_at>=INFLATE_TIMEOUT_MS) { fault(BP_ERR_INFLATE); break; }
  if (fresh && now-state_at>=SETTLE_MS) {
   if (pressure<DEFAULT_INFLATE_MMHG-SETTLE_ACCEPT_MARGIN_MMHG) {
    if (++retries>3U) { fault(BP_ERR_INFLATE); break; }
    Pump_StartSoftRamp(58); last_ramp=now; above=0; change(STATE_INFLATE);
   } else {
    BP_Algorithm_Reset(); slow_pressure=pressure;
    Valve_ResetSmoothBleed(slow_pressure);
    last_bleed=now; last_slow=now; watch_pressure=bleed_ref=slow_pressure;
    change(STATE_DEFLATE_SAMPLE);
   }
  }
  break;
 case STATE_DEFLATE_SAMPLE:
  /* Exit and fault conditions always run, even when a valve pulse is due. */
  if (fresh && pressure<=STOP_DEFLATE_MMHG && now-state_at>=6000UL) {
   Emergency_Exhaust(); change(STATE_CALCULATE); break;
  }
  if (now-last_slow>=8000UL) {
   float drop=watch_pressure-slow_pressure;
   if (drop<BLEED_MIN_DROP_8S_MMHG || drop>BLEED_MAX_DROP_8S_MMHG) { fault(BP_ERR_BLEED); break; }
   last_slow=now; watch_pressure=slow_pressure;
  }
  if (fresh) BP_Algorithm_PushSample(pressure,sample.timestamp_ms);
  /* Valve stays closed: the cuff leaks down on its own. Measure the smoothed
   * leak rate once per second and add one valve pulse only if it is too slow. */
  if (fresh && now-last_bleed>=DEFLATE_CHECK_MS) {
   float rate=(bleed_ref-slow_pressure)*1000.0f/(float)(now-last_bleed);
   bleed_ref=slow_pressure;
   if (Valve_LeakAssist(slow_pressure,rate)) BP_Algorithm_NotifyValvePulse(HAL_GetTick());
   last_bleed=HAL_GetTick();
  }
  break;
 case STATE_CALCULATE:
  Emergency_Exhaust();
  if (!BP_Algorithm_Calculate(&result)) { fault(result.err_code); break; }
  {
   char msg[64];
   snprintf(msg,sizeof(msg),"RESULT:SYS=%.0f,DIA=%.0f,MAP=%.0f,HR=%u\r\n",
      result.sbp,result.dbp,result.map,result.hr);
   BSP_UART1_SendString(msg);
  }
  finished=true; change(STATE_IDLE); break;
 default: fault(BP_ERR_SIGNAL); break;
 }
 /* Ship one bounded OLED chunk after sensor service and safety checks.
  * At most one frame every 500ms; raw sampling and safety take priority. */
 if (!OLED_IsBusy() && now-last_ui>=500UL) {
  char telemetry[64];
  last_ui=now; OLED_Clear();
  if (active()) {
   snprintf(telemetry,sizeof(telemetry),"T:%lu,P:%.2f,R:%.2f,N:%u\r\n",
      (unsigned long)valid_at,pressure,raw_pressure,BP_Algorithm_GetPeakCount());
   BSP_UART1_SendString(telemetry);
  }
  if (state==STATE_ERROR) OLED_ShowError(result.err_code);
  else if (finished) OLED_ShowResult(result.sbp,result.dbp,result.hr);
  else if (state==STATE_SELF_CHECK) { OLED_ShowString(0,16,"Zeroing...",16); OLED_Refresh(); }
  else if (state==STATE_IDLE) {
   OLED_ShowString(0,0,"Smart BP Monitor",16);
   OLED_ShowString(0,24,XGZP6847D_IsOnline()?"Press RESET / A0":"Sensor NoACK!",16);
   OLED_Refresh();
  } else OLED_DrawPressureBar(shown_pressure,DEFAULT_INFLATE_MMHG,BP_Algorithm_GetPeakCount());
 }
 OLED_Service();
 BSP_WatchdogFeed();
}
#ifndef BP_HOST_TEST
int main(void) {
 BSP_Sys_Init(); Actuator_Init(); Emergency_Exhaust();
 HAL_Delay(250); BSP_I2C_Init(); BSP_UART1_Init(9600UL); OLED_Init();
 XGZP6847D_Init(); App_Init();
 if (BSP_CheckResetKeyStart()) App_RequestStart();
 while (1) App_Process();
}
/* Override weak startup fault loops: release actuators, let watchdog reset. */
void HardFault_Handler(void) { Emergency_Exhaust(); while (1) { } }
void MemManage_Handler(void) { HardFault_Handler(); }
void BusFault_Handler(void) { HardFault_Handler(); }
void UsageFault_Handler(void) { HardFault_Handler(); }
#endif
