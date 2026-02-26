#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "usb_cmd_cfg.h"

#define CMD_CDC_PORT 0

void init_cmd_io(uint16_t timeout_ms);
bool is_cmd_same(const char *cmd1_id, const char *cmd2_id);
void tx_usb_cmd_w_args(const char *cmd_id, cmd_arg_int *args_buf, bool disable_ack);
void tx_usb_cmd(const char *cmd_id, bool disable_ack);
void rx_usb_cmd(const char *cmd_id, cmd_arg_int *args_buf, bool blocking_read, bool disable_ack);