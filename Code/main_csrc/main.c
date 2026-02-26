#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_conf.h"

#include "software_timer.h"
#include "system.h"
#include "bit.h"
#include "blocking_delay.h"
#include "rtc_helpers.h"
#include "clock_gfx.h"

#include "display.h"
#include "usb_command_runner.h"
#include "animation_player.h"
#include "usb.h"

#include "flash.h"
#include "w25q.h"
#include "emb_helpers.h"

#define IO_CLK_EN               __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE

#define TILT_SW_IRQN            EXTI0_IRQn
#define TILT_SW_BUS             GPIOB
#define TILT_SW_PIN             GPIO_PIN_0
#define TILT_SW                 TILT_SW_BUS, TILT_SW_PIN

#define MODE_SW_IRQN            EXTI9_5_IRQn
#define MODE_SW_BUS             GPIOA
#define MODE_SW_PIN             GPIO_PIN_7
#define MODE_SW                 MODE_SW_BUS, MODE_SW_PIN 

#define LDR_BUS                 GPIOC
#define LDR_PIN                 GPIO_PIN_4
#define LDR                     LDR_BUS, LDR_PIN
#define LDR_ADC_CLK_EN          __HAL_RCC_ADC1_CLK_ENABLE
#define LDR_ADC                 ADC1
#define LDR_ADC_CHANNEL         ADC_CHANNEL_14 

#define SHOW_SECONDS_ON_CLOCK 0 

#define UPDATE_BRIGHTNESS_PERIOD 1000 /* Update the brightness every second*/
#define DISP_MAX_BRIGHTNESS 400
#define DISP_MIN_BRIGHTNESS 1
#define LDR_READ_MAX 4000
#define LDR_READ_MIN 278 

extern void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(TILT_SW_PIN);
}

extern void EXTI9_5_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(MODE_SW_PIN);
}

// Set true so the pins are read at startup
static volatile bool update_orientation = true;
static volatile bool change_mode = true;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    change_mode = GPIO_Pin == MODE_SW_PIN;
    update_orientation = GPIO_Pin == TILT_SW_PIN;
}

