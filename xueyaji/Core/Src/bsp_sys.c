/* Blue Pill BSP: TIM2 microseconds, SysTick milliseconds, bounded main-context
 * I/O, watchdog and existing RESET/BKP start/stop convention. */
#include "bsp_sys.h"
#include "main.h"

#define BKP_MAGIC_IDLE       (0xA501U) /* 备份域标记: 当前处于待机/结果页，下次按 RESET 立即启动测量 */
#define BKP_MAGIC_MEASURING  (0xA502U) /* 备份域标记: 当前正在充气/测量，若按 RESET 则视为紧急停机 */

static volatile uint32_t s_sys_tick_ms      = 0;
static bool              s_reset_key_start  = false;

/*============================================================================
 * 1. SysTick 1ms 滴答定时器与微秒延时
 *============================================================================*/
void SysTick_Handler(void) {
    s_sys_tick_ms++;
}

uint32_t HAL_GetTick(void) {
    return s_sys_tick_ms;
}

void HAL_Delay(uint32_t Delay_ms) {
    uint32_t start = s_sys_tick_ms;
    while ((s_sys_tick_ms - start) < Delay_ms) {
        __NOP();
    }
}

/* TIM2 free-runs at 1MHz. Extend its 16-bit count in the main context. */
uint32_t BSP_GetMicros(void) {
    static uint16_t last;
    static uint32_t elapsed;
    uint16_t count=TIM2->CNT;
    elapsed+=(uint16_t)(count-last); last=count;
    return elapsed;
}
void BSP_DelayUs(uint32_t us) {
    while (us) {
        uint16_t chunk=(uint16_t)((us>30000UL)?30000UL:us);
        uint16_t start=TIM2->CNT;
        while ((uint16_t)(TIM2->CNT-start)<chunk) { __NOP(); }
        us-=chunk;
    }
}
#define IWDG_KR (*(__IO uint32_t *)0x40003000UL)
#define IWDG_PR (*(__IO uint32_t *)0x40003004UL)
#define IWDG_RLR (*(__IO uint32_t *)0x40003008UL)
#define IWDG_SR (*(__IO uint32_t *)0x4000300CUL)
void BSP_WatchdogFeed(void) { IWDG_KR=0xAAAAUL; }

/*============================================================================
 * 2. 板载 RESET 键智能启停检测 + PA0/PA4 外接按键 + PC13 状态灯初始化
 *============================================================================*/
void BSP_SetMeasuringFlag(bool is_measuring) {
    BKP_DR1_REG = is_measuring ? BKP_MAGIC_MEASURING : BKP_MAGIC_IDLE;
}

bool BSP_CheckResetKeyStart(void) {
    return s_reset_key_start;
}

