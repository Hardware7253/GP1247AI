#pragma once

#include "tusb.h"
#include <stdbool.h>

void init_usb(void);
bool is_usb_connected(void);