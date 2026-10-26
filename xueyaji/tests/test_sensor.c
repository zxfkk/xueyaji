#include "xgzp6847d.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static uint32_t now,raw=0; static bool ack=true,busy=false,read_ok=true;
uint32_t HAL_GetTick(void) { return now; }
uint32_t BSP_GetMicros(void) { return now*1000UL; }
bool BSP_I2C_WriteReg(I2C_Bus_t b,uint8_t a,uint8_t r,uint8_t v) {
 (void)b;(void)a;(void)r;(void)v; return ack;
}
bool BSP_I2C_ReadRegs(I2C_Bus_t b,uint8_t a,uint8_t r,uint8_t *buf,uint16_t n) {
 (void)b;(void)a;(void)n;
 if (!read_ok) return false;
 if (r==XGZP_REG_CMD) buf[0]=busy?8:0;
 else { buf[0]=(raw>>16)&255; buf[1]=(raw>>8)&255; buf[2]=raw&255; }
 return true;
}
static void advance(unsigned ms) { while (ms--) { now++; XGZP6847D_Service(); } }
int main(void) {
 PressureSample_t p; uint32_t seq;
 XGZP6847D_Init(); assert(!XGZP6847D_GetSample(&p));
 advance(50); assert(XGZP6847D_GetSample(&p)); assert(p.raw_mmhg==0);
 raw=0x00FFFFUL; advance(40); assert(XGZP6847D_GetSample(&p));
 assert(p.raw_mmhg>3 && p.raw_mmhg<4); /* Low 16 bits FF is legitimate. */
 assert(XGZP6847D_SetZero(p.raw_mmhg)); assert(XGZP6847D_GetSample(&p));
 assert(fabsf(p.pressure_mmhg)<0.001f);
 seq=p.sequence; busy=true; advance(20);
 assert(XGZP6847D_GetSample(&p)); assert(p.sequence==seq);
 advance(110); assert(!XGZP6847D_GetSample(&p)); assert(!XGZP6847D_SetZero(0));
 busy=false; raw=0x7FFFFFUL; advance(50); assert(!XGZP6847D_GetSample(&p));
 raw=0; advance(50); assert(XGZP6847D_GetSample(&p));
 read_ok=false; advance(30); assert(!XGZP6847D_GetSample(&p));
 read_ok=true; advance(50); assert(XGZP6847D_GetSample(&p));
 ack=false; advance(150); assert(!XGZP6847D_GetSample(&p));
 ack=true; raw=0xFFFFFFUL; advance(50); assert(XGZP6847D_GetSample(&p));
 assert(p.raw_mmhg<0); assert(p.pressure_mmhg<0); /* Never absolute-value. */
 assert(!XGZP6847D_SetZero(11));
 puts("PASS sensor: conversion timeout, I2C failure, signed data, zero, saturation, recovery");
}
