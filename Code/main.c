#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_conf.h"

#include "software_timer.h"
#include "system.h"
#include "bit.h"
#include "blocking_delay.h"

// #include "u8g2.h"
#include "display.h"
#include "usb_animation.h"
#include "usb.h"

#include "button.h"


#include <stdio.h> // Remove later

static uint16_t test_samples[100] = {0}; // Measured before DUT
static uint16_t dut_samples[100] = {0}; // Measured after DUT

static RTC_HandleTypeDef hrtc;

#include <math.h>
int calculate_rtc_ppm_error(float seconds_drift_24h) {
    return (int)round(seconds_drift_24h * 11.57407407F);
}

int main(void) {
    HAL_Init();
    init_clocks();
    init_blocking_delay();
    init_usb_animation();
   
    // LED (PA1) GPIO config 
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitTypeDef pin_cfg = {
            .Pin =  GPIO_PIN_1,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOA, &pin_cfg);
    }

    __HAL_RCC_RTC_ENABLE();
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_PUSHPULL;
    error_handler_msg(HAL_RTC_Init(&hrtc), "Failed to init RTC");
    error_handler_msg(HAL_RTCEx_SetSmoothCalib(&hrtc, RTC_SMOOTHCALIB_PERIOD_32SEC, 0, 70), "Failed to write calibration values to RTC");


    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

  // Calibration
    // {
    //     __HAL_RCC_GPIOC_CLK_ENABLE();

    //     GPIO_InitTypeDef pin_cfg = {
    //         .Pin =  GPIO_PIN_13,
    //         .Mode = GPIO_MODE_OUTPUT_PP,
    //         .Pull = GPIO_NOPULL,
    //         .Speed = GPIO_SPEED_FREQ_LOW,
    //         .Alternate = GPIO_AF0_RTC_50Hz,
    //     };
    //     HAL_GPIO_Init(GPIOC, &pin_cfg);

    //     HAL_RTCEx_SetCalibrationOutPut(&hrtc, RTC_CALIBOUTPUT_512HZ);
    // }

    time.Hours = 0;
    time.Minutes = 0;
    time.Seconds = 0;
    time.TimeFormat = RTC_HOURFORMAT12_AM;
    HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);

    // HAL_RTCEx_SetCalibrationOutPut


    // u8g2_t u8g2;
    // init_display(&u8g2, U8G2_R2);
    // u8g2_SetPowerSave(&u8g2, 0); // wake up display
    // u8g2_SetContrast(&u8g2, 128);

    // u8g2_ClearDisplay(&u8g2);
    // u8g2_SetFont(&u8g2, u8g2_font_victoriabold8_8r); // Large 
    // u8g2_DrawStr(&u8g2, 100, 30,"Hello World!");
    // u8g2_DrawBox(&u8g2, 10, 10, 30, 10);


    // u8g2_SetFont(&u8g2, u8g2_font_squeezed_r6_tr); // Small
    // u8g2_DrawStr(&u8g2, 100, 40,"Awesome epic body text");

    // #define test_width 16
    // #define test_height 7
    // static char test_bits [] = {
    // 0x00,0x00,0x04,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x0c,
    // 0x00,0x00,0x00,0x38,0x00,0x00,0xfe,0x00,0x00,0x00,0xff,0x07,0x00,0x80,0x9f,
    // 0x3f,0x00,0xc0,0x03,0x7c,0x00,0xe0,0x01,0xe0,0x01,0xf0,0x00,0xc0,0x03,0x70,
    // 0x00,0x00,0x07,0x38,0x1c,0x18,0x0e,0x18,0x1c,0x1c,0x1c,0x1c,0x18,0x0c,0x18,
    // 0x0c,0x00,0x00,0x38,0x0c,0x00,0x00,0x30,0x0c,0x00,0xc0,0x30,0x0c,0x03,0xf8,
    // 0x30,0x0c,0xff,0xff,0x30,0x0c,0xfe,0x0f,0x30,0x0c,0x00,0x00,0x38,0x1c,0x00,
    // 0x00,0x1c,0x1c,0x00,0x00,0x0c,0x18,0x00,0x00,0x0e,0x78,0x00,0x80,0x07,0xf0,
    // 0x3f,0xf8,0x03,0xe0,0xff,0xff,0x01,0x00,0xe0,0x0f,0x00,0x00,0x00,0x00,0x00,
    // 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    // };

    // u8g2_DrawXBM(&u8g2, 0, 0, 32, 32, test_bits);
    // u8g2_SendBuffer(&u8g2);



    // Test button
//     button_debounce_t debounce_1 = {0};
//    {
//         __HAL_RCC_GPIOA_CLK_ENABLE();

//         GPIO_InitTypeDef pin_cfg = {
//             .Pin =  GPIO_PIN_5,
//             .Mode = GPIO_MODE_INPUT,
//             .Pull = GPIO_PULLUP,
//             .Speed = GPIO_SPEED_FREQ_LOW,
//             .Alternate = 0,
//         };
//         HAL_GPIO_Init(GPIOA, &pin_cfg);
//     }

    init_display();
    // gp1247ai_set_brightness(400, send_disp_buf);

    // Bitmap tes   t
    // while(true) {

    // {
    //     uint8_t bmp[12] = {0, 0, 0, 0, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    //     gp1247ai_write_bmp(10, 10, bmp, 8, 8, send_disp_buf);
    //     block_until_last_disp_tx_cplt();
    // }
    // delay_ms(500);
    // {

    //     uint8_t bmp[12] = {
    //         0, 0, 0, 0,
    //         0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
    //     };
    //     gp1247ai_write_bmp(10, 10, bmp, 8, 8, send_disp_buf);
    //     block_until_last_disp_tx_cplt();
    // }
    // delay_ms(500);
    // }
    // BLOCK_UNTIL_DISP_SPI_FINISHED

    // This would be a 2 second period software timer
    // software_timer_t stimer = construct_stimer_p(get_tick_frequency(), 2000, HAL_GetTick(), PERIODIC_ST);

    start_usb_animation();

    uint32_t last_tick = 0;


    while (true) {
        tud_task();

        // int bytes = tud_cdc_n_available(0);
        // if (bytes > 0) {
        //     tud_cdc_read_flush();
        //     tud_cdc_n_write(0, "PONG", 4);
        //     tud_cdc_write_flush();
        // }

        if (last_tick > HAL_GetTick()) {
            printf("Woah");
        }
        last_tick = HAL_GetTick();


        run_animation_state_machine(send_disp_buf);
        // if (is_stimer_finished(&stimer, HAL_GetTick())) {

            // #include "command_io.h"
            // tx_usb_cmd("AR");
        // }

        HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

        // bool pin_state = (bool)HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
        // if (button_pressed(&debounce_1, !pin_state, HAL_GetTick())) {
        //     (void) 0; // Any press detected (with debouncing)
        // }

        // if (button_long_pressed(&debounce_1, !pin_state, HAL_GetTick())) {
        //     (void) 0; // Long press detected
        // }

    }

    return 0;
}