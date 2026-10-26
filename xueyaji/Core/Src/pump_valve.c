/**
  ******************************************************************************
  * @file    pump_valve.c
  * @brief   ZR370-03PM 气泵 TIM3_CH2 (PA7) 硬件 PWM 与 Fa0520C 电磁阀 (PB0) 驱动实现
  ******************************************************************************
  */
#include "pump_valve.h"

static uint8_t  s_curr_duty      = 0;
static uint8_t  s_target_duty    = 0;
static uint32_t s_bleed_pulse_us = 650UL;  /* Experimental pulse width, bench calibration required. */
static float s_prev_bleed_p;
static uint32_t s_feedback_at;

void Actuator_Init(void) {
    /* 1. 开启 GPIOA, GPIOB 与 TIM3 时钟 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* 2. 配置 PB0 (电磁阀 MOS 模块触发端) 为通用推挽输出 50MHz (CRL[3:0] = 0b0011) */
    GPIOB->CRL &= ~(0xFUL << 0);
    GPIOB->CRL |=  (0x3UL << 0);
    GPIOB->BRR  =  (1UL << 0); /* 默认输出低电平: 常开电磁阀失电全开，确保断电/待机绝对安全 */

    /* 3. 配置 PA7 (TIM3_CH2 气泵 MOS 模块 PWM 端) 为复用推挽输出 50MHz (CRL[31:28] = 0b1011) */
    GPIOA->CRL &= ~(0xFUL << 28);
    GPIOA->CRL |=  (0xBUL << 28);

    /* 4. 配置 TIM3 产生 20kHz 硬件 PWM (超出人耳听觉上限，消除电机高频啸叫):
     *    TIM3 时钟 = 72MHz, PSC = 36 - 1 -> 计数频率 2MHz
     *    ARR = 100 - 1 -> PWM 频率 = 2MHz / 100 = 20kHz, CCR2 范围 0~100 直接对应占空比百分比
     */
    TIM3->PSC   = (uint16_t)((SystemCoreClock / 2000000UL) - 1UL);
    TIM3->ARR   = 100U - 1U;
    TIM3->CCR2  = 0U;
    TIM3->CCMR1 &= ~(0xFFU << 8);
    TIM3->CCMR1 |=  (0x68U << 8); /* CH2 PWM 模式 1 (OC2M=110) + 预装载使能 (OC2PE=1) */
    TIM3->CCER  |=  (1U << 4);    /* 开启 CH2 输出 (CC2E=1) */
    TIM3->EGR = 1U; /* Load prescaler immediately. */
    TIM3->CR1   |=  (1U << 7) | (1U << 0); /* ARPE=1, CEN=1 启动定时器 */
}

void Pump_SetSpeed(uint8_t duty_percent) {
    if (duty_percent > 100U) duty_percent = 100U;
    s_curr_duty   = duty_percent;
    s_target_duty = duty_percent;
    TIM3->CCR2    = duty_percent;
}

void Pump_StartSoftRamp(uint8_t target_duty_percent) {
    if (target_duty_percent > 100U) target_duty_percent = 100U;
    s_target_duty = target_duty_percent;
    /* 从 20% 占空比柔和起步，平稳向袖带充气，避免窄四通接头产生动态背压虚高 */
    s_curr_duty   = (target_duty_percent > 20U) ? 20U : target_duty_percent;
    TIM3->CCR2    = s_curr_duty;
}

void Pump_UpdateSoftRamp(void) {
    if (s_curr_duty < s_target_duty) {
        s_curr_duty += 1U;
        if (s_curr_duty > s_target_duty) {
            s_curr_duty = s_target_duty;
        }
        TIM3->CCR2 = s_curr_duty;
    }
}

void Valve_Close(void) {
    /* PB0 置高电平 -> 电磁阀 MOS 模块导通 -> Fa0520C 通电吸合关闭气路 */
    GPIOB->BSRR = (1UL << 0);
}

void Valve_Open(void) {
    /* PB0 置低电平 -> 电磁阀 MOS 模块关断 -> Fa0520C 失电弹开快速泄气 */
    GPIOB->BRR = (1UL << 0);
}

void Valve_ResetSmoothBleed(float start_pressure_mmhg) {
    /* Retain the existing hardware pulse range; do not assume a valve travel time. */
    s_bleed_pulse_us = 600UL;
    s_prev_bleed_p = start_pressure_mmhg;
    s_feedback_at = HAL_GetTick();
}

uint32_t Valve_StepBleedAdaptive(float current_pressure_mmhg) {
    /* Tune using smoothed pressure and elapsed time, not the instantaneous pulse. */
    uint32_t now=HAL_GetTick(), elapsed=now-s_feedback_at;
    if (elapsed>=1200UL) {
        float rate=(s_prev_bleed_p-current_pressure_mmhg)*1000.0f/(float)elapsed;
        s_prev_bleed_p=current_pressure_mmhg; s_feedback_at=now;
        if (rate<0.5f) s_bleed_pulse_us+=400UL;
        else if (rate<2.5f) s_bleed_pulse_us+=120UL;
        else if (rate>4.5f && s_bleed_pulse_us>420UL) s_bleed_pulse_us-=120UL;
    }

    /* 随袖带气压降低给予微幅前馈补偿，并严格限制在 300us ~ 3200us (0.3~3.2ms) 微裂区 */
    uint32_t max_us = (current_pressure_mmhg > 100.0f) ? 2200UL : 3200UL;
    if (s_bleed_pulse_us < 300UL)  s_bleed_pulse_us = 300UL;
    if (s_bleed_pulse_us > max_us) s_bleed_pulse_us = max_us;

    Valve_Open();
    BSP_DelayUs(s_bleed_pulse_us);
    Valve_Close();

    return s_bleed_pulse_us;
}

void Emergency_Exhaust(void) {
    Pump_SetSpeed(0);
    Valve_Open();
}
