#pragma once

// #include "u8g2.h"
#include "gp1247ai.h"

void init_usb_animation(void);
void start_usb_animation(void);
void stop_usb_animation(void);
void run_animation_state_machine(send_buf_fp send_buf);