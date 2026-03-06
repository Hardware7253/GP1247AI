#include "animation_player.h"
#include "stm32f4xx_hal.h"
#include "system.h"
#include "bit.h"


// Reads animation metadata from flash to initialise the animation struct
void init_animation(animation_t *animation, vfd_tx_fp disp_send_buf_fn, flash_t *flash) {
    cmd_arg_int args[ANIMATION_META_LEN] = {0};
    w25q_read(flash, 0, (uint8_t*)args, ANIMATION_META_LEN * sizeof(cmd_arg_int));
    animation->width = (uint32_t)args[0];
    animation->height = (uint32_t)args[1];
    animation->pos_x = (uint32_t)args[2];
    animation->pos_y = (uint32_t)args[3];
    animation->frames = (uint32_t)args[4];
    animation->fps = (uint8_t)args[5];

    animation->current_frame = 0;
    animation->bmp = animation->bmp_buf + BMP_BUF_DEADSPACE; 
    animation->bmp_bytes = CALC_BMP_BYTES(animation->width, animation->height); 

    animation->disp_send_buf_fn = disp_send_buf_fn;
    animation->flash = flash;
    animation->frame_timer = construct_stimer_f(
        get_tick_frequency(),
        animation->fps,
        HAL_GetTick(),
        PERIODIC_ST
    );
}

// Rotates the bitmap so it is upside down
static void rotate_bmp(animation_t *animation) {
    uint8_t temp = 0;
    uint32_t bmp_len = animation->bmp_bytes;

    // Swap bytes position in the bitmap array and swap the bitorder of the bytes
    uint32_t halfway_point = bmp_len / 2;
    for (uint32_t i = 0; i < halfway_point; i++) {
        uint32_t swp_idx = bmp_len - 1 - i;
        temp = animation->bmp[swp_idx];
        animation->bmp[swp_idx] = RBIT(animation->bmp[i]);
        animation->bmp[i] = RBIT(temp);
    }

    if ((bmp_len & 1) != 0) {
        animation->bmp[halfway_point] = RBIT(animation->bmp[halfway_point]);
    }
}

// Reads the animation from flash and sends the frames to the display
// This should be run in the main loop without any blocking delays
void run_animation(animation_t *animation, bool display_upside_down) {
    if (!is_stimer_finished(&(animation->frame_timer), HAL_GetTick())) {
        return;
    }

    uint32_t start_addr = ANIMATION_META_LEN * sizeof(cmd_arg_int);
    start_addr += animation->current_frame * animation->bmp_bytes;
    w25q_read(animation->flash, start_addr, (uint8_t*)animation->bmp, animation->bmp_bytes);

    // Flip framebuffer bitmap and adjust position if the display is upside down
    uint8_t pos_x;
    uint8_t pos_y;
    if (display_upside_down) {
        rotate_bmp(animation);
        pos_x = DISPLAY_WIDTH - animation->width - animation->pos_x;
        pos_y = DISPLAY_HEIGHT - CIEL_INT(animation->height, 8) - animation->pos_y;
    } else {
        pos_x = animation->pos_x;
        pos_y = animation->pos_y;
    }

    gp1247ai_write_bmp(
        pos_x, 
        pos_y,
        animation->bmp_buf,
        animation->width,
        animation->height,
        animation->disp_send_buf_fn 
    );

    // Replay animation when it's done
    animation->current_frame++;
    if (animation->current_frame >= animation->frames) {
        animation->current_frame = 0;
    }
}