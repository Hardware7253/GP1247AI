#pragma once

#include <stdint.h>
#include "u8g2.h"
#include "display_cfg.h"


#define BCD_TO_BIN(bcd_byte) (((bcd_byte) >> 4) * 10 + ((bcd_byte) & 0x0F))
#define BIN_TO_BCD(byte)     ((((byte) / 10) << 4) + ((byte) % 10))

// For positioning clock graphics elements
// X coordinates increase from left to right
// Y coordinates increase from top to bottom
#define LARGE_FONT u8g2_font_spleen32x64_mn
#define LARGE_FONT_W 32
#define LARGE_FONT_H 40 

#define SMALL_FONT u8g2_font_6x13_tr
#define SMALL_FONT_W 5 
#define SMALL_FONT_H 9 

// Bottom left position of the clock text
#define DRAW_CLOCK_X 38 
#define DRAW_CLOCK_Y 50
#define CLOCK_COLON_OFFSET 6

// Bottom left position of the date text
#define DRAW_DATE_X 211
#define DRAW_DATE_Y 40 
#define DATE_PAD    3

#define DRAW_BAR_Y (DISPLAY_HEIGHT - 1)

void clock_draw_time(u8g2_t *u8g2, uint8_t hour_bcd, uint8_t min_bcd);
void clock_draw_date(u8g2_t *u8g2, uint8_t day_of_week, uint8_t day_bcd, uint8_t month_bcd);
void clock_draw_second(u8g2_t *u8g2, uint8_t second_bcd);
void clock_draw_second_float(u8g2_t *u8g2, float second);