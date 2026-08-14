#include "main.h"
#include "stm32f4xx_it.h"

void NMI_Handler(void) { while (1) { } }

void HardFault_Handler(void)
{
    /* Deliberately simple: a real fault handler for an MVP just needs to
     * stop safely and be visibly distinguishable (fast LED blink) rather
     * than silently hang or reset, so a fault during the demo is obvious
     * on hardware instead of looking like a frozen/working board. */
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++) { }
    }
}

void MemManage_Handler(void) { while (1) { } }
void BusFault_Handler(void)  { while (1) { } }
void UsageFault_Handler(void){ while (1) { } }
void SVC_Handler(void) { }
void DebugMon_Handler(void) { }
void PendSV_Handler(void) { }

void SysTick_Handler(void)
{
    HAL_IncTick();
}