void BSP_Sys_Init(void) {
    SystemCoreClockUpdate();
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC=(uint16_t)(SystemCoreClock/1000000UL-1UL);
    TIM2->ARR=0xFFFFU; TIM2->EGR=1U; TIM2->CR1=1U;
    /* Start watchdog before device discovery; ~1s at nominal 40kHz LSI. */
    IWDG_KR=0xCCCCUL; IWDG_KR=0x5555UL;
    while (IWDG_SR) { }
    IWDG_PR=4U; IWDG_RLR=625U; BSP_WatchdogFeed();
    /* 1. 配置 SysTick 为 1ms 中断一次 */
    SysTick_Config(SystemCoreClock / 1000UL);

    /* 2. 开启 GPIOA, GPIOB, GPIOC, AFIO 以及 PWR/BKP 备份域时钟 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |
                    RCC_APB2ENR_IOPBEN |
                    RCC_APB2ENR_IOPCEN |
                    RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR_CR_REG   |= PWR_CR_DBP; /* 解锁 BKP 备份寄存器写保护 */

    /* 3. 检测是否为用户按下了核心板自带的 RESET 物理按键:
     *    ★关键防抖设计: 在此处 (t=1ms) 仅读取 BKP_DR1_REG，绝不立刻改写 BKP_DR1_REG！
     *    必须等 main() 中延时 250ms (手指彻底松开、按键弹片机械抖动完全结束) 后再改写 BKP_DR1_REG，
     *    从而 100% 杜绝松开按键时的机械抖动二次复位把“停机泄气”又误翻回“启动充气”！
     */
    uint32_t csr = RCC->CSR;
    RCC->CSR |= RCC_CSR_RMVF; /* 清除复位标志位，供下一次按键检测 */

    if ((csr & RCC_CSR_PINRSTF) && !(csr & (RCC_CSR_PORRSTF | RCC_CSR_SFTRSTF | (3UL << 29)))) {
        if (BKP_DR1_REG == BKP_MAGIC_IDLE) {
            s_reset_key_start = true;
        } else {
            /* 测量途中按下 RESET 键 -> 紧急泄气停机回待机态 */
            s_reset_key_start = false;
        }
    } else {
        /* 首次上电或刚下载完程序 -> 默认进入待机态 */
        s_reset_key_start = false;
        BKP_DR1_REG       = BKP_MAGIC_IDLE;
    }

    /* 4. 配置 PC13 (核心板板载 LED: VCC3.3V -> LED -> PC13)
     *    严格遵循数据手册第20页注4: PC13 只能工作在 2MHz 模式 (0b0010)，低电平点亮
     */
    GPIOC->CRH &= ~(0xFUL << 20);
    GPIOC->CRH |=  (0x2UL << 20);
    BSP_LED_Set(true);

    /* 5. 配置 PA0 (A0) 与 PA4 (A4) 为内部上拉输入:
     *    ★关键抗干扰设计: 关闭 EXTI0 / EXTI4 硬件边沿中断!
     *    原因: 悬空排针 PA0/PA4 紧邻气泵 PWM 引脚 PA7，气泵开启瞬间的高频电磁尖峰极易误触发 EXTI 中断，
     *    导致刚启动充气就被误判为“按下急停”或引起时间戳下溢秒报 Cuff Leak!
     */
    GPIOA->CRL &= ~((0xFUL << 0) | (0xFUL << 16));
    GPIOA->CRL |=  ((0x8UL << 0) | (0x8UL << 16));
    GPIOA->ODR |=  ((1UL << 0)   | (1UL << 4)); /* 启用内部 40kOhm 上拉电阻 */

    EXTI->IMR &= ~((1UL << 0) | (1UL << 4));
    NVIC_DisableIRQ(EXTI0_IRQn);
    NVIC_DisableIRQ(EXTI4_IRQn);
}

/* 核心板自带 KEY0 (PA0) 按键中断服务函数 */
void EXTI0_IRQHandler(void) { EXTI->PR=(1UL<<0); }
void EXTI4_IRQHandler(void) { EXTI->PR=(1UL<<4); }

void BSP_LED_Set(bool on) {
    /* 原理图 LED2 阳极接 VCC3.3V，阴极经 R7(1K) 接 PC13: 低电平亮，高电平灭 */
    if (on) {
        GPIOC->BRR  = (1UL << 13);
    } else {
        GPIOC->BSRR = (1UL << 13);
    }
}

void BSP_LED_Toggle(void) {
    GPIOC->ODR ^= (1UL << 13);
}

/*============================================================================
 * 3. USART1 (PA9=TX, PA10=RX) HC-05 蓝牙串口驱动 (9600 8N1)
 *============================================================================*/
void BSP_UART1_Init(uint32_t baudrate) {
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN;

    /* PA9 (USART1_TX): 复用推挽输出 50MHz (CRH[7:4] = 0b1011) */
    /* PA10(USART1_RX): 内部上拉输入       (CRH[11:8]= 0b1000, ODR[10]=1) */
    GPIOA->CRH &= ~((0xFUL << 4) | (0xFUL << 8));
    GPIOA->CRH |=  ((0xBUL << 4) | (0x8UL << 8));
    GPIOA->ODR |=  (1UL << 10);

    USART1->BRR = (uint16_t)((SystemCoreClock + (baudrate / 2U)) / baudrate);
    /* 仅开启串口发送与接收，关闭 RXNE 中断以防 PA10 悬空时受气泵电磁干扰误触发启停 */
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    NVIC_DisableIRQ(USART1_IRQn);
}

