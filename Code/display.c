#include <stdbool.h>
#include "display.h"
#include "stm32f4xx_hal.h"
#include "system.h"
#include "blocking_delay.h"

#define SPI_INSTANCE            SPI2
// #define SPI_BAUDRATE            SPI_BAUDRATEPRESCALER_128; // 42MHz (APB1) / 128 = 328 KHz
#define SPI_BAUDRATE            SPI_BAUDRATEPRESCALER_64; // 42MHz (APB1) / 128 = 656 KHz

#define SPI_CLK_EN              __HAL_RCC_SPI2_CLK_ENABLE
#define SPI_PIN_CLK_EN          __HAL_RCC_GPIOB_CLK_ENABLE
#define SPI_PIN_AF              GPIO_AF5_SPI2

#define SCK_PIN                 GPIO_PIN_10
#define SCK_PIN_BUS             GPIOB

#define MOSI_PIN                GPIO_PIN_15
#define MOSI_PIN_BUS            GPIOB

#define RST_PIN                 GPIO_PIN_12
#define RST_PIN_BUS             GPIOB

#define CS_PIN                  GPIO_PIN_11
#define CS_PIN_BUS              GPIOB

#define CS_IDLE_STATE 1 

static SPI_HandleTypeDef hspi_2;
static DMA_HandleTypeDef hdma;


#if (USE_U8G2 == 0)

static volatile bool is_tx_done = true;

static inline void block_until_last_disp_tx_cplt(void) {
    while (!is_tx_done) {
        (void) 0;
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
    volatile HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(&hspi_2, buf, (uint16_t)buf_len);
}

// Sends a buffer to the display over spi and blocks until complete
// This function also sets CS
void send_disp_buf_blocking(uint8_t *buf, uint32_t buf_len) {
    HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, !CS_IDLE_STATE);
    delay_us(1);
    HAL_SPI_Transmit(&hspi_2, buf, (uint16_t)buf_len, 10);
    delay_us(1);
    HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, CS_IDLE_STATE);
    delay_us(1);
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI_INSTANCE) {
        is_tx_done = true;
        HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, CS_IDLE_STATE);
    }
}

// Initialises peripherals for display, and writes startup sequence
void init_display(void) {
    SPI_PIN_CLK_EN();

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
    software_pin_cfg.Pin = RST_PIN;
    HAL_GPIO_Init(RST_PIN_BUS, &software_pin_cfg); // RST


    // Setup SPI
    SPI_CLK_EN();
    hspi_2.Instance = SPI_INSTANCE;
    hspi_2.Init.Mode = SPI_MODE_MASTER;
    hspi_2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi_2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi_2.Init.NSS = SPI_NSS_SOFT; // NSS is a chip select output, goes low during byte transfer
    hspi_2.Init.BaudRatePrescaler = SPI_BAUDRATE; 
    hspi_2.Init.FirstBit = SPI_FIRSTBIT_LSB;
    hspi_2.Init.TIMode = SPI_TIMODE_DISABLE;

    // Use mode 3
    hspi_2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi_2.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi_2.Init.CLKPhase = SPI_PHASE_2EDGE;

    // Send garbage byte over spi so clock idles properly
    HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, CS_IDLE_STATE);
    error_handler_msg(HAL_SPI_Init(&hspi_2), "Failed to init SPI");
    HAL_StatusTypeDef status = HAL_ERROR;
    uint8_t data[1] = {0};
    while (status == HAL_ERROR) {
        status = HAL_SPI_Transmit(&hspi_2, data, 1, 10);
    } 

    // Config DMA
    __HAL_RCC_DMA1_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 4, 1);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    hdma.Instance = DMA1_Stream4;
    hdma.Init.Channel = DMA_CHANNEL_0;
    hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma.Init.MemInc = DMA_MINC_ENABLE;
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma.Init.Mode = DMA_NORMAL;
    hdma.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    error_handler_msg(HAL_DMA_Init(&hdma), "Failed to init DMA");
    __HAL_LINKDMA(&hspi_2, hdmatx, hdma);

    HAL_GPIO_WritePin(RST_PIN_BUS, RST_PIN, GPIO_PIN_SET);
    delay_ms(1);

    gp1247ai_init(send_disp_buf_blocking);
}

extern void DMA1_Stream4_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma);
}
#endif

