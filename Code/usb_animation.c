#include "usb_animation.h"

#include "stm32f4xx_hal.h"
#include "system.h"
#include "software_timer.h"

#include "command_io.h"
#include "usb_cmd_cfg.h"
#include "usb.h"
#include "blocking_delay.h"

#define REQUEST_T        1000 /* Request for animation to start every 1000ms*/
#define DEFAULT_FPS      10
#define DISPLAY_WIDTH    253ULL
#define DISPLAY_HEIGHT   63ULL

#define FB_DEADSPACE     4 /* Keep 4 bytes at the start of the framebuffer free to pack with command bytes*/
// #define SWAP_BIT_ORDER(b) ( \
//     ((b & 0x80) >> 7) | \
//     ((b & 0x40) >> 5) | \
//     ((b & 0x20) >> 3) | \
//     ((b & 0x10) >> 1) | \
//     ((b & 0x08) << 1) | \
//     ((b & 0x04) << 3) | \
//     ((b & 0x02) << 5) | \
//     ((b & 0x01) << 7)   \
// )

// __RBIT()

#define REVERSE_BYTE(b) (__RBIT(b) >> 24)

typedef enum {
    IDLE, // No animation is reqeuested
    REQUEST_START,
    STARTING,
    DRAWING,
    RUNNING_IDLE,
} state_t;

static state_t state = IDLE;
static software_timer_t frame_timer;
static software_timer_t request_timer;

static cmd_arg_int fb_pos_x = 0;
static cmd_arg_int fb_pos_y = 0;

// The size of the pixel data currently loaded in the frame buffer
static cmd_arg_int fb_width = 0;
static cmd_arg_int fb_height = 0;

static bool is_upside_down = false;

static uint8_t fb[FB_DEADSPACE + PACKET_ALIGNED_FB_SIZE(DISPLAY_WIDTH, DISPLAY_HEIGHT)] = {0};

// Init usb animation module and usb cdc communication
// This function does not initialise u8g2, that needs
// happen before calling start_usb_animation
void init_usb_animation(void) {
    init_usb();
    init_cmd_io(ACK_TIMEOUT_MS);

    frame_timer = construct_stimer_f(
        get_tick_frequency(),
        DEFAULT_FPS,
        HAL_GetTick(),
        PERIODIC_ST
    );

    request_timer = construct_stimer_p(
        (uint16_t)get_tick_frequency(),
        REQUEST_T,
        HAL_GetTick(),
        PERIODIC_ST
    );
}

// Tells the computer the MCU is ready to start the animation
void start_usb_animation(void) {
    state = REQUEST_START;
}

// Tells the computer the MCU will no longer be accepting frames
void stop_usb_animation(void) {
    tx_usb_cmd(CANCEL_ANIMATION, false);
    state = IDLE;
}

// Rotates the framebuffer so it is upside down
static void rotate_fb(void) {
    uint8_t temp = 0;
    uint32_t fb_raw_len = CALC_FB_BYTES(fb_width, fb_height);

    uint8_t *raw_fb = fb + FB_DEADSPACE;

    // Reverse the byte order of the raw bytes
    uint32_t halfway_point = fb_raw_len / 2;
    for (uint32_t i = 0; i < halfway_point; i++) {
        uint32_t swp_idx = fb_raw_len - 1 - i;
        temp = raw_fb[swp_idx];
        raw_fb[swp_idx] = REVERSE_BYTE(raw_fb[i]);
        raw_fb[i] = REVERSE_BYTE(temp);
    }

    if (fb_raw_len % 2 != 0) {
        raw_fb[halfway_point] = REVERSE_BYTE(raw_fb[halfway_point]);
    }
}

// This function will handle the TX and RX of usb commands 
// and data for funning an animation on the u8g2 display
// Intended to be run in the main while loop
void run_animation_state_machine(send_buf_fp send_buf) {
    // Reset state machine if usb communication stops
    if (!is_usb_connected() && state != IDLE) {
        state = REQUEST_START;
    }

    // Request a new frame
    if (is_stimer_finished(&frame_timer, HAL_GetTick())) {
        if (state == RUNNING_IDLE) {
            tx_usb_cmd(FRAME_REQUEST, false);
            state = DRAWING;
        }
    }

    // Combine with frame timer later
    // if (is_stimer_finished(&request_timer, HAL_GetTick())) {
    if (is_stimer_finished(&request_timer, HAL_GetTick())) {

        // Send an animation request to the PC if the MCU wants to start the animation
        if (state == REQUEST_START && is_usb_connected()) {
            tx_usb_cmd(ANIMATION_REQUEST, false);
        }
    }


    // If state is drawing the incoming bytes will be a framebuffer
    // This stops working when the mcu reads the packet and sends ack succesfully
    // but the pc doesn't see the ack then the whole shit breaks until it magically fixes itself
    // In a couple of seconds
    if (state == DRAWING) {

        uint32_t fb_size = PACKET_ALIGNED_FB_SIZE(fb_width, fb_height);
        uint32_t rx_cnt = 0;

        bool need_new_packet = true;

        while (rx_cnt < fb_size) {
            if (need_new_packet) {
                tx_usb_cmd(NEXT_FRAME_PACKET, false); 
                need_new_packet = false;
            }

            tud_task();

            if (tud_cdc_n_available(CMD_CDC_PORT)) {
                uint32_t tmp_rx_cnt = tud_cdc_n_read(CMD_CDC_PORT, fb + FB_DEADSPACE + rx_cnt, fb_size - rx_cnt);

                if (tmp_rx_cnt % FRAME_PACKET_SIZE == 0 && tmp_rx_cnt > 0) {
                    tx_usb_cmd(ACKNOWLEDGE, true); // Acknowledge frame packet

                    need_new_packet = true;
                    rx_cnt += tmp_rx_cnt;
                }
            }
        }

        // Flip framebuffer bitmap and adjust position if the display is upside down
        uint8_t pos_x;
        uint8_t pos_y;
        if (is_upside_down) {
            rotate_fb();
            pos_x = DISPLAY_WIDTH - fb_width - fb_pos_x;
            pos_y = DISPLAY_HEIGHT - fb_height - fb_pos_y;
        } else {
            pos_x = fb_pos_x;
            pos_y = fb_pos_y;
        }

        gp1247ai_write_bmp(pos_x, pos_y, fb, fb_width, fb_height, send_buf);
        state = RUNNING_IDLE;
        return;
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

    if (is_cmd_same(cmd, ACKNOWLEDGE) && state == REQUEST_START) {
        state = STARTING;
    }

    else if (is_cmd_same(cmd, SET_FRAMERATE)) {
        frame_timer = construct_stimer_f(
            get_tick_frequency(),
            args[0],
            HAL_GetTick(),
            PERIODIC_ST
        );
        ack = true;
    }

    else if (is_cmd_same(cmd, SET_LOCATION)) {
        fb_pos_x = args[0];
        fb_pos_y = args[1];
        ack = true;
    }

    else if (is_cmd_same(cmd, START_ANIMATION)) {
        fb_width = args[0];
        fb_height = args[1];
        state = RUNNING_IDLE;
        reset_stimer(&frame_timer, HAL_GetTick());
        ack = true;
    }

    else if (is_cmd_same(cmd, CANCEL_ANIMATION)) {
        state = IDLE;
        ack = true;
    }

    // Acknowledge recieved command if it was recognised
    if (ack) {
        tx_usb_cmd(ACKNOWLEDGE, true);
    }
}