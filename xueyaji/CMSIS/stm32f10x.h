/**
  ******************************************************************************
  * @file    stm32f10x.h
  * @brief   STM32F103x8/B (Medium-Density) 外设寄存器映像与位定义头文件
  ******************************************************************************
  */
#ifndef __STM32F10x_H
#define __STM32F10x_H

#ifdef __cplusplus
extern "C" {
#endif

#include "core_cm3.h"
#include "system_stm32f10x.h"

/* 中断号定义 (IRQn) */
typedef enum IRQn {
    NonMaskableInt_IRQn   = -14,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn         = -11,
    UsageFault_IRQn       = -10,
    SVCall_IRQn           = -5,
    DebugMonitor_IRQn     = -4,
    PendSV_IRQn           = -2,
    SysTick_IRQn          = -1,
    EXTI0_IRQn            = 6,   /*!< PA0 外部中断 0 */
    EXTI4_IRQn            = 10,  /*!< PA4 外部中断 4 */
    TIM2_IRQn             = 28,  /*!< TIM2 全局中断  */
    TIM3_IRQn             = 29,  /*!< TIM3 全局中断  */
    I2C1_EV_IRQn          = 31,  /*!< I2C1 事件中断  */
    I2C2_EV_IRQn          = 33,  /*!< I2C2 事件中断  */
    USART1_IRQn           = 37   /*!< USART1 全局中断 */
} IRQn_Type;

/* GPIO 寄存器结构体 */
typedef struct {
    __IO uint32_t CRL;           /*!< 端口配置低寄存器 (Pin 0~7)  */
    __IO uint32_t CRH;           /*!< 端口配置高寄存器 (Pin 8~15) */
    __IO uint32_t IDR;           /*!< 端口输入数据寄存器          */
    __IO uint32_t ODR;           /*!< 端口输出数据寄存器          */
    __IO uint32_t BSRR;          /*!< 端口位设置/清除寄存器       */
    __IO uint32_t BRR;           /*!< 端口位清除寄存器            */
    __IO uint32_t LCKR;          /*!< 端口配置锁定寄存器          */
} GPIO_TypeDef;

/* AFIO 复用功能寄存器结构体 */
typedef struct {
    __IO uint32_t EVCR;
    __IO uint32_t MAPR;
    __IO uint32_t EXTICR[4];
    uint32_t      RESERVED0;
    __IO uint32_t MAPR2;
} AFIO_TypeDef;

/* EXTI 外部中断寄存器结构体 */
typedef struct {
    __IO uint32_t IMR;           /*!< 中断屏蔽寄存器     */
    __IO uint32_t EMR;           /*!< 事件屏蔽寄存器     */
    __IO uint32_t RTSR;          /*!< 上升沿触发选择     */
    __IO uint32_t FTSR;          /*!< 下降沿触发选择     */
    __IO uint32_t SWIER;         /*!< 软件中断事件寄存器 */
    __IO uint32_t PR;            /*!< 挂起寄存器         */
} EXTI_TypeDef;

/* RCC 时钟控制寄存器结构体 */
typedef struct {
    __IO uint32_t CR;
    __IO uint32_t CFGR;
    __IO uint32_t CIR;
    __IO uint32_t APB2RSTR;
    __IO uint32_t APB1RSTR;
    __IO uint32_t AHBENR;
    __IO uint32_t APB2ENR;
    __IO uint32_t APB1ENR;
    __IO uint32_t BDCR;
    __IO uint32_t CSR;
} RCC_TypeDef;

/* FLASH 接口寄存器结构体 */
typedef struct {
    __IO uint32_t ACR;
    __IO uint32_t KEYR;
    __IO uint32_t OPTKEYR;
    __IO uint32_t SR;
    __IO uint32_t CR;
    __IO uint32_t AR;
    __IO uint32_t RESERVED;
    __IO uint32_t OBR;
    __IO uint32_t WRPR;
} FLASH_TypeDef;

/* TIM 通用定时器寄存器结构体 */
typedef struct {
    __IO uint16_t CR1;
    uint16_t      RESERVED0;
    __IO uint16_t CR2;
    uint16_t      RESERVED1;
    __IO uint16_t SMCR;
    uint16_t      RESERVED2;
    __IO uint16_t DIER;
    uint16_t      RESERVED3;
    __IO uint16_t SR;
    uint16_t      RESERVED4;
    __IO uint16_t EGR;
    uint16_t      RESERVED5;
    __IO uint16_t CCMR1;
    uint16_t      RESERVED6;
    __IO uint16_t CCMR2;
    uint16_t      RESERVED7;
    __IO uint16_t CCER;
    uint16_t      RESERVED8;
    __IO uint16_t CNT;
    uint16_t      RESERVED9;
    __IO uint16_t PSC;
    uint16_t      RESERVED10;
    __IO uint16_t ARR;
    uint16_t      RESERVED11;
    __IO uint16_t RCR;
    uint16_t      RESERVED12;
    __IO uint16_t CCR1;
    uint16_t      RESERVED13;
    __IO uint16_t CCR2;
    uint16_t      RESERVED14;
    __IO uint16_t CCR3;
    uint16_t      RESERVED15;
    __IO uint16_t CCR4;
    uint16_t      RESERVED16;
    __IO uint16_t BDTR;
    uint16_t      RESERVED17;
    __IO uint16_t DCR;
    uint16_t      RESERVED18;
    __IO uint16_t DMAR;
    uint16_t      RESERVED19;
} TIM_TypeDef;

/* USART 串口寄存器结构体 */
typedef struct {
    __IO uint16_t SR;
    uint16_t      RESERVED0;
    __IO uint16_t DR;
    uint16_t      RESERVED1;
    __IO uint16_t BRR;
    uint16_t      RESERVED2;
    __IO uint16_t CR1;
    uint16_t      RESERVED3;
    __IO uint16_t CR2;
    uint16_t      RESERVED4;
    __IO uint16_t CR3;
    uint16_t      RESERVED5;
    __IO uint16_t GTPR;
    uint16_t      RESERVED6;
} USART_TypeDef;

/* 外设基地址映射 */
#define PERIPH_BASE           (0x40000000UL)
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x10000UL)
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x20000UL)

