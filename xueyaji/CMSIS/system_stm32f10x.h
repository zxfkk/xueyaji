/**
  ******************************************************************************
  * @file    system_stm32f10x.h
  * @brief   STM32F103C8T6 系统时钟初始化头文件 (72MHz HSE PLL)
  ******************************************************************************
  */
#ifndef __SYSTEM_STM32F10X_H
#define __SYSTEM_STM32F10X_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern uint32_t SystemCoreClock;

extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_STM32F10X_H */
