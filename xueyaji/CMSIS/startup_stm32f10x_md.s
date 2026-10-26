;******************** (C) COPYRIGHT STMicroelectronics ********************
;* File Name          : startup_stm32f10x_md.s
;* Description        : STM32F103x8/B (Medium Density, 64KB Flash / 20KB RAM)
;*                      Keil MDK-ARM vector table and Reset_Handler
;******************************************************************************

Stack_Size      EQU     0x00001000

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

Heap_Size       EQU     0x00000200

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                PRESERVE8
                THUMB

; Vector Table Mapped to Address 0 at Reset
                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp               ; Top of Stack
                DCD     Reset_Handler              ; Reset Handler
                DCD     NMI_Handler                ; NMI Handler
                DCD     HardFault_Handler          ; Hard Fault Handler
                DCD     MemManage_Handler          ; MPU Fault Handler
                DCD     BusFault_Handler           ; Bus Fault Handler
                DCD     UsageFault_Handler         ; Usage Fault Handler
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     SVC_Handler                ; SVCall Handler
                DCD     DebugMon_Handler           ; Debug Monitor Handler
                DCD     0                          ; Reserved
                DCD     PendSV_Handler             ; PendSV Handler
                DCD     SysTick_Handler            ; SysTick Handler

                ; External Interrupts
                DCD     Default_Handler            ; Window Watchdog
                DCD     Default_Handler            ; PVD through EXTI Line detect
                DCD     Default_Handler            ; Tamper
                DCD     Default_Handler            ; RTC
                DCD     Default_Handler            ; Flash
                DCD     Default_Handler            ; RCC
                DCD     EXTI0_IRQHandler           ; EXTI Line 0 (PA0 Key)
                DCD     Default_Handler            ; EXTI Line 1
                DCD     Default_Handler            ; EXTI Line 2
                DCD     Default_Handler            ; EXTI Line 3
                DCD     EXTI4_IRQHandler           ; EXTI Line 4 (PA4 Key)
                DCD     Default_Handler            ; DMA1 Channel 1
                DCD     Default_Handler            ; DMA1 Channel 2
                DCD     Default_Handler            ; DMA1 Channel 3
                DCD     Default_Handler            ; DMA1 Channel 4
                DCD     Default_Handler            ; DMA1 Channel 5
                DCD     Default_Handler            ; DMA1 Channel 6
                DCD     Default_Handler            ; DMA1 Channel 7
                DCD     Default_Handler            ; ADC1 & ADC2
                DCD     Default_Handler            ; USB High Priority or CAN1 TX
                DCD     Default_Handler            ; USB Low  Priority or CAN1 RX0
                DCD     Default_Handler            ; CAN1 RX1
                DCD     Default_Handler            ; CAN1 SCE
                DCD     Default_Handler            ; EXTI Line 9..5
                DCD     Default_Handler            ; TIM1 Break
                DCD     Default_Handler            ; TIM1 Update
                DCD     Default_Handler            ; TIM1 Trigger and Commutation
                DCD     Default_Handler            ; TIM1 Capture Compare
                DCD     Default_Handler            ; TIM2
                DCD     Default_Handler            ; TIM3 (PA7 PWM)
                DCD     Default_Handler            ; TIM4
                DCD     Default_Handler            ; I2C1 Event
                DCD     Default_Handler            ; I2C1 Error
                DCD     Default_Handler            ; I2C2 Event
                DCD     Default_Handler            ; I2C2 Error
                DCD     Default_Handler            ; SPI1
                DCD     Default_Handler            ; SPI2
                DCD     USART1_IRQHandler          ; USART1 (PA9/PA10 HC-05 Bluetooth)
                DCD     Default_Handler            ; USART2
                DCD     Default_Handler            ; USART3
                DCD     Default_Handler            ; EXTI Line 15..10
                DCD     Default_Handler            ; RTCAlarm through EXTI Line
                DCD     Default_Handler            ; USBWakeup from suspend
__Vectors_End

__Vectors_Size  EQU  __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY

; Reset handler
Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  __main
                IMPORT  SystemInit
                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP

; Dummy Exception Handlers
NMI_Handler     PROC
                EXPORT  NMI_Handler               [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler         [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler         [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler          [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler        [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler               [WEAK]
                B       .
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler          [WEAK]
                B       .
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler            [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler           [WEAK]
                B       .
                ENDP

Default_Handler PROC
                EXPORT  EXTI0_IRQHandler          [WEAK]
                EXPORT  EXTI4_IRQHandler          [WEAK]
                EXPORT  USART1_IRQHandler         [WEAK]
EXTI0_IRQHandler
EXTI4_IRQHandler
USART1_IRQHandler
                B       .
                ENDP

                ALIGN

;*******************************************************************************
; User Stack and Heap initialization
;*******************************************************************************
                 IF      :DEF:__MICROLIB
                
                 EXPORT  __initial_sp
                 EXPORT  __heap_base
                 EXPORT  __heap_limit
                
                 ELSE
                
                 IMPORT  __use_two_region_memory
                 EXPORT  __user_initial_stackheap
                 
__user_initial_stackheap

                 LDR     R0, =  Heap_Mem
                 LDR     R1, =(Stack_Mem + Stack_Size)
                 LDR     R2, = (Heap_Mem +  Heap_Size)
                 LDR     R3, = Stack_Mem
                 BX      LR

                 ALIGN

                 ENDIF

                 END
