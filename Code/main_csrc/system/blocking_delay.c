#include <stdbool.h>
#include "blocking_delay.h"
#include "stm32f4xx_hal.h"

// Enables DWT counter
void init_blocking_delay(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

// Blocking wait for wait_time us
void delay_us(uint32_t wait_time) {
    uint32_t start_cycles = DWT->CYCCNT;
    uint32_t clk_speed = HAL_RCC_GetSysClockFreq();
    uint32_t wait_cycles = (uint32_t)(((uint64_t)wait_time * (uint64_t)clk_speed) / 1000000ULL);

    // Can account for one DWT cycle counter rollover
    while ((DWT->CYCCNT - start_cycles) < wait_cycles) {
        (void) 0;
    }
}

// Blocking wait for wait_time ms
// Max period: SYSCLK SPEED / BIT(32) seconds
// e.g. SYSCLK = 168 MHz => 25.56 seconds
inline void delay_ms(uint32_t wait_time) {
    delay_us(wait_time * 1000);
}