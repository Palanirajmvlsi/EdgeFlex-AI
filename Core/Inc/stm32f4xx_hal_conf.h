/**
 * @file stm32f4xx_hal_conf.h
 * @brief Minimal HAL configuration for EdgeFlex AI on STM32F401CCU6.
 *        Deliberately enables ONLY the modules the runtime demo needs
 *        (RCC, GPIO, UART, CORTEX, FLASH, PWR, DMA-base) to keep the
 *        build small and the dependency surface honest - see project
 *        spec section 15 ("don't add unnecessary features").
 */
#ifndef STM32F4xx_HAL_CONF_H
#define STM32F4xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED

#if !defined(HSE_VALUE)
#define HSE_VALUE 25000000U /* Black Pill HSE crystal (unused - we run on HSI) */
#endif

#if !defined(HSI_VALUE)
#define HSI_VALUE 16000000U
#endif

#if !defined(EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE 12288000U /* I2S external clock, unused by this MVP */
#endif

#define VDD_VALUE 3300U
#define HSE_STARTUP_TIMEOUT 100U
#define LSE_STARTUP_TIMEOUT 5000U
/* NOTE: LSI_STARTUP_TIME is already defined by stm32f401xc.h - don't
 * redefine it here (was producing a harmless-but-avoidable redefinition
 * warning on every translation unit; removed to keep the build at 0
 * avoidable warnings per the project's build-quality bar). */
#define TICK_INT_PRIORITY 0x0FU
#define USE_RTOS 0U
#define PREFETCH_ENABLE 1U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE 1U
#define USE_HAL_UART_REGISTER_CALLBACKS 0U

#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_pwr.h"
#include "stm32f4xx_hal_uart.h"

#define assert_param(expr) ((void)0U)

#ifdef __cplusplus
}
#endif

#endif /* STM32F4xx_HAL_CONF_H */
