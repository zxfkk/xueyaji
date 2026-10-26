/**
  ******************************************************************************
  * @file    bsp_sys.h
  * @brief   STM32F103C8T6 板级支持包头文件 (SysTick / GPIO / EXTI0 / USART1 / I2C)
  ******************************************************************************
  */
#ifndef __BSP_SYS_H
#define __BSP_SYS_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>

/* I2C 物理通道枚举:
 * I2C_BUS_1: PB6 (SCL), PB7 (SDA)  —— 推荐传感器与 OLED 共接此总线以共享 OLED 板载 4.7k 上拉电阻
 * I2C_BUS_2: PB10(SCL), PB11(SDA)  —— 独立第二通道 (若 OLED 单独接 PB10/PB11 亦可自动识别)
 */
typedef enum {
    I2C_BUS_1 = 0,  /* PB6=SCL, PB7=SDA */
    I2C_BUS_2 = 1   /* PB10=SCL, PB11=SDA */
} I2C_Bus_t;

/* 系统滴答与延时接口 */
void     BSP_Sys_Init(void);
bool     BSP_CheckResetKeyStart(void);
void     BSP_SetMeasuringFlag(bool is_measuring);
uint32_t HAL_GetTick(void);
void     HAL_Delay(uint32_t Delay_ms);
void     BSP_DelayUs(uint32_t us);
uint32_t BSP_GetMicros(void);
void BSP_IO_Service(void);
void BSP_WatchdogFeed(void);

/* 板载 PC13 状态指示灯接口 */
void     BSP_LED_Set(bool on);
void     BSP_LED_Toggle(void);

/* HC-05 蓝牙串口 (USART1: PA9=TX, PA10=RX, 9600 8N1) 接口 */
void     BSP_UART1_Init(uint32_t baudrate);
void     BSP_UART1_SendString(const char *str);

/* 防死锁高速 I2C 总线读写接口 (支持 7-bit 左移后的 8-bit 写地址 dev_addr_8bit) */
void     BSP_I2C_Init(void);
bool     BSP_I2C_CheckDevice(I2C_Bus_t bus, uint8_t dev_addr_8bit);
bool     BSP_I2C_WriteReg(I2C_Bus_t bus, uint8_t dev_addr_8bit, uint8_t reg, uint8_t data);
bool     BSP_I2C_WriteMulti(I2C_Bus_t bus, uint8_t dev_addr_8bit, uint8_t reg, const uint8_t *buf, uint16_t len);
bool     BSP_I2C_ReadRegs(I2C_Bus_t bus, uint8_t dev_addr_8bit, uint8_t reg, uint8_t *buf, uint16_t len);

#endif /* __BSP_SYS_H */
