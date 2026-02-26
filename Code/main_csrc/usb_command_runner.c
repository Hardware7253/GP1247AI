#include "usb_command_runner.h"
#include <math.h>

#include "rtc_helpers.h"
#include "system.h"
#include "software_timer.h"

#include "command_io.h"
#include "usb_cmd_cfg.h"
#include "usb.h"
#include "blocking_delay.h"

#define PAGE_SIZE W25Q_PAGE_SIZE
static uint8_t page_buf[PAGE_SIZE] = {0};

typedef enum {
    USB_DC,  // USB is not connected
    IDLE,    // Waiting for commands
    RX_FILE, // Recieving file packets
} state_t;

static state_t state = USB_DC;
static cmd_arg_int file_bytes = 0;

static RTC_TimeTypeDef time = {0};
static RTC_DateTypeDef date = {0};

// Request new packets if no response was recieved within some time
static software_timer_t retry_timer;

// Initialises USB and command_io module
void init_command_runner(RTC_HandleTypeDef *hrtc) {
    init_usb();
    init_cmd_io(ACK_TIMEOUT_MS);

    // Get time so time.SecondFraction is initialised
    HAL_RTC_GetTime(hrtc, &time, RTC_FORMAT_BIN); 

    retry_timer = construct_stimer_p(
        get_tick_frequency(),
        ACK_TIMEOUT_MS,
        HAL_GetTick(),
        ONESHOT_ST 
    );
}

