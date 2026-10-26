/**
  ******************************************************************************
  * @file    system_stm32f10x.c
  * @brief   STM32F103C8T6 系统时钟配置 (8MHz HSE -> 72MHz PLL, 含 HSI 防死机回退)
  ******************************************************************************
  */
#include "stm32f10x.h"

uint32_t SystemCoreClock = 72000000UL;

void SystemInit(void) {
    uint32_t timeout = 0x5000UL;

    /* 1. 开启内部 8MHz HSI 并复位 RCC 配置 */
    RCC->CR   |= 0x00000001UL;
    RCC->CFGR &= 0xF8FF0000UL;
    RCC->CR   &= 0xFEF6FFFFUL;
    RCC->CR   &= 0xFFFBFFFFUL;
    RCC->CFGR &= 0xFF80FFFFUL;
    RCC->CIR   = 0x009F0000UL;

    /* 2. 开启外部 8MHz 晶振 (HSEON = Bit 16) */
    RCC->CR |= (1UL << 16);
    while (((RCC->CR & (1UL << 17)) == 0UL) && (timeout > 0UL)) {
        timeout--;
    }

    /* 3. 配置 FLASH 预取指与 2 个等待周期 (72MHz 必配) */
    FLASH->ACR |= 0x00000010UL;
    FLASH->ACR &= ~0x00000007UL;
    FLASH->ACR |= 0x00000002UL;

    /* 4. HCLK = SYSCLK, PCLK2 = HCLK(72MHz), PCLK1 = HCLK/2(36MHz) */
    RCC->CFGR |= (0x0UL << 4) | (0x0UL << 11) | (0x4UL << 8);

    if (RCC->CR & (1UL << 17)) {
        /* 外部晶振就绪: PLLSRC = HSE, PLLMULL = x9 (8MHz * 9 = 72MHz) */
        RCC->CFGR &= ~(0xFUL << 18);
        RCC->CFGR |= (1UL << 16) | (0x7UL << 18);
        SystemCoreClock = 72000000UL;
    } else {
        /* 若外部晶振未起振，自动回退至 HSI/2 * 16 = 64MHz，绝不卡死在启动阶段 */
        RCC->CFGR &= ~(0xFUL << 18);
        RCC->CFGR |= (0xEUL << 18);
        SystemCoreClock = 64000000UL;
    }

    /* 5. 使能 PLL (PLLON = Bit 24) 并等待锁定 (PLLRDY = Bit 25) */
    RCC->CR |= (1UL << 24);
    while ((RCC->CR & (1UL << 25)) == 0UL) {
    }

    /* 6. 切换系统时钟源至 PLL (SW = 0b10) */
    RCC->CFGR &= ~0x00000003UL;
    RCC->CFGR |=  0x00000002UL;
    while ((RCC->CFGR & 0x0000000CUL) != 0x00000008UL) {
    }
}

void SystemCoreClockUpdate(void) {
    /* SystemInit runs before C data initialization; derive rather than trust its assignment. */
    uint32_t cfg=RCC->CFGR, source=(cfg>>2)&3U;
    uint32_t clock=8000000UL;
    if (source==2U) {
        uint32_t input=(cfg&(1UL<<16))?8000000UL:4000000UL;
        uint32_t multiplier=((cfg>>18)&15U)+2U;
        if ((cfg&(1UL<<16)) && (cfg&(1UL<<17))) input/=2U;
        if (multiplier>16U) multiplier=16U;
        clock=input*multiplier;
    }
    SystemCoreClock=clock;
}
