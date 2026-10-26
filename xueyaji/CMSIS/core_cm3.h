/**
  ******************************************************************************
  * @file    core_cm3.h
  * @brief   CMSIS Cortex-M3 核心寄存器与内联指令头文件 (适配 Keil MDK ARMCC/AC6)
  ******************************************************************************
  */
#ifndef __CORE_CM3_H_GENERIC
#define __CORE_CM3_H_GENERIC

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define __I     volatile const   /*!< 只读权限 */
#define __O     volatile         /*!< 只写权限 */
#define __IO    volatile         /*!< 读写权限 */

/* SysTick 寄存器结构体 (0xE000E010) */
typedef struct {
    __IO uint32_t CTRL;          /*!< Offset: 0x00  SysTick 控制与状态寄存器 */
    __IO uint32_t LOAD;          /*!< Offset: 0x04  SysTick 重装载值寄存器   */
    __IO uint32_t VAL;           /*!< Offset: 0x08  SysTick 当前值寄存器     */
    __I  uint32_t CALIB;         /*!< Offset: 0x0C  SysTick 校准寄存器       */
} SysTick_Type;

/* NVIC 寄存器结构体 (0xE000E100) */
typedef struct {
    __IO uint32_t ISER[8U];      /*!< Offset: 0x000 中断使能设置寄存器       */
         uint32_t RESERVED0[24U];
    __IO uint32_t ICER[8U];      /*!< Offset: 0x080 中断使能清除寄存器       */
         uint32_t RESERVED1[24U];
    __IO uint32_t ISPR[8U];      /*!< Offset: 0x100 中断挂起设置寄存器       */
         uint32_t RESERVED2[24U];
    __IO uint32_t ICPR[8U];      /*!< Offset: 0x180 中断挂起清除寄存器       */
         uint32_t RESERVED3[24U];
    __IO uint32_t IABR[8U];      /*!< Offset: 0x200 中断活动位寄存器         */
         uint32_t RESERVED4[56U];
    __IO uint8_t  IP[240U];      /*!< Offset: 0x300 中断优先级寄存器 (8Bit)  */
         uint32_t RESERVED5[644U];
    __O  uint32_t STIR;          /*!< Offset: 0xE00 软件触发中断寄存器       */
} NVIC_Type;

/* SCB 系统控制块寄存器结构体 (0xE000ED00) */
typedef struct {
    __I  uint32_t CPUID;         /*!< Offset: 0x00  CPUID 基址寄存器         */
    __IO uint32_t ICSR;          /*!< Offset: 0x04  中断控制及状态寄存器     */
    __IO uint32_t VTOR;          /*!< Offset: 0x08  向量表偏移量寄存器       */
    __IO uint32_t AIRCR;         /*!< Offset: 0x0C  应用程序中断及复位控制   */
    __IO uint32_t SCR;           /*!< Offset: 0x10  系统控制寄存器           */
    __IO uint32_t CCR;           /*!< Offset: 0x14  配置与控制寄存器         */
    __IO uint8_t  SHP[12U];      /*!< Offset: 0x18  系统异常优先级寄存器     */
    __IO uint32_t SHCSR;         /*!< Offset: 0x24  系统Handler控制及状态    */
    __IO uint32_t CFSR;          /*!< Offset: 0x28  可配置故障状态寄存器     */
    __IO uint32_t HFSR;          /*!< Offset: 0x2C  硬故障状态寄存器         */
    __IO uint32_t DFSR;          /*!< Offset: 0x30  调试故障状态寄存器       */
    __IO uint32_t MMFAR;         /*!< Offset: 0x34  存储管理故障地址寄存器   */
    __IO uint32_t BFAR;          /*!< Offset: 0x38  总线故障地址寄存器       */
    __IO uint32_t AFSR;          /*!< Offset: 0x3C  辅助故障状态寄存器       */
} SCB_Type;

#define SCS_BASE            (0xE000E000UL)
#define SysTick_BASE        (SCS_BASE + 0x0010UL)
#define NVIC_BASE           (SCS_BASE + 0x0100UL)
#define SCB_BASE            (SCS_BASE + 0x0D00UL)

#define SCB                 ((SCB_Type     *) SCB_BASE)
#define SysTick             ((SysTick_Type *) SysTick_BASE)
#define NVIC                ((NVIC_Type    *) NVIC_BASE)

/* SysTick 控制位定义 */
#define SysTick_CTRL_COUNTFLAG_Msk  (1UL << 16U)
#define SysTick_CTRL_CLKSOURCE_Msk  (1UL << 2U)
#define SysTick_CTRL_TICKINT_Msk    (1UL << 1U)
#define SysTick_CTRL_ENABLE_Msk     (1UL << 0U)

/* NVIC 辅助内联函数 */
static __inline void NVIC_EnableIRQ(int32_t IRQn) {
    if (IRQn >= 0) {
        NVIC->ISER[(uint32_t)IRQn >> 5U] = (uint32_t)(1UL << ((uint32_t)IRQn & 0x1FUL));
    }
}

static __inline void NVIC_DisableIRQ(int32_t IRQn) {
    if (IRQn >= 0) {
        NVIC->ICER[(uint32_t)IRQn >> 5U] = (uint32_t)(1UL << ((uint32_t)IRQn & 0x1FUL));
    }
}

static __inline void NVIC_SetPriority(int32_t IRQn, uint32_t priority) {
    if (IRQn >= 0) {
        NVIC->IP[(uint32_t)IRQn] = (uint8_t)((priority << 4U) & 0xFFUL);
    } else {
        SCB->SHP[(((uint32_t)IRQn) & 0xFUL) - 4UL] = (uint8_t)((priority << 4U) & 0xFFUL);
    }
}

static __inline uint32_t SysTick_Config(uint32_t ticks) {
    if ((ticks - 1UL) > 0xFFFFFFUL) return 1UL;
    SysTick->LOAD = (uint32_t)(ticks - 1UL);
    NVIC_SetPriority(-1, 15U);
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;
    return 0UL;
}

#if defined(__CC_ARM)
  #define __NOP() __nop()
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #define __NOP() __builtin_arm_nop()
#elif defined(__GNUC__)
  #define __NOP() __asm volatile ("nop")
#else
  #define __NOP() do {} while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CORE_CM3_H_GENERIC */
