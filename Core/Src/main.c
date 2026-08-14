/**
 ******************************************************************************
 * @file    main.c
 * @brief   EdgeFlex AI - STM32F401CCU6 (Black Pill) demo entry point.
 *
 * Boots the board, wires the Cortex-M4 DWT cycle counter into the
 * Performance Monitor as its (real, hardware) time source, runs one
 * inference of the shared TinyMLP workload through the EdgeFlex Dynamic
 * Memory Manager, and prints a report over USART1 (PA9/PA10, 115200 8N1).
 *
 * Every number printed here is read from a live struct at runtime - there
 * is no hardcoded/precomputed "expected" result baked into this file.
 ******************************************************************************
 */
#include "main.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "edgeflex_runtime.h"
#include "tinymlp_model.h"

UART_HandleTypeDef huart1;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void DWT_Init(void);
static uint32_t dwt_get_cycles(void);
static void uart_print(const char *s);
static void uart_printf(const char *fmt, ...);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    DWT_Init();

    /* Real hardware tick source: DWT cycle counter running at SystemCoreClock. */
    edgeflex_monitor_set_time_source(dwt_get_cycles, SystemCoreClock / 1000U);

    uart_print("\r\n\r\n");
    uart_print("=========================================\r\n");
    uart_print("     EdgeFlex AI Runtime - STM32F401CCU6  \r\n");
    uart_print("=========================================\r\n");
    uart_printf("SystemCoreClock: %lu Hz\r\n", (unsigned long)SystemCoreClock);
    uart_printf("Pool size:       %u bytes\r\n", (unsigned)EDGEFLEX_POOL_SIZE);

    edgeflex_runtime_init();
    uart_print("[INIT] Dynamic Memory Manager initialized.\r\n");

    if (edgeflex_model_validate(&g_tinymlp_model) != EDGEFLEX_MODEL_OK) {
        uart_print("[ERROR] Model validation failed. Halting.\r\n");
        Error_Handler();
    }
    uart_printf("[LOAD] Model: %s | Layers: %u | In: %u | Out: %u\r\n",
                g_tinymlp_model.name, g_tinymlp_model.layer_count,
                g_tinymlp_model.input_size, g_tinymlp_model.output_size);

    edgeflex_runtime_result_t result;
    uart_print("[RUN ] Executing layer-by-layer inference...\r\n");
    edgeflex_runtime_run(&g_tinymlp_model, g_tinymlp_sample_input, TINYMLP_INPUT_SIZE, &result);

    if (result.infer_status != EDGEFLEX_INFER_OK) {
        uart_printf("[ERROR] Inference failed, status=%d\r\n", (int)result.infer_status);
        Error_Handler();
    }

    uart_print("-----------------------------------------\r\n");
    uart_print("EdgeFlex AI Runtime - Performance Report\r\n");
    uart_print("-----------------------------------------\r\n");
    uart_printf("Model:            %s\r\n", g_tinymlp_model.name);
    uart_printf("Layers:           %u\r\n", g_tinymlp_model.layer_count);
    uart_printf("Peak SRAM (pool): %lu bytes\r\n", (unsigned long)result.perf.mem.peak_usage_bytes);
    uart_printf("Current usage:    %lu bytes\r\n", (unsigned long)result.perf.mem.current_usage_bytes);
    uart_printf("Free bytes:       %lu bytes\r\n", (unsigned long)result.perf.mem.free_bytes);
    uart_printf("Allocations:      %lu\r\n", (unsigned long)result.perf.mem.allocation_count);
    uart_printf("Releases:         %lu\r\n", (unsigned long)result.perf.mem.release_count);
    uart_printf("Reuses:           %lu\r\n", (unsigned long)result.perf.mem.reuse_count);
    uart_printf("Alloc failures:   %lu\r\n", (unsigned long)result.perf.mem.failure_count);

    if (result.perf.timing_available && result.perf.ticks_per_ms > 0) {
        /* Real, hardware-measured value (DWT cycle counter). */
        int ms_int = (int)result.perf.elapsed_ms;
        int ms_frac = (int)((result.perf.elapsed_ms - (float)ms_int) * 1000.0f);
        if (ms_frac < 0) ms_frac = -ms_frac;
        uart_printf("Inference time:   %d.%03d ms (%lu cycles)\r\n",
                    ms_int, ms_frac, (unsigned long)result.perf.elapsed_ticks);
    } else {
        uart_print("Inference time:   NOT YET MEASURED\r\n");
    }

    uart_print("Final Output:     [");
    for (uint16_t i = 0; i < result.output_len; i++) {
        int whole = (int)result.output[i];
        int frac = (int)((result.output[i] - (float)whole) * 10000.0f);
        if (frac < 0) frac = -frac;
        uart_printf("%d.%04d%s", whole, frac, (i + 1 < result.output_len) ? ", " : "");
    }
    uart_print("]\r\n");
    uart_print("-----------------------------------------\r\n");
    uart_print("[DONE] Copy everything above and send it back for the benchmark log.\r\n");

    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
        HAL_Delay(500);
    }
}

/* ------------------------------------------------------------------------ */
/* DWT cycle counter - real hardware timing source                          */
/* ------------------------------------------------------------------------ */
static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static uint32_t dwt_get_cycles(void)
{
    return DWT->CYCCNT;
}

/* ------------------------------------------------------------------------ */
/* Minimal UART print helpers (no libc stdout retarget needed)              */
/* ------------------------------------------------------------------------ */
static void uart_print(const char *s)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

static void uart_printf(const char *fmt, ...)
{
    char buf[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_print(buf);
}

/* ------------------------------------------------------------------------ */
/* Clock: deliberately kept on the internal 16 MHz HSI, no PLL.             */
/* Black Pill boards ship with different HSE crystal populations across    */
/* revisions; running on HSI avoids depending on that hardware variable    */
/* for this MVP. Swapping in an HSE+PLL config to reach 84 MHz is called   */
/* out as future work in the README, not required for the demo to work.    */
/* ------------------------------------------------------------------------ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gi = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET); /* LED off (active-low) */

    gi.Pin = LED_PIN;
    gi.Mode = GPIO_MODE_OUTPUT_PP;
    gi.Pull = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &gi);
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

/* Called by HAL_UART_Init() - enables clocks and configures AF pins. */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gi = {0};
    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        gi.Pin = USART1_TX_PIN | USART1_RX_PIN;
        gi.Mode = GPIO_MODE_AF_PP;
        gi.Pull = GPIO_PULLUP;
        gi.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gi.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(USART1_GPIO_PORT, &gi);
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 500000; i++) { }
    }
}
