#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "w25q.h"

typedef w25q_t flash_t;

void set_cs(bool is_not_idle);
void flash_tx(uint8_t *tx_buf, uint32_t tx_buf_len);
void flash_rx(uint8_t *rx_buf, uint32_t rx_buf_len);
void flash_init(flash_t *flash);