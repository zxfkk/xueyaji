/**
  ******************************************************************************
  * @file    oled_ssd1306.h
  * @brief   0.96寸 OLED (SSD1306 驱动, 128x64, 4针IIC) 完整字库与图形驱动头文件
  ******************************************************************************
  */
#ifndef __OLED_SSD1306_H
#define __OLED_SSD1306_H

#include "bsp_sys.h"

/* SSD1306 标准 I2C 7-bit 地址 0x3C (左移1位写地址 0x78) */
#define OLED_I2C_ADDR   0x78U

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_Service(void);
bool OLED_IsBusy(void);
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);
void OLED_DrawPressureBar(float current_pressure, float target_pressure, uint16_t pulse_cnt);
void OLED_ShowResult(float sbp, float dbp, uint16_t hr);
void OLED_ShowError(uint8_t err_code);

#endif /* __OLED_SSD1306_H */
