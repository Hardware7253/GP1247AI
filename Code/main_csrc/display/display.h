#pragma once

#include <stdint.h>
#include "u8g2.h"
#include "gp1247ai.h"

void send_disp_buf(uint8_t *buf, uint32_t buf_len);
void send_disp_buf_blocking(uint8_t *buf, uint32_t buf_len);
void init_display(u8g2_t* u8g2, const u8g2_cb_t *rotation);
void change_display_rotation(u8g2_t* u8g2, const u8g2_cb_t *rotation);
