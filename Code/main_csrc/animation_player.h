#pragma once


#include <stdint.h>
#include <stdbool.h>

#include "gp1247ai.h"
#include "flash.h"
#include "usb_cmd_cfg.h"
#include "software_timer.h"
#include "display_cfg.h"

// Keep 4 bytes at the start of the bitmap buffer free to pack with command bytes
// as required by the gp1247ai write command
#define BMP_BUF_DEADSPACE 4  

// Struct for an animation
// Includes metadata, frame timer, and buffer for storing a frame bitmap
typedef struct {

    // Width and height in pixels
    uint32_t width;
    uint32_t height;

    // Byte length of the bitmaps used by the current animation
    uint32_t bmp_bytes;

    // Position of the bitmap on the display
    uint32_t pos_x;
    uint32_t pos_y;

    uint32_t frames;
    uint32_t current_frame;
    uint8_t fps;

    software_timer_t frame_timer;
    uint8_t bmp_buf[BMP_BUF_DEADSPACE + CALC_BMP_BYTES(DISPLAY_WIDTH, DISPLAY_HEIGHT)];
    uint8_t *bmp;
    vfd_tx_fp disp_send_buf_fn;
    flash_t *flash;

} animation_t;


void init_animation(animation_t *animation, vfd_tx_fp disp_send_buf_fn, flash_t *flash);
void run_animation(animation_t *animation, bool display_upside_down);