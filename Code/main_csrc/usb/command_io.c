#include "command_io.h"
#include "usb.h"
#include "system.h"
#include "software_timer.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdbool.h>

// Todo
// Frame command bytes with start and stop byte
// Request new frame packet if the length is wrong 
// Add frame packet timout (request new frame packet) then remove that shit from python
// Maybe all transactions should require an ack, then if no ack is recieved retry after 1ms
//
// Ok
// Then basic version
// Everything requires an ack
// If the bytes recieved is different than expected don't ack
// If the command is wrong dont ack
// No need for error message
// Then the command will be resent (hopefully correctly) after a 1ms timeout
// 1ms should be fast enough given that an error is rare
// Maybe it'll occasionaly cause a slight hiccup when recieving a bitmap packet
// 
// Another idea make the bitmap packet recieving loop break
// If the frame timer is > 90%
// So frames will automatically be skipped if it's taking too long

// How long the rx function should block for
static software_timer_t block_timer;

// Initialises block timer with the timout_ms
// TUSB needs to be initialised by the caller
void init_cmd_io(uint16_t timeout_ms) {
    block_timer = construct_stimer_p(
        get_tick_frequency(),
        timeout_ms,
        HAL_GetTick(),
        ONESHOT_ST 
    );
}

// Returns true if the two command id's EG. "HI" and "AK" are the same
bool is_cmd_same(const char *cmd1_id, const char *cmd2_id) {
    if (!strncmp(cmd1_id, cmd2_id, COMMAND_CHARS)) {
        return true;
    }
    return false;
}

// Sends a usb command
// The command will be resent until ACK is recieved,
// unless ACK is disabled
void tx_usb_cmd_w_args(const char *cmd_id, cmd_arg_int *args_buf, bool disable_ack) {
    uint8_t command_buffer[COMMAND_LEN] = {0};
    memcpy(command_buffer, cmd_id, COMMAND_CHARS);
    memcpy(command_buffer + COMMAND_CHARS, (uint8_t*)args_buf, MAX_ARGS * sizeof(cmd_arg_int));

    if (disable_ack) {
        tud_cdc_n_write(CMD_CDC_PORT, command_buffer, COMMAND_LEN);
        tud_cdc_write_flush();
        return;
    }

    char cmd[] = ERROR_COMMAND;
    cmd_arg_int args[MAX_ARGS];

    while(!is_cmd_same(cmd, ACKNOWLEDGE) && is_usb_connected()) {
        tud_cdc_n_write(CMD_CDC_PORT, command_buffer, COMMAND_LEN);
        tud_cdc_write_flush();
        rx_usb_cmd(cmd, args, true, true);
    }
}

void tx_usb_cmd(const char *cmd_id, bool disable_ack) {
   cmd_arg_int no_args[MAX_ARGS] = {0};
   tx_usb_cmd_w_args(cmd_id, no_args, disable_ack);
}

// Update parameters when a command is recieved
// Will return an error command if the number of bytes recieved doesn't match the command length
// Or there are no bytes to read
// If blocking_read is true the read will block until the block timer has elapsed
// If disable_ack is true ack command sending upon succesfully recieving a command is disabled
// The return value is the number of bytes read, or the number of bytes available in the case of an error
void rx_usb_cmd(const char *cmd_id, cmd_arg_int *args_buf, bool blocking_read, bool disable_ack) {

    uint32_t bytes_available = 0;
    if (blocking_read) {
        reset_stimer(&block_timer, HAL_GetTick());
        while (!is_stimer_finished(&block_timer, HAL_GetTick()) & !bytes_available) {
            tud_task();
            bytes_available = tud_cdc_n_available(CMD_CDC_PORT);
            (void) 0;
        }
    } else {
        bytes_available = tud_cdc_n_available(CMD_CDC_PORT);
    }

    if (bytes_available != COMMAND_LEN) {
        memcpy((uint8_t*)cmd_id, ERROR_COMMAND, COMMAND_CHARS);
        tud_cdc_read_flush();
        args_buf[0] = bytes_available;
        return;
    }

    uint8_t rx_buf[COMMAND_LEN] = {0};
    tud_cdc_n_read(CMD_CDC_PORT, &rx_buf, COMMAND_LEN);
    memcpy((uint8_t*)cmd_id, rx_buf, COMMAND_CHARS);
    memcpy((uint8_t*)args_buf, rx_buf + COMMAND_CHARS, MAX_ARGS * sizeof(cmd_arg_int));

    if (!disable_ack) {
        tx_usb_cmd(ACKNOWLEDGE, true);
    }
    return;
}