/**
  ******************************************************************************
  * @file    pump_valve.h
  * @brief   ZR370-03PM 气泵 PWM 软启动与 Fa0520C 常开电磁阀 Plan B 自适应脉冲控制
  ******************************************************************************
  */
#ifndef __PUMP_VALVE_H
#define __PUMP_VALVE_H

#include "bsp_sys.h"

/* 初始化 PA7 (TIM3_CH2 20kHz 硬件 PWM) 与 PB0 (推挽输出控制常开电磁阀) */
void Actuator_Init(void);

/* 直接设置气泵 PWM 占空比 (duty: 0 ~ 100%) */
void Pump_SetSpeed(uint8_t duty_percent);

/* 启动气泵斜坡软启动 (20% -> target_duty_percent，防止 14500 电池瞬态压降重启) */
void Pump_StartSoftRamp(uint8_t target_duty_percent);

/* Called on the independent 12ms ramp schedule. */
void Pump_UpdateSoftRamp(void);

/* 电磁阀通电闭合 (PB0 = 1，用于充气与静态平台保压) */
void Valve_Close(void);

/* 电磁阀断电开启 (PB0 = 0，常开阀失电全开快速泄气) */
void Valve_Open(void);

/* 重置闭环柔顺放气控制器状态 (在进入放气阶段前调用) */
void Valve_ResetSmoothBleed(float start_pressure_mmhg);

/* 自然漏气辅助：rate 为平滑后的实测泄压速率 (mmHg/s)。
 * 低于 2.0 mmHg/s 时开一个微脉冲并返回脉宽 (us)，否则不动阀门并返回 0。 */
uint32_t Valve_LeakAssist(float current_pressure_mmhg, float rate_mmhg_s);

/* 紧急完全泄压与断电停泵 */
void Emergency_Exhaust(void);

#endif /* __PUMP_VALVE_H */
