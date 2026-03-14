#pragma once

#include <stdint.h>

#define USE_U8G2 0

#if (USE_U8G2 == 1)
#include "u8g2.h"
void init_display(u8g2_t* u8g2, const u8g2_cb_t *rotation);
#endif


#if (USE_U8G2 == 0)
#include "gp1247ai.h"

// void block_until_last_disp_tx_cplt(void);
bool update_disp_cs(void);
void send_disp_buf(uint8_t *buf, uint32_t buf_len);
void send_disp_buf_blocking(uint8_t *buf, uint32_t buf_len);
void init_display(void);
#endif