// Recieves and transmits USB commands and carries out tasks associated with those commands
// This should be run in the main loop without any blocking delays
void run_command_runner(RTC_HandleTypeDef *hrtc, animation_t *animation, flash_t *flash) {
    tud_task();

    // Reset state machine if usb communication stops
    if (!is_usb_connected()) {
        state = USB_DC;
        return;
    }

    // Read file packets over USB and write them into the flash chip
    if (state == RX_FILE) {
        uint32_t rx_cnt = 0;
        uint32_t rx_size = CIEL_INT(file_bytes, FILE_PACKET_SIZE);

        bool need_new_packet = true;

        while ((rx_cnt < rx_size)) {
            if (!is_usb_connected()) {
                return;
            }

            tud_task();

            // Need to wait for busy before requesting packet to maintain timing
            while (w25q_is_busy(flash)) {
                tud_task();
            }

            if (need_new_packet) {
                tx_usb_cmd(REQUEST_NEXT_PACKET, true); 
                reset_stimer(&retry_timer, HAL_GetTick());
                need_new_packet = false;
            }

            // Read usb packets into a page buffer
            if (tud_cdc_n_available(CMD_CDC_PORT)) {
                uint32_t this_rx_cnt = tud_cdc_n_read(CMD_CDC_PORT, page_buf + (rx_cnt % PAGE_SIZE), FILE_PACKET_SIZE);

                if (this_rx_cnt % FILE_PACKET_SIZE == 0 && this_rx_cnt > 0) {
                    tx_usb_cmd(ACKNOWLEDGE, true); // Acknowledge frame packet

                    need_new_packet = true;
                    rx_cnt += this_rx_cnt;
                }

                // Write a whole page at a time
                if ((rx_cnt % PAGE_SIZE) == 0 && rx_cnt > 0) {
                    w25q_write(flash, rx_cnt - PAGE_SIZE, page_buf, PAGE_SIZE);
                }
            }

            if (is_stimer_finished(&retry_timer, HAL_GetTick())) {
                need_new_packet = true;
            }
        }

        // Write the remaining bytes to the flash if they aren't fully aligned to a page
        uint32_t remainder = rx_size % PAGE_SIZE;
        if (remainder != 0) {
            w25q_write(flash, rx_cnt - remainder, page_buf, remainder);
        }

        init_animation(animation, animation->disp_send_buf_fn, flash);
        state = IDLE;
    }


    // Parse incoming commands
    // Disable acking of incoming commands so we can check it's valid MAYBE
    char cmd[COMMAND_CHARS];
    cmd_arg_int args[MAX_ARGS];
    bool ack = false;
    rx_usb_cmd(cmd, args, false, true);

    if (is_cmd_same(cmd, ERROR_COMMAND)) {
        return;
    }

    else if (is_cmd_same(cmd, START_FILE_TRANSFER)) {
        state = RX_FILE;
        file_bytes = args[0];
        cmd_arg_int arg_array[MAX_ARGS] = {0};

        // Ack early because erasing flash can block for a long time
        tx_usb_cmd(ACKNOWLEDGE, true);

        // Erase flash space required by the file
        uint32_t erase_sectors = CIEL_INT(file_bytes, W25Q_SECTOR_SIZE) / W25Q_SECTOR_SIZE;
        for (uint32_t i = 0; i < erase_sectors; i++) {
            w25q_erase(flash, SECTOR_ERASE_4KIB, i * W25Q_SECTOR_SIZE);

            // Send flash progress
            arg_array[0] = (cmd_arg_int)((i * 100) / erase_sectors);
            tx_usb_cmd_w_args(SEND_ERASE_PROGRESS, arg_array, false);
        }
    }

    else if (is_cmd_same(cmd, REQUEST_TIME)) {
        HAL_RTC_GetTime(hrtc, &time, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(hrtc, &date, RTC_FORMAT_BIN);

        // Send hours, minutes, seconds, and subseconds
        cmd_arg_int arg_array[MAX_ARGS] = {0};
        arg_array[0] = (cmd_arg_int)time.Hours;
        tx_usb_cmd_w_args(SEND_HOUR, arg_array, false);
        arg_array[0] = (cmd_arg_int)time.Minutes;
        tx_usb_cmd_w_args(SEND_MINUTE, arg_array, false);
        arg_array[0] = (cmd_arg_int)time.Seconds;
        tx_usb_cmd_w_args(SEND_SECOND, arg_array, false);
        arg_array[0] = (cmd_arg_int)round(100 * get_second_fraction(&time));
        tx_usb_cmd_w_args(SEND_SUBSECOND, arg_array, false);
        tx_usb_cmd(SET_TIME, false);

        ack = true;
    }

    else if (is_cmd_same(cmd, SEND_HOUR)) {
        time.Hours = (uint8_t)args[0];
        ack = true;
    }

    else if (is_cmd_same(cmd, SEND_MINUTE)) {
        time.Minutes = (uint8_t)args[0];
        ack = true;
    }

    else if (is_cmd_same(cmd, SEND_SECOND)) {
        time.Seconds = (uint8_t)args[0];
        ack = true;
    }

    else if (is_cmd_same(cmd, SEND_SUBSECOND)) {
        // Pointless the SSR is set to max when HAL_RTC_SetTime is called
        // time.SubSeconds = get_ssr(&time, args[0] / 100.0); 
        ack = true;
    }

    else if (is_cmd_same(cmd, SET_TIME)) {
        HAL_RTC_SetTime(hrtc, &time, RTC_FORMAT_BIN);
        ack = true;
    }

    else if (is_cmd_same(cmd, SEND_WEEKDAY)) {
        date.WeekDay = (uint8_t)args[0];
        ack = true;
    }

    else if (is_cmd_same(cmd, SEND_DAY)) {
        date.Date = (uint8_t)args[0];
        ack = true;
    }

    else if (is_cmd_same(cmd, SEND_MONTH)) {
        date.Month = (uint8_t)args[0];
        ack = true;
    }

    else if (is_cmd_same(cmd, SEND_YEAR)) {
        date.Year = (uint8_t)args[0] % 100;
        ack = true;
    }

    else if (is_cmd_same(cmd, SET_DATE)) {
        HAL_RTC_SetDate(hrtc, &date, RTC_FORMAT_BIN);
        ack = true;
    }

    else if (is_cmd_same(cmd, CAL_RTC_FORWARD)) {
        cmd_arg_int ppm_p = args[0];
        HAL_RTCEx_SetSmoothCalib(hrtc, RTC_SMOOTHCALIB_PERIOD_32SEC, ppm_p, 0);
        ack = true;
    }

    else if (is_cmd_same(cmd, CAL_RTC_BACKWARD)) {
        cmd_arg_int ppm_m = args[0];
        HAL_RTCEx_SetSmoothCalib(hrtc, RTC_SMOOTHCALIB_PERIOD_32SEC, 0, ppm_m);
        ack = true;
    }

   // Acknowledge recieved command if it was recognised
    if (ack) {
        tx_usb_cmd(ACKNOWLEDGE, true);
    }
}