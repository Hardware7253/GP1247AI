#pragma once

#include <stdint.h>

// This module uses the DWT cycle counter to create a blocking wait function

void init_blocking_delay(void);
void delay_us(uint32_t wait_time);
void delay_ms(uint32_t wait_time);