/* Main-context producer/consumer: no interrupt mutates app state. */
static uint8_t tx_buf[256];
static uint16_t tx_head,tx_tail;
void BSP_UART1_SendString(const char *str) {
    uint16_t len=0,free_space=(uint16_t)((tx_tail-tx_head-1U)&255U);
    if (!str) return;
    while (str[len] && len<255U) len++;
    if (str[len] || len>free_space) return; /* Drop whole diagnostic message. */
    while (len--) { tx_buf[tx_head]=(uint8_t)*str++; tx_head=(tx_head+1U)&255U; }
}
void BSP_IO_Service(void) {
    static uint32_t down_since;
    static bool key_down,key_latched;
    uint32_t now=HAL_GetTick();
    bool down=(GPIOA->IDR&((1UL<<0)|(1UL<<4)))!=((1UL<<0)|(1UL<<4));
    if (!down) { key_down=false; key_latched=false; }
    else if (!key_down) { key_down=true; down_since=now; }
    else if (!key_latched && now-down_since>=30UL) {
        key_latched=true; App_RequestToggleStartStop();
    }
    if (USART1->SR & USART_SR_RXNE) {
        uint16_t status=USART1->SR;
        uint8_t ch=(uint8_t)USART1->DR;
        if (!(status&0x0FU)) {
            if (ch=='S' || ch=='s') App_RequestStart();
            if (ch=='E' || ch=='e') App_RequestStop();
        }
    } else if (USART1->SR&0x0FU) { (void)USART1->DR; }
    if (tx_head!=tx_tail && (USART1->SR&USART_SR_TXE)) {
        USART1->DR=tx_buf[tx_tail]; tx_tail=(tx_tail+1U)&255U;
    }
}
void USART1_IRQHandler(void) { /* UART is polled; NVIC IRQ stays disabled. */ }

/*============================================================================
 * 4. Bounded software I2C with bus-idle verification and recovery clocks
 *============================================================================*/
static uint16_t GetPinSCL(I2C_Bus_t bus) {
    return (bus == I2C_BUS_1) ? (1U << 6) : (1U << 10);
}

static uint16_t GetPinSDA(I2C_Bus_t bus) {
    return (bus == I2C_BUS_1) ? (1U << 7) : (1U << 11);
}

static void I2C_SCL_H(I2C_Bus_t bus) { GPIOB->BSRR = GetPinSCL(bus); }
static void I2C_SCL_L(I2C_Bus_t bus) { GPIOB->BRR  = GetPinSCL(bus); }

/* Release SDA instead of ever driving it high against a slave. */
static void I2C_SDA_ReleasePullUp(I2C_Bus_t bus) {
    if (bus == I2C_BUS_1) {
        GPIOB->BSRR = (1UL << 7);
        GPIOB->CRL  = (GPIOB->CRL & ~(0xFUL << 28)) | (0x8UL << 28);
        GPIOB->BSRR = (1UL << 7);
    } else {
        GPIOB->BSRR = (1UL << 11);
        GPIOB->CRH  = (GPIOB->CRH & ~(0xFUL << 12)) | (0x8UL << 12);
        GPIOB->BSRR = (1UL << 11);
    }
}

static void I2C_SDA_H(I2C_Bus_t bus) { I2C_SDA_ReleasePullUp(bus); }

static void I2C_SDA_L(I2C_Bus_t bus) {
    if (bus == I2C_BUS_1) {
        GPIOB->BRR = (1UL << 7);
        GPIOB->CRL = (GPIOB->CRL & ~(0xFUL << 28)) | (0x7UL << 28);
    } else {
        GPIOB->BRR = (1UL << 11);
        GPIOB->CRH = (GPIOB->CRH & ~(0xFUL << 12)) | (0x7UL << 12);
    }
}

