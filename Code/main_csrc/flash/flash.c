#include "flash.h"
#include "stm32f4xx_hal.h"
#include "blocking_delay.h"
#include "system.h"

#define SPI_INSTANCE            SPI3
#define SPI_BAUDRATE            SPI_BAUDRATEPRESCALER_32 // 42MHz (APB1) / 32 = 1.3125 MHz 

#define SPI_CLK_EN              __HAL_RCC_SPI3_CLK_ENABLE
#define SPI_PIN_CLK_EN          __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE
#define SPI_PIN_AF              GPIO_AF6_SPI3

#define SCK_PIN                 GPIO_PIN_10
#define SCK_PIN_BUS             GPIOC

#define MOSI_PIN                GPIO_PIN_12
#define MOSI_PIN_BUS            GPIOC

#define MISO_PIN                GPIO_PIN_11
#define MISO_PIN_BUS            GPIOC

#define CS_PIN                  GPIO_PIN_15
#define CS_PIN_BUS              GPIOA

#define CS_IDLE_STATE 1 

#define TX_RX_TIMEOUT 100 /* In ms. Timeout needs to be long enough to support a full page write or arbitrary read*/

static SPI_HandleTypeDef hspi;
// static DMA_HandleTypeDef hdma_rx;
// static DMA_HandleTypeDef hdma_tx;

void set_cs(bool is_not_idle) {
    HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, is_not_idle ? !CS_IDLE_STATE : CS_IDLE_STATE);
}

void flash_tx(uint8_t *tx_buf, uint32_t tx_buf_len) {
    delay_us(1);
    HAL_SPI_Transmit(&hspi, tx_buf, (uint16_t)tx_buf_len, TX_RX_TIMEOUT);
    delay_us(1);
}

void flash_rx(uint8_t *rx_buf, uint32_t rx_buf_len) {
    delay_us(1);
    HAL_SPI_Receive(&hspi, rx_buf, (uint16_t)rx_buf_len, TX_RX_TIMEOUT);
    delay_us(1);
}

// Initialise SPI peripheral, pins, and c struct for the flash chip
void flash_init(flash_t *flash) {
    init_blocking_delay();
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
    spi_pin_cfg.Pin = MISO_PIN;
    spi_pin_cfg.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(MISO_PIN_BUS, &spi_pin_cfg); // MISO 

    spi_pin_cfg = (GPIO_InitTypeDef){
        .Pin       = CS_PIN,
        .Mode      = GPIO_MODE_OUTPUT_PP,
        .Pull      = GPIO_NOPULL,
        .Speed     = GPIO_SPEED_FREQ_MEDIUM,
        .Alternate = 0,
    };
    HAL_GPIO_Init(CS_PIN_BUS, &spi_pin_cfg); // CS

    // Setup SPI
    SPI_CLK_EN();
    hspi.Instance = SPI_INSTANCE;
    hspi.Init.Mode = SPI_MODE_MASTER;
    hspi.Init.Direction = SPI_DIRECTION_2LINES;
    hspi.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi.Init.NSS = SPI_NSS_SOFT; // NSS is a chip select output, goes low during byte transfer
    hspi.Init.BaudRatePrescaler = SPI_BAUDRATE; 
    hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi.Init.TIMode = SPI_TIMODE_DISABLE;

    // Use mode 0
    hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi.Init.CLKPhase = SPI_PHASE_1EDGE;

    // Send garbage byte over spi so clock idles properly
    HAL_GPIO_WritePin(CS_PIN_BUS, CS_PIN, CS_IDLE_STATE);
    error_handler_msg(HAL_SPI_Init(&hspi), "Failed to init SPI");
    HAL_StatusTypeDef status = HAL_ERROR;
    while (status == HAL_ERROR) {
        status = HAL_SPI_Transmit(&hspi, (uint8_t[]){0}, 1, 10);
    } 

    // Config RX DMA
    // __HAL_RCC_DMA2_CLK_ENABLE();
    // HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 4, 1);
    // HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    // hdma_rx.Instance = DMA1_Stream2;
    // hdma_rx.Init.Channel = DMA_CHANNEL_0;
    // hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    // hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    // hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
    // hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    // hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    // hdma_rx.Init.Mode = DMA_NORMAL;
    // hdma_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    // hdma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    // error_handler_msg(HAL_DMA_Init(&hdma_rx), "Failed to init DMA");
    // __HAL_LINKDMA(&hspi, hdmarx, hdma_rx);

    // Config TX DMA
    // __HAL_RCC_DMA2_CLK_ENABLE();
    // HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 4, 1);
    // HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    // hdma_tx.Instance = DMA1_Stream2;
    // hdma_tx.Init.Channel = DMA_CHANNEL_0;
    // hdma_tx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    // hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    // hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
    // hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    // hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    // hdma_tx.Init.Mode = DMA_NORMAL;
    // hdma_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    // hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    // error_handler_msg(HAL_DMA_Init(&hdma_tx), "Failed to init DMA");
    // __HAL_LINKDMA(&hspi, hdmatx, hdma_tx);

    delay_ms(1);
    w25q_init(flash, flash_tx, flash_rx, set_cs);
    delay_ms(1);
}