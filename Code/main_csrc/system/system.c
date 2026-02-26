#include "system.h"
#include "stm32f4xx_hal.h"

// Get stuck in a while loop incase of an error
// This can easily be detected when debugging
void error_handler(HAL_StatusTypeDef status) {
  while (status != HAL_OK) (void) 0;
}

// Get stuck in a while loop incase of an error
// Also takes a pointer to an error message to be viewed by the debugger
void error_handler_msg(HAL_StatusTypeDef status, volatile char* msg) {
    while (status != HAL_OK) (void) msg;
}


// Init clocks for https://stm32-base.org/boards/STM32F407VGT6-STM32F4XX-M
// SYSCLK = 168 MHz
// HCLK =   168 MHz
// PCLK1 =  42 MHz
// PCLK2 =  84 MHz
void init_clocks(void) {
    // Oscillator config
    {
        RCC_PLLInitTypeDef pll_cfg = {
            .PLLState = RCC_PLL_ON,
            .PLLSource = RCC_PLLSOURCE_HSE,
            .PLLM = 4,
            .PLLN = 168,
            .PLLP = RCC_PLLP_DIV2,
            .PLLQ = 7,
        };

        // Turn on HSE and LSE
        // Leave LSI and HSI untouched
        RCC_OscInitTypeDef osc_cfg;
        osc_cfg.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
        osc_cfg.HSEState = RCC_HSE_ON;
        osc_cfg.LSEState = RCC_LSE_ON;
        osc_cfg.PLL = pll_cfg;
        error_handler_msg(HAL_RCC_OscConfig(&osc_cfg), "Error with oscillator config");
    }

    // Clock config
    {
        RCC_ClkInitTypeDef clk_cfg = {
            .ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
            .SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
            .AHBCLKDivider = RCC_SYSCLK_DIV1,
            .APB1CLKDivider = RCC_HCLK_DIV4,
            .APB2CLKDivider = RCC_HCLK_DIV2,
        };
        error_handler_msg(HAL_RCC_ClockConfig(&clk_cfg, FLASH_ACR_LATENCY_5WS), "Error with clock config");
    }

    // Enable RTC
    RCC_PeriphCLKInitTypeDef periph_cfg = {
        .PeriphClockSelection = RCC_PERIPHCLK_RTC,
        .RTCClockSelection = RCC_RTCCLKSOURCE_LSE,
    };
    error_handler_msg(HAL_RCCEx_PeriphCLKConfig(&periph_cfg), "Error with periph clock config");
}

// Returns the tick frequency of the systick interrupt in Hz
uint32_t get_tick_frequency(void) {
    return 1000 / HAL_GetTickFreq();
}

extern void SysTick_Handler(void) {
    HAL_IncTick();
}

// extern void