/*
 * EdgeFlex AI startup file - STM32F411xE, GNU Arm GCC
 * Cortex-M4 vector table and reset handler.
 */

.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb

.global g_pfnVectors
.global Reset_Handler

.extern SystemInit
.extern __libc_init_array
.extern main

.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object
.align 2

g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0
  .word 0
  .word 0
  .word 0
  .word SVC_Handler
  .word DebugMon_Handler
  .word 0
  .word PendSV_Handler
  .word SysTick_Handler

  .word WWDG_IRQHandler
  .word PVD_IRQHandler
  .word TAMP_STAMP_IRQHandler
  .word RTC_WKUP_IRQHandler
  .word FLASH_IRQHandler
  .word RCC_IRQHandler
  .word EXTI0_IRQHandler
  .word EXTI1_IRQHandler
  .word EXTI2_IRQHandler
  .word EXTI3_IRQHandler
  .word EXTI4_IRQHandler
  .word DMA1_Stream0_IRQHandler
  .word DMA1_Stream1_IRQHandler
  .word DMA1_Stream2_IRQHandler
  .word DMA1_Stream3_IRQHandler
  .word DMA1_Stream4_IRQHandler
  .word DMA1_Stream5_IRQHandler
  .word DMA1_Stream6_IRQHandler
  .word ADC_IRQHandler
  .word 0
  .word 0
  .word 0
  .word 0
  .word EXTI9_5_IRQHandler
  .word TIM1_BRK_TIM9_IRQHandler
  .word TIM1_UP_TIM10_IRQHandler
  .word TIM1_TRG_COM_TIM11_IRQHandler
  .word TIM1_CC_IRQHandler
  .word TIM2_IRQHandler
  .word TIM3_IRQHandler
  .word TIM4_IRQHandler
  .word I2C1_EV_IRQHandler
  .word I2C1_ER_IRQHandler
  .word I2C2_EV_IRQHandler
  .word I2C2_ER_IRQHandler
  .word SPI1_IRQHandler
  .word SPI2_IRQHandler
  .word USART1_IRQHandler
  .word USART2_IRQHandler
  .word 0
  .word EXTI15_10_IRQHandler
  .word RTC_Alarm_IRQHandler
  .word OTG_FS_WKUP_IRQHandler
  .word 0
  .word 0
  .word 0
  .word DMA1_Stream7_IRQHandler
  .word 0
  .word SDIO_IRQHandler
  .word TIM5_IRQHandler
  .word SPI3_IRQHandler
  .word 0
  .word 0
  .word 0
  .word 0
  .word DMA2_Stream0_IRQHandler
  .word DMA2_Stream1_IRQHandler
  .word DMA2_Stream2_IRQHandler
  .word DMA2_Stream3_IRQHandler
  .word DMA2_Stream4_IRQHandler
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word OTG_FS_IRQHandler
  .word DMA2_Stream5_IRQHandler
  .word DMA2_Stream6_IRQHandler
  .word DMA2_Stream7_IRQHandler
  .word USART6_IRQHandler
  .word I2C3_EV_IRQHandler
  .word I2C3_ER_IRQHandler
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word FPU_IRQHandler
  .word 0
  .word 0
  .word SPI4_IRQHandler
  .word SPI5_IRQHandler

.size g_pfnVectors, .-g_pfnVectors

.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
  ldr r0, =_estack
  mov sp, r0
  bl SystemInit
  bl __libc_init_array
  bl main
1:
  b 1b
.size Reset_Handler, .-Reset_Handler

