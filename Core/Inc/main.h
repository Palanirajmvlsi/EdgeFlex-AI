#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void Error_Handler(void);

/* STM32F411CEU6 Black Pill hardware pin map */
#define LED_PIN             GPIO_PIN_13
#define LED_GPIO_PORT       GPIOC
#define USART1_TX_PIN       GPIO_PIN_9
#define USART1_RX_PIN       GPIO_PIN_10
#define USART1_GPIO_PORT    GPIOA

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
