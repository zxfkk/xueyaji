.syntax unified
.cpu cortex-m3
.thumb
.section .isr_vector,"a",%progbits
.global vectors
vectors:
.word _estack,Reset_Handler,NMI_Handler,HardFault_Handler
.word MemManage_Handler,BusFault_Handler,UsageFault_Handler,0,0,0,0
.word SVC_Handler,DebugMon_Handler,0,PendSV_Handler,SysTick_Handler
/* IRQs 0..42, matching the Keil startup for STM32F103 medium density. */
.rept 6
.word Default_Handler
.endr
.word EXTI0_IRQHandler
.rept 3
.word Default_Handler
.endr
.word EXTI4_IRQHandler
.rept 26
.word Default_Handler
.endr
.word USART1_IRQHandler
.rept 5
.word Default_Handler
.endr
.section .text.Reset_Handler,"ax",%progbits
.global Reset_Handler
.thumb_func
Reset_Handler:
 ldr r0,=_sidata
 ldr r1,=_sdata
 ldr r2,=_edata
1:
 cmp r1,r2
 bcs 2f
 ldr r3,[r0],#4
 str r3,[r1],#4
 b 1b
2:
 ldr r1,=_sbss
 ldr r2,=_ebss
 movs r3,#0
3:
 cmp r1,r2
 bcs 4f
 str r3,[r1],#4
 b 3b
4:
 bl SystemInit
 bl main
 b Default_Handler
.thumb_func
Default_Handler:
 b .
.macro weak_handler name
 .weak \name
 .thumb_set \name,Default_Handler
.endm
weak_handler NMI_Handler
weak_handler SVC_Handler
weak_handler DebugMon_Handler
weak_handler PendSV_Handler