int main(void) {
    RTC_HandleTypeDef hrtc = {0};
    ADC_HandleTypeDef hadc = {0};
    animation_t animation = {0};

    RTC_TimeTypeDef time = {0};
    RTC_TimeTypeDef last_time = {0};

    RTC_DateTypeDef date = {0};
    RTC_DateTypeDef last_date = {0};

    w25q_t flash = {0};

    HAL_Init();
    init_clocks();
    init_blocking_delay();
    flash_init(&flash);
    init_animation(&animation, send_disp_buf, &flash);
   
    // IO pins config
    {
        IO_CLK_EN();

        GPIO_InitTypeDef pin_cfg = {
            .Pin =  TILT_SW_PIN,
            .Mode = GPIO_MODE_IT_RISING_FALLING,
            .Pull = GPIO_PULLUP,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(TILT_SW_BUS, &pin_cfg);
        pin_cfg.Pin = MODE_SW_PIN;
        HAL_GPIO_Init(MODE_SW_BUS, &pin_cfg);

        pin_cfg = (GPIO_InitTypeDef){
            .Pin =  LDR_PIN,
            .Mode = GPIO_MODE_ANALOG,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(LDR_BUS, &pin_cfg);


        HAL_NVIC_EnableIRQ(TILT_SW_IRQN);
        HAL_NVIC_SetPriority(TILT_SW_IRQN, 15, 5);
        HAL_NVIC_EnableIRQ(MODE_SW_IRQN);
        HAL_NVIC_SetPriority(MODE_SW_IRQN, 15, 5);
    }

    // Adc config
    {
        LDR_ADC_CLK_EN();
        hadc.Instance = LDR_ADC;
        hadc.Init.Resolution = ADC_RESOLUTION_12B;
        hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
        hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
        hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
        hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
        hadc.Init.ScanConvMode = DISABLE;
        hadc.Init.ContinuousConvMode = DISABLE;
        hadc.Init.DiscontinuousConvMode = DISABLE;
        error_handler_msg(HAL_ADC_Init(&hadc), "Failed to init ADC");


        // Config channel
        ADC_ChannelConfTypeDef channel_cfg = {
            .Channel = LDR_ADC_CHANNEL,
            .Rank = 1,
            .SamplingTime = ADC_SAMPLETIME_480CYCLES, 
        };
        error_handler_msg(HAL_ADC_ConfigChannel(&hadc, &channel_cfg), "Failed to config ADC channel");
    }

    __HAL_RCC_RTC_ENABLE();
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;

    // Values for 32.768 KHz LSE
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_PUSHPULL;
    error_handler_msg(HAL_RTC_Init(&hrtc), "Failed to init RTC");
    init_command_runner(&hrtc);

    u8g2_t u8g2;
    init_display(&u8g2, U8G2_R0);
    u8g2_SetPowerSave(&u8g2, 0); // wake up display
    u8g2_ClearDisplay(&u8g2);
    u8g2_SetContrast(&u8g2, DISP_MAX_BRIGHTNESS / 2);

    uint8_t wbuf[256] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
        50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
        60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
        70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
        90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
        100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
        110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
        120, 121, 122, 123, 124, 125, 126, 127,
        128, 129, 130, 131, 132, 133, 134, 135, 136, 137,
        138, 139, 140, 141, 142, 143, 144, 145, 146, 147,
        148, 149, 150, 151, 152, 153, 154, 155, 156, 157,
        158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
        168, 169, 170, 171, 172, 173, 174, 175, 176, 177,
        178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
        188, 189, 190, 191, 192, 193, 194, 195, 196, 197,
        198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
        208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
        218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
        228, 229, 230, 231, 232, 233, 234, 235, 236, 237,
        238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
        248, 249, 250, 251, 252, 253, 254, 255
    };
    // w25q128_erase(SECTOR_ERASE_4KIB, 0);
    // w25q128_write(0, wbuf, 256);

    uint8_t rbuf[512] = {0};
    uint8_t *buf = rbuf;
    w25q_read(&flash, 0x0, rbuf, 512);


    software_timer_t brightness_adj_timer = construct_stimer_p(get_tick_frequency(), UPDATE_BRIGHTNESS_PERIOD, HAL_GetTick(), PERIODIC_ST);

    bool update_gfx = true;
    bool mode_sw_state = false;
    bool is_device_upright = false;
    while (true) {
        run_command_runner(&hrtc, &animation, &flash);

        // Adjust brightness
        if (is_stimer_finished(&brightness_adj_timer, HAL_GetTick())) {
            HAL_ADC_Start(&hadc);
            
            // Read LDR and map the ADC read to a brightness value
            uint32_t brightness = DISP_MIN_BRIGHTNESS;
            if (HAL_ADC_PollForConversion(&hadc, 10) == HAL_OK) {
                uint32_t ldr_read = HAL_ADC_GetValue(&hadc);
                brightness = map(
                    ldr_read,
                    LDR_READ_MIN, LDR_READ_MAX,
                    DISP_MIN_BRIGHTNESS, DISP_MAX_BRIGHTNESS
                );

                if (brightness > DISP_MAX_BRIGHTNESS) {
                    brightness = DISP_MAX_BRIGHTNESS;
                }
            }
            HAL_ADC_Stop(&hadc);

            u8g2_SetContrast(&u8g2, brightness);
        }

        // Check for mode change interrupt event 
        if (change_mode) {
            mode_sw_state = HAL_GPIO_ReadPin(MODE_SW);
            u8g2_ClearDisplay(&u8g2);

            update_gfx = true;
            change_mode = false;
        }

        // Check for update orientation interrupt event
        if (update_orientation) {
            is_device_upright = !HAL_GPIO_ReadPin(TILT_SW);
            change_display_rotation(&u8g2, is_device_upright ? U8G2_R0 : U8G2_R2);
            u8g2_ClearDisplay(&u8g2);

            update_gfx = true;
            update_orientation = false;
        }

        if (mode_sw_state) {
            run_animation(&animation, !is_device_upright);
        } else { // Run clock
            HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BCD);
            HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BCD);

            if (date.Date != last_date.Date | update_gfx) {
                clock_draw_date(&u8g2, date.WeekDay, date.Date, date.Month);
                update_gfx = true;
            }

            if (time.Minutes != last_time.Minutes | update_gfx) {
                clock_draw_time(&u8g2, time.Hours, time.Minutes);
                clock_draw_date(&u8g2, date.WeekDay, date.Date, date.Month);
                update_gfx = true;
            }

            #if SHOW_SECONDS_ON_CLOCK != 0
            if (time.Seconds != last_time.Seconds | update_gfx) {
                clock_draw_second(&u8g2, time.Seconds); 
                update_gfx = true;
            }

            // Subseconds bar needs to use a smarter method to transfer to the display
            // Because sending the whole buffer every couple milliseconds causes too much blocking
            // if (time.SubSeconds != last_time.SubSeconds | update_gfx) {
            //     float seconds = (float)BCD_TO_BIN(time.Seconds);
            //     seconds += get_second_fraction(&time);
            //     clock_draw_second_float(&u8g2, seconds); 
            //     update_gfx = true;
            // }
            #endif

            if (update_gfx) {
                u8g2_SendBuffer(&u8g2);
                update_gfx = false;
            }

            last_time = time;
            last_date = date;
        }
    }

    return 0;
}