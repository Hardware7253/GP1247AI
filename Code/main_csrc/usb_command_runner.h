#pragma once

#include "stm32f4xx_hal.h"
#include "animation_player.h"
#include "flash.h"

void init_command_runner(RTC_HandleTypeDef *hrtc);
void run_command_runner(RTC_HandleTypeDef *hrtc, animation_t *animation, flash_t *flash);