#if (USE_U8G2 == 1)
static uint8_t u8x8_byte_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static bool spi_setup = false;

    switch(msg) {
        case U8X8_MSG_BYTE_SEND:
            error_handler_msg(HAL_SPI_Transmit(&hspi_2, (uint8_t*)(arg_ptr), arg_int, 10), "SPI transfer failed");
            break;

        case U8X8_MSG_BYTE_INIT:
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            break;

        case U8X8_MSG_BYTE_SET_DC:
            // u8x8_gpio_SetDC(u8x8, arg_int); // Unused
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
        /* SPI mode has to be mapped to the mode of the current controller, at least Uno, Due, 101 have different SPI_MODEx values */

            if (!spi_setup) {
                spi_setup = true;
                switch(u8x8->display_info->spi_mode) {
                    case 0:
                        hspi_2.Init.CLKPolarity = SPI_POLARITY_LOW;
                        hspi_2.Init.CLKPhase = SPI_PHASE_1EDGE;
                        break;
                    case 1: 
                        hspi_2.Init.CLKPolarity = SPI_POLARITY_LOW;
                        hspi_2.Init.CLKPhase = SPI_PHASE_2EDGE;
                        break;
                    case 2:
                        hspi_2.Init.CLKPolarity = SPI_POLARITY_HIGH;
                        hspi_2.Init.CLKPhase = SPI_PHASE_1EDGE;
                        break;
                    case 3:
                        hspi_2.Init.CLKPolarity = SPI_POLARITY_HIGH;
                        hspi_2.Init.CLKPhase = SPI_PHASE_2EDGE;
                        break;
                }
                error_handler_msg(HAL_SPI_Init(&hspi_2), "Failed to init SPI");
            }

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
            software_pin_cfg.Pin = RST_PIN;
            HAL_GPIO_Init(RST_PIN_BUS, &software_pin_cfg); // RST

            // Setup SPI
            __HAL_RCC_SPI2_CLK_ENABLE();

            hspi_2.Instance = SPI_INSTANCE;
            hspi_2.Init.Mode = SPI_MODE_MASTER;
            hspi_2.Init.Direction = SPI_DIRECTION_2LINES;
            hspi_2.Init.DataSize = SPI_DATASIZE_8BIT;
            hspi_2.Init.NSS = SPI_NSS_SOFT; // NSS is a chip select output, goes low during byte transfer
            hspi_2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128; // 42MHz (APB1) / 128 = 328 KHz
            hspi_2.Init.FirstBit = SPI_FIRSTBIT_MSB;
            hspi_2.Init.TIMode = SPI_TIMODE_DISABLE;
            hspi_2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

            // Default mode 3
            hspi_2.Init.CLKPolarity = SPI_POLARITY_HIGH;
            hspi_2.Init.CLKPhase = SPI_PHASE_2EDGE;
            error_handler_msg(HAL_SPI_Init(&hspi_2), "Failed to init SPI");
            break;

        case U8X8_MSG_DELAY_100NANO:
            if (arg_int < 10) wait_us(1);
            else wait_us(((uint32_t)arg_int + 5) / 10);
            break;

        case U8X8_MSG_DELAY_10MICRO:
            wait_us((uint32_t)arg_int * 10);
            break;

        case U8X8_MSG_DELAY_MILLI:
            wait_ms((uint32_t)arg_int);
            break;

        case U8X8_MSG_GPIO_SPI_CLOCK:
            HAL_GPIO_WritePin(SCK_PIN_BUS, SCK_PIN, arg_int);
            break;

        case U8X8_MSG_GPIO_SPI_DATA:
            HAL_GPIO_WritePin(MOSI_PIN_BUS, MOSI_PIN, arg_int);
            break;

        case U8X8_MSG_GPIO_CS:
            HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, arg_int);
            break;

        case U8X8_MSG_GPIO_RESET:
            HAL_GPIO_WritePin(RST_PIN_BUS, RST_PIN, arg_int);
            break;

        default:
            u8x8_SetGPIOResult(u8x8, 1);			// default return value
            break;
    }
    return 1;
}


void init_display(u8g2_t* u8g2, const u8g2_cb_t *rotation) {
    u8g2_Setup_gp1247ai_253x63_f(u8g2, rotation, u8x8_byte_hw_spi, u8x8_gpio_and_delay);  
    u8g2_InitDisplay(u8g2); // send init sequence to the display, display is in sleep mode after this,
}
#endif