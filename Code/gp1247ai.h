#pragma once

#include <stdint.h>
#include <stdbool.h>

#define INIT_BRIGHTNESS 200 

typedef void (*write_cs_fp)(uint8_t);

// For GP1247AI the bytes should be sent LSB first over SPI
typedef void (*send_buf_fp)(uint8_t*, uint32_t); 

void gp1247ai_set_brightness(uint32_t brightness, send_buf_fp senf_buf);

void gp1247ai_write_bmp(
    uint8_t start_x,
    uint8_t start_y,
    uint8_t bmp[],
    uint8_t bmp_width,
    uint8_t bmp_height,
    send_buf_fp send_buf
);

void gp1247ai_init(send_buf_fp send_buf);