/* Default handlers */
.section .text.Default_Handler,"ax",%progbits
.weak NMI_Handler,HardFault_Handler,MemManage_Handler,BusFault_Handler,UsageFault_Handler
.weak SVC_Handler,DebugMon_Handler,PendSV_Handler,SysTick_Handler
.weak WWDG_IRQHandler,PVD_IRQHandler,TAMP_STAMP_IRQHandler,RTC_WKUP_IRQHandler
.weak FLASH_IRQHandler,RCC_IRQHandler,EXTI0_IRQHandler,EXTI1_IRQHandler,EXTI2_IRQHandler
.weak EXTI3_IRQHandler,EXTI4_IRQHandler,DMA1_Stream0_IRQHandler,DMA1_Stream1_IRQHandler
.weak DMA1_Stream2_IRQHandler,DMA1_Stream3_IRQHandler,DMA1_Stream4_IRQHandler,DMA1_Stream5_IRQHandler
.weak DMA1_Stream6_IRQHandler,ADC_IRQHandler,EXTI9_5_IRQHandler,TIM1_BRK_TIM9_IRQHandler
.weak TIM1_UP_TIM10_IRQHandler,TIM1_TRG_COM_TIM11_IRQHandler,TIM1_CC_IRQHandler,TIM2_IRQHandler
.weak TIM3_IRQHandler,TIM4_IRQHandler,I2C1_EV_IRQHandler,I2C1_ER_IRQHandler,I2C2_EV_IRQHandler
.weak I2C2_ER_IRQHandler,SPI1_IRQHandler,SPI2_IRQHandler,USART1_IRQHandler,USART2_IRQHandler
.weak EXTI15_10_IRQHandler,RTC_Alarm_IRQHandler,OTG_FS_WKUP_IRQHandler,DMA1_Stream7_IRQHandler
.weak SDIO_IRQHandler,TIM5_IRQHandler,SPI3_IRQHandler,DMA2_Stream0_IRQHandler,DMA2_Stream1_IRQHandler
.weak DMA2_Stream2_IRQHandler,DMA2_Stream3_IRQHandler,DMA2_Stream4_IRQHandler,OTG_FS_IRQHandler
.weak DMA2_Stream5_IRQHandler,DMA2_Stream6_IRQHandler,DMA2_Stream7_IRQHandler,USART6_IRQHandler
.weak I2C3_EV_IRQHandler,I2C3_ER_IRQHandler,FPU_IRQHandler,SPI4_IRQHandler,SPI5_IRQHandler

NMI_Handler:
HardFault_Handler:
MemManage_Handler:
BusFault_Handler:
UsageFault_Handler:
SVC_Handler:
DebugMon_Handler:
PendSV_Handler:
SysTick_Handler:
WWDG_IRQHandler:
PVD_IRQHandler:
TAMP_STAMP_IRQHandler:
RTC_WKUP_IRQHandler:
FLASH_IRQHandler:
RCC_IRQHandler:
EXTI0_IRQHandler:
EXTI1_IRQHandler:
EXTI2_IRQHandler:
EXTI3_IRQHandler:
EXTI4_IRQHandler:
DMA1_Stream0_IRQHandler:
DMA1_Stream1_IRQHandler:
DMA1_Stream2_IRQHandler:
DMA1_Stream3_IRQHandler:
DMA1_Stream4_IRQHandler:
DMA1_Stream5_IRQHandler:
DMA1_Stream6_IRQHandler:
ADC_IRQHandler:
EXTI9_5_IRQHandler:
TIM1_BRK_TIM9_IRQHandler:
TIM1_UP_TIM10_IRQHandler:
TIM1_TRG_COM_TIM11_IRQHandler:
TIM1_CC_IRQHandler:
TIM2_IRQHandler:
TIM3_IRQHandler:
TIM4_IRQHandler:
I2C1_EV_IRQHandler:
I2C1_ER_IRQHandler:
I2C2_EV_IRQHandler:
I2C2_ER_IRQHandler:
SPI1_IRQHandler:
SPI2_IRQHandler:
USART1_IRQHandler:
USART2_IRQHandler:
EXTI15_10_IRQHandler:
RTC_Alarm_IRQHandler:
OTG_FS_WKUP_IRQHandler:
DMA1_Stream7_IRQHandler:
SDIO_IRQHandler:
TIM5_IRQHandler:
SPI3_IRQHandler:
DMA2_Stream0_IRQHandler:
DMA2_Stream1_IRQHandler:
DMA2_Stream2_IRQHandler:
DMA2_Stream3_IRQHandler:
DMA2_Stream4_IRQHandler:
OTG_FS_IRQHandler:
DMA2_Stream5_IRQHandler:
DMA2_Stream6_IRQHandler:
DMA2_Stream7_IRQHandler:
USART6_IRQHandler:
I2C3_EV_IRQHandler:
I2C3_ER_IRQHandler:
FPU_IRQHandler:
SPI4_IRQHandler:
SPI5_IRQHandler:
  b .
.size NMI_Handler, .-NMI_Handler