#define TIM2_BASE             (APB1PERIPH_BASE + 0x0000UL)
#define TIM3_BASE             (APB1PERIPH_BASE + 0x0400UL)

#define AFIO_BASE             (APB2PERIPH_BASE + 0x0000UL)
#define EXTI_BASE             (APB2PERIPH_BASE + 0x0400UL)
#define GPIOA_BASE            (APB2PERIPH_BASE + 0x0800UL)
#define GPIOB_BASE            (APB2PERIPH_BASE + 0x0C00UL)
#define GPIOC_BASE            (APB2PERIPH_BASE + 0x1000UL)
#define USART1_BASE           (APB2PERIPH_BASE + 0x3800UL)

#define RCC_BASE              (AHBPERIPH_BASE + 0x1000UL)
#define FLASH_R_BASE          (AHBPERIPH_BASE + 0x2000UL)

#define TIM2                  ((TIM_TypeDef   *) TIM2_BASE)
#define TIM3                  ((TIM_TypeDef   *) TIM3_BASE)
#define AFIO                  ((AFIO_TypeDef  *) AFIO_BASE)
#define EXTI                  ((EXTI_TypeDef  *) EXTI_BASE)
#define GPIOA                 ((GPIO_TypeDef  *) GPIOA_BASE)
#define GPIOB                 ((GPIO_TypeDef  *) GPIOB_BASE)
#define GPIOC                 ((GPIO_TypeDef  *) GPIOC_BASE)
#define USART1                ((USART_TypeDef *) USART1_BASE)
#define RCC                   ((RCC_TypeDef   *) RCC_BASE)
#define FLASH                 ((FLASH_TypeDef *) FLASH_R_BASE)

/* 常用 RCC 时钟使能位宏定义 */
#define RCC_APB2ENR_AFIOEN    (1UL << 0)
#define RCC_APB2ENR_IOPAEN    (1UL << 2)
#define RCC_APB2ENR_IOPBEN    (1UL << 3)
#define RCC_APB2ENR_IOPCEN    (1UL << 4)
#define RCC_APB2ENR_USART1EN  (1UL << 14)
#define RCC_APB1ENR_TIM2EN    (1UL << 0)
#define RCC_APB1ENR_TIM3EN    (1UL << 1)
#define RCC_APB1ENR_BKPEN     (1UL << 27)
#define RCC_APB1ENR_PWREN     (1UL << 28)

/* RCC->CSR 复位标志位宏定义 (用于识别板载 RESET 按键点击) */
#define RCC_CSR_RMVF          (1UL << 24) /*!< 清除复位标志位       */
#define RCC_CSR_PINRSTF       (1UL << 26) /*!< NRST 引脚复位标志位  */
#define RCC_CSR_PORRSTF       (1UL << 27) /*!< 上电/掉电复位标志位  */
#define RCC_CSR_SFTRSTF       (1UL << 28) /*!< 软件复位标志位       */

/* PWR 电源控制与 BKP 备份寄存器 (用于在按下板载 RESET 键跨复位保存启停状态) */
#define PWR_CR_REG            (*(__IO uint32_t *)(APB1PERIPH_BASE + 0x7000UL))
#define PWR_CR_DBP            (1UL << 8)  /*!< 允许访问 BKP 备份域寄存器 */
#define BKP_DR1_REG           (*(__IO uint16_t *)(APB1PERIPH_BASE + 0x6C04UL))

/* USART 状态位与控制位 */
#define USART_SR_RXNE         (1U << 5)
#define USART_SR_TC           (1U << 6)
#define USART_SR_TXE          (1U << 7)
#define USART_CR1_RE          (1U << 2)
#define USART_CR1_TE          (1U << 3)
#define USART_CR1_RXNEIE      (1U << 5)
#define USART_CR1_UE          (1U << 13)

#ifdef __cplusplus
}
#endif

#endif /* __STM32F10x_H */
