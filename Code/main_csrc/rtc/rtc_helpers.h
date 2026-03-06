#pragma once

#include <stdint.h>
#include "stm32f4xx_hal_rtc.h"

int calculate_rtc_ppm_error(float seconds_drift_24h);
float get_second_fraction(RTC_TimeTypeDef *time);
uint32_t get_ssr(RTC_TimeTypeDef *time, float subsecond_frac);