static bool I2C_SDA_Read(I2C_Bus_t bus) {
    return ((GPIOB->IDR & GetPinSDA(bus)) != 0U);
}

/* I2C 总线防死锁恢复: 发送 16 个 SCL 脉冲强制从机移出残余位并释放 SDA 总线 */
static void I2C_BusUnlock(I2C_Bus_t bus) {
    I2C_SDA_ReleasePullUp(bus);
    for (uint8_t i = 0; i < 16U; i++) {
        I2C_SCL_L(bus);
        BSP_DelayUs(5);
        I2C_SCL_H(bus);
        BSP_DelayUs(5);
    }
    /* 发送标准 STOP 停止位复位所有 I2C 从机状态机 */
    I2C_SCL_L(bus);
    I2C_SDA_L(bus);
    BSP_DelayUs(5);
    I2C_SCL_H(bus);
    BSP_DelayUs(5);
    I2C_SDA_H(bus);
    BSP_DelayUs(5);
}

/* A stuck-low wire must not masquerade as ACK + zero-valued pressure. */
static bool I2C_BusReady(I2C_Bus_t bus) {
    uint16_t mask=GetPinSCL(bus)|GetPinSDA(bus);
    I2C_SDA_H(bus); I2C_SCL_H(bus); BSP_DelayUs(10);
    if ((GPIOB->IDR&mask)==mask) return true;
    I2C_BusUnlock(bus); BSP_DelayUs(10);
    return (GPIOB->IDR&mask)==mask;
}

void BSP_I2C_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    /* PB6(SCL1) 推挽输出, PB7(SDA1) 内部上拉输入(高电平态) */
    GPIOB->BSRR = (1UL << 6) | (1UL << 7) | (1UL << 10) | (1UL << 11);
    GPIOB->CRL &= ~((0xFUL << 24) | (0xFUL << 28));
    GPIOB->CRL |=  ((0x3UL << 24) | (0x8UL << 28));

    /* PB10(SCL2) 推挽输出, PB11(SDA2) 内部上拉输入(高电平态) */
    GPIOB->CRH &= ~((0xFUL << 8) | (0xFUL << 12));
    GPIOB->CRH |=  ((0x3UL << 8) | (0x8UL << 12));

    /* 对两路 I2C 通道执行 16 时钟脉冲解锁 */
    I2C_BusUnlock(I2C_BUS_1);
    I2C_BusUnlock(I2C_BUS_2);
}

static void I2C_Start(I2C_Bus_t bus) {
    I2C_SDA_H(bus);
    BSP_DelayUs(5);
    I2C_SCL_H(bus);
    BSP_DelayUs(5);
    I2C_SDA_L(bus);
    BSP_DelayUs(5);
    I2C_SCL_L(bus);
    BSP_DelayUs(2);
}

static void I2C_Stop(I2C_Bus_t bus) {
    I2C_SCL_L(bus);
    I2C_SDA_L(bus);
    BSP_DelayUs(5);
    I2C_SCL_H(bus);
    BSP_DelayUs(5);
    I2C_SDA_H(bus);
    BSP_DelayUs(5);
}

static bool I2C_WriteByte(I2C_Bus_t bus, uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        if (byte & 0x80U) {
            I2C_SDA_H(bus);
        } else {
            I2C_SDA_L(bus);
        }
        byte <<= 1;
        BSP_DelayUs(3);
        I2C_SCL_H(bus);
        BSP_DelayUs(4);
        I2C_SCL_L(bus);
        BSP_DelayUs(1);
    }
    /* 第8个时钟下降沿后立即切为纯内部上拉输入 (绝不输出推挽高电平顶死传感器的 ACK)，
     * 并等待 10us 让真传感器稳稳拉低 SDA，同时若无传感器则 40k 上拉充饱至 3.3V 消除假 ACK!
     */
    I2C_SDA_ReleasePullUp(bus);
    BSP_DelayUs(10);
    I2C_SCL_H(bus);
    BSP_DelayUs(5);
    bool ack = !I2C_SDA_Read(bus);
    I2C_SCL_L(bus);
    BSP_DelayUs(2);
    return ack;
}

