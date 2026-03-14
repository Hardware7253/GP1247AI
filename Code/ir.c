// #define USE_THIS_DEFAULT_HANDLR
#ifdef USE_THIS_DEFAULT_HANDLR

#define WEAK __attribute__((weak))
#define ALIAS(f) __attribute__((alias(#f)))


void def_handler(void)
{
    volatile unsigned int stay_here = 1;
    while (stay_here) {
        __asm volatile ("nop");
    }
}

WEAK void WWDG_IRQHandler(void)              ALIAS(def_handler);
WEAK void PVD_IRQHandler(void)               ALIAS(def_handler);
WEAK void TAMP_STAMP_IRQHandler(void)        ALIAS(def_handler);
WEAK void RTC_WKUP_IRQHandler(void)          ALIAS(def_handler);
WEAK void FLASH_IRQHandler(void)             ALIAS(def_handler);
WEAK void RCC_IRQHandler(void)               ALIAS(def_handler);
WEAK void EXTI0_IRQHandler(void)             ALIAS(def_handler);
WEAK void EXTI1_IRQHandler(void)             ALIAS(def_handler);
WEAK void EXTI2_IRQHandler(void)             ALIAS(def_handler);
WEAK void EXTI3_IRQHandler(void)             ALIAS(def_handler);
WEAK void EXTI4_IRQHandler(void)             ALIAS(def_handler);
WEAK void DMA1_Stream0_IRQHandler(void)      ALIAS(def_handler);
WEAK void DMA1_Stream1_IRQHandler(void)      ALIAS(def_handler);
WEAK void DMA1_Stream2_IRQHandler(void)      ALIAS(def_handler);
WEAK void DMA1_Stream3_IRQHandler(void)      ALIAS(def_handler);
WEAK void DMA1_Stream4_IRQHandler(void)      ALIAS(def_handler);
WEAK void DMA1_Stream5_IRQHandler(void)      ALIAS(def_handler);
WEAK void DMA1_Stream6_IRQHandler(void)      ALIAS(def_handler);
WEAK void ADC_IRQHandler(void)               ALIAS(def_handler);
WEAK void CAN1_TX_IRQHandler(void)            ALIAS(def_handler);
WEAK void CAN1_RX0_IRQHandler(void)           ALIAS(def_handler);
WEAK void CAN1_RX1_IRQHandler(void)           ALIAS(def_handler);
WEAK void CAN1_SCE_IRQHandler(void)           ALIAS(def_handler);
WEAK void EXTI9_5_IRQHandler(void)            ALIAS(def_handler);
WEAK void TIM1_BRK_TIM9_IRQHandler(void)      ALIAS(def_handler);
WEAK void TIM1_UP_TIM10_IRQHandler(void)      ALIAS(def_handler);
WEAK void TIM1_TRG_COM_TIM11_IRQHandler(void) ALIAS(def_handler);
WEAK void TIM1_CC_IRQHandler(void)            ALIAS(def_handler);
WEAK void TIM2_IRQHandler(void)               ALIAS(def_handler);
WEAK void TIM3_IRQHandler(void)               ALIAS(def_handler);
WEAK void TIM4_IRQHandler(void)               ALIAS(def_handler);
WEAK void I2C1_EV_IRQHandler(void)            ALIAS(def_handler);
WEAK void I2C1_ER_IRQHandler(void)            ALIAS(def_handler);
WEAK void I2C2_EV_IRQHandler(void)            ALIAS(def_handler);
WEAK void I2C2_ER_IRQHandler(void)            ALIAS(def_handler);
WEAK void SPI1_IRQHandler(void)               ALIAS(def_handler);
WEAK void SPI2_IRQHandler(void)               ALIAS(def_handler);
WEAK void USART1_IRQHandler(void)             ALIAS(def_handler);
WEAK void USART2_IRQHandler(void)             ALIAS(def_handler);
WEAK void USART3_IRQHandler(void)             ALIAS(def_handler);
WEAK void EXTI15_10_IRQHandler(void)           ALIAS(def_handler);
WEAK void RTC_Alarm_IRQHandler(void)           ALIAS(def_handler);
WEAK void OTG_FS_WKUP_IRQHandler(void)         ALIAS(def_handler);
WEAK void TIM8_BRK_TIM12_IRQHandler(void)      ALIAS(def_handler);
WEAK void TIM8_UP_TIM13_IRQHandler(void)       ALIAS(def_handler);
WEAK void TIM8_TRG_COM_TIM14_IRQHandler(void)  ALIAS(def_handler);
WEAK void TIM8_CC_IRQHandler(void)             ALIAS(def_handler);
WEAK void DMA1_Stream7_IRQHandler(void)        ALIAS(def_handler);
WEAK void FSMC_IRQHandler(void)                ALIAS(def_handler);
WEAK void SDIO_IRQHandler(void)                ALIAS(def_handler);
WEAK void TIM5_IRQHandler(void)                ALIAS(def_handler);
WEAK void SPI3_IRQHandler(void)                ALIAS(def_handler);
WEAK void UART4_IRQHandler(void)               ALIAS(def_handler);
WEAK void UART5_IRQHandler(void)               ALIAS(def_handler);
WEAK void TIM6_DAC_IRQHandler(void)             ALIAS(def_handler);
WEAK void TIM7_IRQHandler(void)                ALIAS(def_handler);
WEAK void DMA2_Stream0_IRQHandler(void)        ALIAS(def_handler);
WEAK void DMA2_Stream1_IRQHandler(void)        ALIAS(def_handler);
WEAK void DMA2_Stream2_IRQHandler(void)        ALIAS(def_handler);
WEAK void DMA2_Stream3_IRQHandler(void)        ALIAS(def_handler);
WEAK void DMA2_Stream4_IRQHandler(void)        ALIAS(def_handler);
WEAK void ETH_IRQHandler(void)                 ALIAS(def_handler);
WEAK void ETH_WKUP_IRQHandler(void)            ALIAS(def_handler);
WEAK void CAN2_TX_IRQHandler(void)             ALIAS(def_handler);
WEAK void CAN2_RX0_IRQHandler(void)            ALIAS(def_handler);
WEAK void CAN2_RX1_IRQHandler(void)            ALIAS(def_handler);
WEAK void CAN2_SCE_IRQHandler(void)            ALIAS(def_handler);
WEAK void OTG_FS_IRQHandler(void)              ALIAS(def_handler);
WEAK void DMA2_Stream5_IRQHandler(void)        ALIAS(def_handler);
WEAK void DMA2_Stream6_IRQHandler(void)        ALIAS(def_handler);
WEAK void DMA2_Stream7_IRQHandler(void)        ALIAS(def_handler);
WEAK void USART6_IRQHandler(void)              ALIAS(def_handler);
WEAK void I2C3_EV_IRQHandler(void)             ALIAS(def_handler);
WEAK void I2C3_ER_IRQHandler(void)             ALIAS(def_handler);
WEAK void OTG_HS_EP1_OUT_IRQHandler(void)      ALIAS(def_handler);
WEAK void OTG_HS_EP1_IN_IRQHandler(void)       ALIAS(def_handler);
WEAK void OTG_HS_WKUP_IRQHandler(void)         ALIAS(def_handler);
WEAK void OTG_HS_IRQHandler(void)              ALIAS(def_handler);
WEAK void DCMI_IRQHandler(void)                ALIAS(def_handler);
WEAK void HASH_RNG_IRQHandler(void)            ALIAS(def_handler);
WEAK void FPU_IRQHandler(void)                 ALIAS(def_handler);
#endif