#include "rtc_helpers.h"
#include <math.h>

inline int calculate_rtc_ppm_error(float seconds_drift_24h) {
    return (int)round(seconds_drift_24h * 11.57407407F);
}

// Returns a value between 0.0 and 1.0 indicating how much of the current
// second has passed according to the RTC subseconds register
inline float get_second_fraction(RTC_TimeTypeDef *time) {
    return (float)(time->SecondFraction - time->SubSeconds) / (time->SecondFraction + 1);
}

// Gets the SSR value from a subsecond fraction (0.0 - 1.0)
inline uint32_t get_ssr(RTC_TimeTypeDef *time, float subsecond_frac) {
    return (uint32_t)round(time->SecondFraction - (subsecond_frac) * (time->SecondFraction + 1));
}