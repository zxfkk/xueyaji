/* Nonblocking single-conversion driver. A failed transaction never creates a sample. */
#include "xgzp6847d.h"
#include "main.h"
#include <string.h>
#include <math.h>
static I2C_Bus_t bus;
static uint8_t addr, probe;
static bool selected, pending, online;
static uint32_t deadline_us, started_ms, poll_ms, retry_ms;
static float zero;
static PressureSample_t latest;
static bool finite_value(float x) { return x == x && x > -1000000.0f && x < 1000000.0f; }
static void failed(uint32_t now) {
 online=false; pending=false; selected=false; retry_ms=now+10UL;
}
void XGZP6847D_Init(void) {
 bus=I2C_BUS_1; addr=XGZP6847D_ADDR_6D; probe=0;
 selected=false; pending=false; online=false; zero=0;
 memset(&latest,0,sizeof(latest));
 deadline_us=BSP_GetMicros(); retry_ms=HAL_GetTick();
}
bool XGZP6847D_IsOnline(void) { return online; }
bool XGZP6847D_SetZero(float raw) {
 if (!online || !finite_value(raw) || fabsf(raw)>10.0f) return false;
 zero=raw; return true;
}
bool XGZP6847D_GetSample(PressureSample_t *sample) {
 if (!sample || !online || latest.sequence==0 ||
     HAL_GetTick()-latest.timestamp_ms>SENSOR_TIMEOUT_MS) return false;
 *sample=latest; sample->pressure_mmhg=sample->raw_mmhg-zero; return true;
}
void XGZP6847D_Service(void) {
 uint32_t now=HAL_GetTick(), us=BSP_GetMicros();
 if (!pending) {
  if ((int32_t)(now-retry_ms)<0 || (int32_t)(us-deadline_us)<0) return;
  if (!selected) {
   bus=(probe&2U)?I2C_BUS_2:I2C_BUS_1;
   addr=(probe&1U)?XGZP6847D_ADDR_58:XGZP6847D_ADDR_6D;
   probe=(uint8_t)((probe+1U)&3U);
  }
  if (!BSP_I2C_WriteReg(bus,addr,XGZP_REG_CMD,XGZP_CMD_SINGLE_CONV)) { failed(now); return; }
  pending=true; started_ms=now; poll_ms=now+1UL;
  deadline_us+=SAMPLE_PERIOD_US;
  /* Do not burst conversions to catch up after a scheduling delay. */
  if ((int32_t)(us-deadline_us)>=0) deadline_us=us+SAMPLE_PERIOD_US;
  return;
 }
 if (now-started_ms>=30UL) { failed(now); return; }
 if ((int32_t)(now-poll_ms)<0) return;
 poll_ms=now+1UL;
 {
  uint8_t status, bytes[3]; uint32_t raw; int32_t signed_raw; float pressure;
  if (!BSP_I2C_ReadRegs(bus,addr,XGZP_REG_CMD,&status,1)) { failed(now); return; }
  if (status==0xFFU) { failed(now); return; }
  if (status&0x08U) return;
  if (!BSP_I2C_ReadRegs(bus,addr,XGZP_REG_PRESS_MSB,bytes,3)) { failed(now); return; }
  raw=((uint32_t)bytes[0]<<16)|((uint32_t)bytes[1]<<8)|bytes[2];
  if (raw==0x7FFFFFUL || raw==0x800000UL) { failed(now); return; }
  signed_raw=(raw&0x800000UL)?(int32_t)raw-16777216L:(int32_t)raw;
  pressure=(float)signed_raw/XGZP6847D_SCALE_K/PA_PER_MMHG;
  if (!finite_value(pressure) || pressure < -20.0f || pressure > 330.0f) { failed(now); return; }
  latest.raw_mmhg=pressure; latest.timestamp_ms=HAL_GetTick(); latest.sequence++;
  if (!latest.sequence) latest.sequence=1;
  online=true; selected=true; pending=false;
 }
}