static uint8_t I2C_ReadByte(I2C_Bus_t bus, bool send_ack) {
    uint8_t byte = 0;
    I2C_SDA_ReleasePullUp(bus);
    for (uint8_t i = 0; i < 8; i++) {
        byte <<= 1;
        BSP_DelayUs(8);
        I2C_SCL_H(bus);
        BSP_DelayUs(5);
        if (I2C_SDA_Read(bus)) {
            byte |= 0x01U;
        }
        I2C_SCL_L(bus);
        BSP_DelayUs(2);
    }
    if (send_ack) {
        I2C_SDA_L(bus);
    } else {
        I2C_SDA_H(bus);
    }
    BSP_DelayUs(3);
    I2C_SCL_H(bus);
    BSP_DelayUs(4);
    I2C_SCL_L(bus);
    BSP_DelayUs(2);
    I2C_SDA_ReleasePullUp(bus);
    return byte;
}

bool BSP_I2C_CheckDevice(I2C_Bus_t bus, uint8_t dev_addr_8bit) {
    if (!I2C_BusReady(bus)) return false;
    I2C_Start(bus);
    bool ack = I2C_WriteByte(bus, dev_addr_8bit & 0xFEU);
    I2C_Stop(bus);
    return ack;
}

bool BSP_I2C_WriteReg(I2C_Bus_t bus, uint8_t dev_addr_8bit, uint8_t reg, uint8_t data) {
    if (!I2C_BusReady(bus)) return false;
    I2C_Start(bus);
    if (!I2C_WriteByte(bus, dev_addr_8bit & 0xFEU) ||
        !I2C_WriteByte(bus, reg) ||
        !I2C_WriteByte(bus, data)) {
        I2C_Stop(bus);
        return false;
    }
    I2C_Stop(bus);
    return true;
}

bool BSP_I2C_WriteMulti(I2C_Bus_t bus, uint8_t dev_addr_8bit, uint8_t reg, const uint8_t *buf, uint16_t len) {
    if (!I2C_BusReady(bus)) return false;
    I2C_Start(bus);
    if (!I2C_WriteByte(bus, dev_addr_8bit & 0xFEU) || !I2C_WriteByte(bus, reg)) {
        I2C_Stop(bus);
        return false;
    }
    for (uint16_t i = 0; i < len; i++) {
        if (!I2C_WriteByte(bus, buf[i])) {
            I2C_Stop(bus);
            return false;
        }
    }
    I2C_Stop(bus);
    return true;
}

bool BSP_I2C_ReadRegs(I2C_Bus_t bus, uint8_t dev_addr_8bit, uint8_t reg, uint8_t *buf, uint16_t len) {
    if (!I2C_BusReady(bus)) return false;
    I2C_Start(bus);
    if (!I2C_WriteByte(bus, dev_addr_8bit & 0xFEU) || !I2C_WriteByte(bus, reg)) {
        I2C_Stop(bus);
        return false;
    }
    /* 优先使用标准 Repeated Start (Sr) 读取寄存器，若未应答则自动兼容 Stop + Start 模式 */
    I2C_Start(bus);
    if (!I2C_WriteByte(bus, dev_addr_8bit | 0x01U)) {
        I2C_Stop(bus);
        BSP_DelayUs(5);
        I2C_Start(bus);
        if (!I2C_WriteByte(bus, dev_addr_8bit | 0x01U)) {
            I2C_Stop(bus);
            return false;
        }
    }
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = I2C_ReadByte(bus, (i + 1U) < len);
    }
    I2C_Stop(bus);
    return true;
}
