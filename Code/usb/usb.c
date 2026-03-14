#include "stm32f4xx_hal.h"
#include "usb.h"
#include "system.h"

static bool usb_connected = false;

// Initialise USB pins, clocks, and interrupt
void init_usb(void) {
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    // Config DP and DM
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef pin_cfg = {
        .Pin =  GPIO_PIN_11 | GPIO_PIN_12,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_MEDIUM,
        .Alternate = GPIO_AF10_OTG_FS,
    };
    HAL_GPIO_Init(GPIOA, &pin_cfg); 

    PCD_HandleTypeDef hpcd;
    hpcd.Instance = USB_OTG_FS;
    hpcd.Init.dev_endpoints = 4;
    hpcd.Init.speed = PCD_SPEED_FULL;
    hpcd.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd.Init.Sof_enable = DISABLE;
    hpcd.Init.low_power_enable = DISABLE;
    hpcd.Init.vbus_sensing_enable = DISABLE;
    hpcd.Init.use_dedicated_ep1 = DISABLE;
    error_handler_msg(HAL_PCD_Init(&hpcd), "Failed to init PCD");

    tusb_rhport_init_t usb_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };
    tusb_init(0, &usb_init);

    HAL_NVIC_SetPriority(OTG_FS_IRQn, 2, 1);
    NVIC_EnableIRQ(OTG_FS_IRQn);
}

bool is_usb_connected(void) {
    return usb_connected;
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void) itf;
    (void) rts;
    usb_connected = dtr;
}

extern void OTG_FS_IRQHandler(void) {
    tusb_int_handler(0, true);
}