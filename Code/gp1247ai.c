#include "gp1247ai.h"
#include "blocking_delay.h"

// Draw the bitmap at the position (start_x, start_y) on the display
// bmp_width and height should be provided in pixels
// Bitmap must be column major, byte alligned
// IMPORTANT: the bitmap should start after byte 3 (the first 4 bytes are reserved)
void gp1247ai_write_bmp(
    uint8_t start_x,
    uint8_t start_y,
    uint8_t bmp[],
    uint8_t bmp_width,
    uint8_t bmp_height,
    send_buf_fp send_buf
) {
    uint8_t height_bytes = (bmp_height + 7) / 8;
    uint32_t bmp_len = height_bytes * bmp_width;
    bmp[0] = 0xF0;
    bmp[1] = start_x;
    bmp[2] = start_y;
    bmp[3] = (height_bytes * 8) - 1; // c parameter


    // Writes to the cg_ram of the display
    // The drawing of the bitmap starts at start_x and start_y
    // c is the vertical size of the bitmap, must be a multiple of 8
    send_buf(bmp, bmp_len + 4);
}

// Set the display brightness 
// Brightness should be in the range 0-1023
// Datasheet recommends brightness below 500 for longjevity
void gp1247ai_set_brightness(uint32_t brightness, send_buf_fp send_buf) {
    brightness &= 0x03FF; // There are only 12 brightness bits
    uint8_t buf[] = {0xA0, (uint8_t)(brightness >> 8), (uint8_t)brightness};
    send_buf(buf, 3);
}

// Send init commands    
// senf_buf should control spi and cs
void gp1247ai_init(send_buf_fp send_buf) {

    // Hardcode standard init sequence from datasheet
    // First byte is the command ID
    send_buf((uint8_t[]){0xAA}, 1);                                                    // Software reset
    send_buf((uint8_t[]){0x78, 0x08}, 2);                                              // Oscillation setting
    send_buf((uint8_t[]){0xCC, 0x05, 0x00}, 3);                                        // VFD Mode Setting
    send_buf((uint8_t[]){0xE0, 0xFC, 0x3E, 0x00, 0x20, 0x80, 0x80, 0x80}, 8);          // Display Area Setting
    send_buf((uint8_t[]){0xB1, 0x20, 0x3F, 0x00, 0x01}, 5);                            // Internal Speed Setting
    gp1247ai_set_brightness(INIT_BRIGHTNESS, send_buf);
    send_buf((uint8_t[]){0x55}, 1);                                                    // Clear GRAM
    delay_ms(20);
    send_buf((uint8_t[]){0xC0, 0x00, 0x00}, 3);                                        // Display Position Offset
    send_buf((uint8_t[]){0x80, 0x00}, 2);                                              // Display Mode Setting
}