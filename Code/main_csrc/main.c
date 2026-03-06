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

// Set to 1 to enable features
#define SHOW_SECONDS_ON_CLOCK 0 
#define USE_12_HOUR_TIME 1

#define UPDATE_BRIGHTNESS_PERIOD 1000 /* Update the brightness every second*/
#define DISP_MAX_BRIGHTNESS 400
#define DISP_MIN_BRIGHTNESS 10
#define LDR_READ_MAX 4000
#define LDR_READ_MIN 100 

// Define a second threshold where the brightness abrubtly drops off in a pitch black room
// Also define hysteresis so the brightness won't change all the time if the LDR read hovers around 30
#define LDR_READ_ABS_MIN 28 
#define ABS_MIN_HYSTERESIS 10
#define DISP_ABS_MIN_BRIGHTNESS 1

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

    software_timer_t brightness_adj_timer = construct_stimer_p(get_tick_frequency(), UPDATE_BRIGHTNESS_PERIOD, HAL_GetTick(), PERIODIC_ST);

    bool update_gfx = true;
    bool mode_sw_state = false;
    bool is_device_upright = false;
    bool dark_room = false;
    while (true) {
        run_command_runner(&hrtc, &animation, &flash);

        // Adjust brightness
        if (is_stimer_finished(&brightness_adj_timer, HAL_GetTick())) {
            HAL_ADC_Start(&hadc);
            
            // Read LDR and map the ADC read to a brightness value
            uint32_t brightness = DISP_MIN_BRIGHTNESS;
            if (HAL_ADC_PollForConversion(&hadc, 10) == HAL_OK) {
                uint32_t ldr_read = HAL_ADC_GetValue(&hadc);

                if (ldr_read > LDR_READ_ABS_MIN + ABS_MIN_HYSTERESIS) {
                    dark_room = false;
                }

                if ((ldr_read <= LDR_READ_ABS_MIN) || dark_room) {
                    dark_room = true;
                    brightness = DISP_ABS_MIN_BRIGHTNESS;
                } else {
                    brightness = map(
                        ldr_read,
                        LDR_READ_MIN, LDR_READ_MAX,
                        DISP_MIN_BRIGHTNESS, DISP_MAX_BRIGHTNESS
                    );
                }
            }
            HAL_ADC_Stop(&hadc);

            gp1247ai_set_brightness(brightness, send_disp_buf_blocking);
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
            HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BCD);

            if ((date.Date != last_date.Date) || update_gfx) {
                clock_draw_date(&u8g2, date.WeekDay, date.Date, date.Month);
                update_gfx = true;
            }

            if ((time.Minutes != last_time.Minutes) || update_gfx) {
                uint8_t hours = time.Hours;

                // Convert 24 hour time to 12 hour time
                #if USE_12_HOUR_TIME == 1
                if (hours > 12) {
                    hours -= 12;
                } else if (hours == 0) {
                    hours = 12;
                }
                #endif

                clock_draw_time(&u8g2, RTC_ByteToBcd2(hours), RTC_ByteToBcd2(time.Minutes));
                clock_draw_date(&u8g2, date.WeekDay, date.Date, date.Month);
                update_gfx = true;
            }

            #if SHOW_SECONDS_ON_CLOCK != 0
            if (time.Seconds != last_time.Seconds || update_gfx) {
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