#include <stdbool.h>
#include "display.h"
#include "stm32f4xx_hal.h"
#include "system.h"
#include "blocking_delay.h"
#include "bit.h"

#define SPI_INSTANCE            SPI2
#define SPI_BAUDRATE            SPI_BAUDRATEPRESCALER_32 // 42MHz (APB1) / 32 = 1.3125 MHz 

#define SPI_CLK_EN              __HAL_RCC_SPI2_CLK_ENABLE
#define SPI_PIN_CLK_EN          __HAL_RCC_GPIOB_CLK_ENABLE
#define SPI_PIN_AF              GPIO_AF5_SPI2

#define SCK_PIN                 GPIO_PIN_10
#define SCK_PIN_BUS             GPIOB

#define MOSI_PIN                GPIO_PIN_15
#define MOSI_PIN_BUS            GPIOB

#define CS_PIN                  GPIO_PIN_12
#define CS_PIN_BUS              GPIOB

#define DMA_CLK_EN              __HAL_RCC_DMA1_CLK_ENABLE
#define DMA_IRQN                DMA1_Stream4_IRQn
#define DMA_IRQHANDLER          DMA1_Stream4_IRQHandler
#define DMA_INSTANCE            DMA1_Stream4
#define DMA_CHANNEL             DMA_CHANNEL_0

#define CS_IDLE_STATE 1 

static SPI_HandleTypeDef hspi;
static DMA_HandleTypeDef hdma;

static volatile bool is_tx_done = true;

static inline void block_until_last_disp_tx_cplt(void) {
    while (!is_tx_done) {
        (void) 0;
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *_hspi) {
    if (_hspi->Instance == SPI_INSTANCE) {
        is_tx_done = true;
        HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, CS_IDLE_STATE);
    }
}

extern void DMA_IRQHANDLER(void) {
    HAL_DMA_IRQHandler(&hdma);
}

// Transmit a buffer over spi
// This will block until the HAL is not busy
// So blocking spi transfers will wait until the non blocking DMA transfer is complete
static void spi_tx(uint8_t *buf, uint16_t len) {
    HAL_StatusTypeDef status = HAL_BUSY;
    while (status != HAL_OK) {
        status = HAL_SPI_Transmit(&hspi, buf, len, 10);
    }
}

// Sends a buffer to the display over spi without blocking
// This function can still block if the last transfer isn't done,
// So it will act as a blocking function if it is called back to back
// This function also sets CS
void send_disp_buf(uint8_t *buf, uint32_t buf_len) {
    block_until_last_disp_tx_cplt();
    is_tx_done = false;
    HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, !CS_IDLE_STATE);
    volatile HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(&hspi, buf, (uint16_t)buf_len);
}

// Sends a buffer to the display over spi and blocks until complete
// This function also sets CS
void send_disp_buf_blocking(uint8_t *buf, uint32_t buf_len) {
    HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, !CS_IDLE_STATE);
    delay_us(1);
    spi_tx(buf, (uint16_t)buf_len);
    delay_us(1);
    HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, CS_IDLE_STATE);
    delay_us(1);
}

static uint8_t u8x8_byte_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static bool spi_setup = false;

    switch(msg) {
        case U8X8_MSG_BYTE_SEND:
            uint8_t arg = RBIT(*(uint8_t*)arg_ptr);
            spi_tx(&arg, (uint16_t)arg_int);
            break;

        case U8X8_MSG_BYTE_INIT:
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            break;

        case U8X8_MSG_BYTE_SET_DC:
            // u8x8_gpio_SetDC(u8x8, arg_int); // Unused
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);  
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:      
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            break;

        default:
            return 0;
    }  
    return 1;
}

static uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    (void) arg_ptr;
    switch(msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            SPI_PIN_CLK_EN();
            init_blocking_delay();
            delay_ms(1);

            GPIO_InitTypeDef spi_pin_cfg = {
                .Pin       = SCK_PIN,
                .Mode      = GPIO_MODE_AF_PP,
                .Pull      = GPIO_NOPULL,
                .Speed     = GPIO_SPEED_FREQ_MEDIUM,
                .Alternate = SPI_PIN_AF,
            };
            HAL_GPIO_Init(SCK_PIN_BUS, &spi_pin_cfg); // CLK
            spi_pin_cfg.Pin = MOSI_PIN;
            HAL_GPIO_Init(MOSI_PIN_BUS, &spi_pin_cfg); // MOSI

            GPIO_InitTypeDef software_pin_cfg = {
                .Pin       = CS_PIN,
                .Mode      = GPIO_MODE_OUTPUT_PP,
                .Pull      = GPIO_NOPULL,
                .Speed     = GPIO_SPEED_FREQ_MEDIUM,
                .Alternate = 0,
            };
            HAL_GPIO_Init(CS_PIN_BUS, &software_pin_cfg); // CS

            // Setup SPI
            SPI_CLK_EN();
            hspi.Instance = SPI_INSTANCE;
            hspi.Init.Mode = SPI_MODE_MASTER;
            hspi.Init.Direction = SPI_DIRECTION_2LINES;
            hspi.Init.DataSize = SPI_DATASIZE_8BIT;
            hspi.Init.NSS = SPI_NSS_SOFT; 
            hspi.Init.BaudRatePrescaler = SPI_BAUDRATE; 
            hspi.Init.FirstBit = SPI_FIRSTBIT_LSB;
            hspi.Init.TIMode = SPI_TIMODE_DISABLE;
            hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

            // Use mode 3 
            hspi.Init.CLKPolarity = SPI_POLARITY_HIGH;
            hspi.Init.CLKPhase = SPI_PHASE_2EDGE;

            // Config DMA
            DMA_CLK_EN();
            HAL_NVIC_SetPriority(DMA_IRQN, 4, 1);
            HAL_NVIC_EnableIRQ(DMA_IRQN);
            hdma.Instance = DMA_INSTANCE;
            hdma.Init.Channel = DMA_CHANNEL;
            hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
            hdma.Init.PeriphInc = DMA_PINC_DISABLE;
            hdma.Init.MemInc = DMA_MINC_ENABLE;
            hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
            hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
            hdma.Init.Mode = DMA_NORMAL;
            hdma.Init.Priority = DMA_PRIORITY_MEDIUM;
            hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
            error_handler_msg(HAL_DMA_Init(&hdma), "Failed to init DMA");
            __HAL_LINKDMA(&hspi, hdmatx, hdma);

            // Send garbage bytes over spi so clock idles properly
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            error_handler_msg(HAL_SPI_Init(&hspi), "Failed to init SPI");
            HAL_SPI_Transmit(&hspi, (uint8_t[]){0}, 1, 10);
            delay_us(1);

            break;

        case U8X8_MSG_DELAY_100NANO:
            if (arg_int < 10) delay_us(1);
            else delay_us(((uint32_t)arg_int + 5) / 10);
            break;

        case U8X8_MSG_DELAY_10MICRO:
            delay_us((uint32_t)arg_int * 10);
            break;

        case U8X8_MSG_DELAY_MILLI:
            delay_ms((uint32_t)arg_int);
            break;

        case U8X8_MSG_GPIO_CS:
            HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, arg_int);
            break;

        default:
            u8x8_SetGPIOResult(u8x8, 1);			// default return value
            break;
    }
    return 1;
}


// Initialises u8g2 and custom gp1247ai library
// The custom gp1247ai library can be used for writing bitmaps quickly with DMA
// Or u8g2 can be used for more advanced graphics
void init_display(u8g2_t* u8g2, const u8g2_cb_t *rotation) {
    u8g2_Setup_gp1247ai_253x63_f(u8g2, rotation, u8x8_byte_hw_spi, u8x8_gpio_and_delay);  
    u8g2_InitDisplay(u8g2); 
    gp1247ai_init(send_disp_buf_blocking);
}

// Used to change the displays rotation after it has already been initialised
void change_display_rotation(u8g2_t* u8g2, const u8g2_cb_t *rotation) {
    u8g2_Setup_gp1247ai_253x63_f(u8g2, rotation, u8x8_byte_hw_spi, u8x8_gpio_and_delay);  
}