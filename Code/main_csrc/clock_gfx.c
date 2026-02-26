#include <string.h>
#include <math.h>
#include "clock_gfx.h"
#include "display_cfg.h"

// Index by day of week - 1
static const char* DOW_DAYS_3[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

// Takes a char and writes the ascii representation of the BCD number into it
// The provided char pointer must hold atleast 3 chars (2 for the BCD numbers and 1 for the null terminator)
static void populate_bcd_string(uint8_t bcd, char *chars) {
    chars[0] = ((bcd & 0xF0) >> 4) + 48; 
    chars[1] = (bcd & 0x0F) + 48;
    chars[2] = '\0';
}

// Draw the clockface with the hour and minute into the u8g2 buffer
void clock_draw_time(u8g2_t *u8g2, uint8_t hour_bcd, uint8_t min_bcd) {
    u8g2_SetFont(u8g2, LARGE_FONT);
    char bcd_string[3] = {0};

    populate_bcd_string(hour_bcd, bcd_string);
    u8g2_DrawStr(u8g2, DRAW_CLOCK_X, DRAW_CLOCK_Y, bcd_string);

    u8g2_DrawStr(u8g2, DRAW_CLOCK_X + 2 * LARGE_FONT_W, DRAW_CLOCK_Y - CLOCK_COLON_OFFSET, ":");

    populate_bcd_string(min_bcd, bcd_string);
    u8g2_DrawStr(u8g2, DRAW_CLOCK_X + 3 * LARGE_FONT_W, DRAW_CLOCK_Y, bcd_string) ;
}

// Draw the date into the u8g2 buffer
void clock_draw_date(u8g2_t *u8g2, uint8_t day_of_week, uint8_t day_bcd, uint8_t month_bcd) {
    u8g2_SetFont(u8g2, SMALL_FONT);
    char date_str[6] = {0};
    char bcd_string[3] = {0};

    populate_bcd_string(day_bcd, bcd_string);
    strcpy(date_str, bcd_string);

    strcpy(date_str + 2, "/");

    populate_bcd_string(month_bcd, bcd_string);
    strcpy(date_str + 3, bcd_string);

    u8g2_DrawStr(u8g2, DRAW_DATE_X, DRAW_DATE_Y, date_str);
    u8g2_DrawStr(u8g2, DRAW_DATE_X, DRAW_DATE_Y - SMALL_FONT_H - DATE_PAD, DOW_DAYS_3[day_of_week - 1]);
}

// Draw the second bar into the u8g2 buffer
void clock_draw_second(u8g2_t *u8g2, uint8_t second_bcd) {
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawBox(u8g2, 0, DRAW_BAR_Y, DISPLAY_WIDTH, 1);

    uint8_t second = BCD_TO_BIN(second_bcd);
    uint32_t second_progress = ((uint32_t)second * 100) / 60;
    uint32_t progress_bar_length = (DISPLAY_WIDTH * second_progress) / 100;


    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawBox(u8g2, 0, DRAW_BAR_Y, progress_bar_length, 1);
}

// Draw the second bar into the u8g2 buffer
// This function uses floating point math rather than integer math
void clock_draw_second_float(u8g2_t *u8g2, float second) {
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawBox(u8g2, 0, DRAW_BAR_Y, DISPLAY_WIDTH, 1);

    uint32_t progress_bar_length = (uint32_t)round(DISPLAY_WIDTH * (second / 60));

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawBox(u8g2, 0, DRAW_BAR_Y, progress_bar_length, 1);
}



