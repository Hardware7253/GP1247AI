#pragma once

#include <stdint.h>
#include <stdbool.h>

#define INIT_BRIGHTNESS 200 

// GP1247AI SPI tx function pointer - should also control chip select
// SPI should send bytes LSB
// Parameters should be: tx_buf, tx_buf_size
typedef void (*vfd_tx_fp)(uint8_t*, uint32_t); 

void gp1247ai_set_brightness(uint32_t brightness, vfd_tx_fp senf_buf);

void gp1247ai_write_bmp(
    uint8_t start_x,
    uint8_t start_y,
    uint8_t bmp_buf[],
    uint8_t bmp_width,
    uint8_t bmp_height,
    vfd_tx_fp send_buf
);

void gp1247ai_init(vfd_tx_fp send_buf);