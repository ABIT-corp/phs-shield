#include <string.h>
#include <stdio.h>

#include "phs_shield.h"
#include "stm32f4xx_hal.h"
#include "modem.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "rtc.h"
#include "watchdog_timer.h"
#include "phs_shield_uart.h"

struct_psld psld;

static struct_memory_map memory_map [] = {
    {0x00000000, 0x000fffff},
    {0x08000000, 0x080fffff},
    {0x10000000, 0x1000ffff},
    {0x1fff0000, 0x1fff7a0f},
    {0x1fffc000, 0x1fffc007},
    {0x20000000, 0x2001ffff},
    {0x40000000, 0x40007fff},
    {0x40010000, 0x400157ff},
    {0x40020000, 0x4007ffff},
    {0x50000000, 0x50060bff},
    {0x60000000, 0xa0000fff},
    {0xe0000000, 0xe000ffff},
    {0, 0}
};

struct_device_list device_list [] = {
    {"/dev/tty0",    "uart1", 1, &psld.uart_handle[0]},
    {"/dev/ttyACM0", "uart2", 2, &psld.uart_handle[1]},
    {"/dev/tty1",    "uart5", 5, &psld.uart_handle[2]},
    {0, 0, 0, 0}
};

extern xSemaphoreHandle semaphore_sio;



/*
 * global variable configuration
 * @param : None
 * @return: None
 */
void global_configuration()
{
    psld.memory_map = memory_map;
    psld.memory_map_size = sizeof(memory_map) / sizeof(memory_map[0]);

    psld.device_list = device_list;
    psld.device_list_size = sizeof(device_list) / sizeof(device_list[0]);

    psld.direct_channel_modem = 0;
    psld.direct_channel_arduino = 0;

    psld.modem_powerup_flag = 0;
    psld.modem_power_enable_flag = 0;
    psld.modem_status = MODEM_POWER_OFF;
    psld.profile_number_changed = 0;
    psld.profile_number = PROFILE_PPP_LINKUP;
    psld.ppp_linkup_retry = 0;
    psld.tcpip_connect_retry = 0;
}

/*
 * GPIO pin configuration
 * @param : None
 * @return: None
 */
void gpio_configuration()
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    /* Enable GPIOA Clock  */
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();

    /* Configure LED1 pin */
    GPIO_InitStruct.Pin = LED1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

    /* Configure LED2 pin */
    GPIO_InitStruct.Pin = LED2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

    /* Configure modem Power */
    HAL_GPIO_WritePin(MODEM_POWER_PORT, MODEM_POWER_PIN , 1);
    GPIO_InitStruct.Pin = MODEM_POWER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(MODEM_POWER_PORT, &GPIO_InitStruct);

    /* Configure modem Reset */
    GPIO_InitStruct.Pin = MODEM_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(MODEM_RESET_PORT, &GPIO_InitStruct);

    /* Configure Option UART */
    GPIO_InitStruct.Pin       = UART2_DCD_PIN ;
    GPIO_InitStruct.Mode      = UART2_DCD_MODE;
    HAL_GPIO_Init(UART2_DCD_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = UART2_DTR_PIN ;
    GPIO_InitStruct.Mode      = UART2_DTR_MODE;
    HAL_GPIO_Init(UART2_DTR_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = UART2_DSR_PIN ;
    GPIO_InitStruct.Mode      = UART2_DSR_MODE;
    HAL_GPIO_Init(UART2_DSR_PORT, &GPIO_InitStruct);
    /* Configure Option RI as interrupt */
    GPIO_InitStruct.Pin       = UART2_RI_PIN ;
    GPIO_InitStruct.Mode      = GPIO_MODE_IT_RISING;
    HAL_GPIO_Init(UART2_RI_PORT, &GPIO_InitStruct);

    /* Configure Option Arduino */
    GPIO_InitStruct.Pin       = ARDUINO_REGON_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
    HAL_GPIO_Init(ARDUINO_REGON_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = ARDUINO_POWERON_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
    HAL_GPIO_Init(ARDUINO_POWERON_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(ARDUINO_INTERRUPT_PORT, ARDUINO_INTERRUPT_PIN , 1);
    GPIO_InitStruct.Pin       = ARDUINO_INTERRUPT_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(ARDUINO_INTERRUPT_PORT, &GPIO_InitStruct);
}


/*
 * UART configuration
 *     UART1: Debug purpose
 *     UART2: Modem(APM-002) interface
 *     UART5: Arduino interface
 * @param : None
 * @return: None
 */
void uart_configuration(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    uint32_t baudrate;

    /* Configure UART pin */
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Pin       = USART1_TX_PIN;
    GPIO_InitStruct.Alternate = USART1_AF;
    HAL_GPIO_Init(USART1_TX_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = USART1_RX_PIN;
    GPIO_InitStruct.Alternate = USART1_AF;
    HAL_GPIO_Init(USART1_RX_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = USART2_TX_PIN;
    GPIO_InitStruct.Alternate = USART2_AF;
    HAL_GPIO_Init(USART2_TX_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = USART2_RX_PIN;
    GPIO_InitStruct.Alternate = USART2_AF;
    HAL_GPIO_Init(USART2_RX_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = UART2_CTS_PIN ;
    GPIO_InitStruct.Alternate = USART2_AF;
    HAL_GPIO_Init(UART2_CTS_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = UART2_RTS_PIN ;
    GPIO_InitStruct.Alternate = USART2_AF;
    HAL_GPIO_Init(UART2_RTS_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = USART5_TX_PIN;
    GPIO_InitStruct.Alternate = USART5_AF;
    HAL_GPIO_Init(USART5_TX_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = USART5_RX_PIN;
    GPIO_InitStruct.Alternate = USART5_AF;
    HAL_GPIO_Init(USART5_RX_GPIO_PORT, &GPIO_InitStruct);

    /* Enable Clock */
    __USART1_CLK_ENABLE();
    __USART2_CLK_ENABLE();
    __UART5_CLK_ENABLE();
    __DMA1_CLK_ENABLE();
    __DMA2_CLK_ENABLE();

    /* Configure UART1 peripheral */
    baudrate = 115200;
    psld.uart1_handle.Instance        = USART1;
    psld.uart1_handle.Init.BaudRate   = baudrate;
    psld.uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;
    psld.uart1_handle.Init.StopBits   = UART_STOPBITS_1;
    psld.uart1_handle.Init.Parity     = UART_PARITY_NONE;
    psld.uart1_handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    psld.uart1_handle.Init.Mode       = UART_MODE_TX_RX;
    if(HAL_UART_Init(&psld.uart1_handle) != HAL_OK)
    {
      error_handler("UART1");
    }

    /* Configure UART2 peripheral */
    baudrate = 115200;
    psld.uart2_handle.Instance        = USART2;
    psld.uart2_handle.Init.BaudRate   = baudrate;
    psld.uart2_handle.Init.WordLength = UART_WORDLENGTH_8B;
    psld.uart2_handle.Init.StopBits   = UART_STOPBITS_1;
    psld.uart2_handle.Init.Parity     = UART_PARITY_NONE;
    psld.uart2_handle.Init.HwFlowCtl  = UART_HWCONTROL_RTS_CTS;
    psld.uart2_handle.Init.Mode       = UART_MODE_TX_RX;
    if(HAL_UART_Init(&psld.uart2_handle) != HAL_OK)
    {
      error_handler("UART2");
    }

    /* Configure UART5 peripheral */
    baudrate = 9600;
    psld.uart5_handle.Instance        = UART5;
    psld.uart5_handle.Init.BaudRate   = baudrate;
    psld.uart5_handle.Init.WordLength = UART_WORDLENGTH_8B;
    psld.uart5_handle.Init.StopBits   = UART_STOPBITS_1;
    psld.uart5_handle.Init.Parity     = UART_PARITY_NONE;
    psld.uart5_handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    psld.uart5_handle.Init.Mode       = UART_MODE_TX_RX;
    if(HAL_UART_Init(&psld.uart5_handle) != HAL_OK)
    {
      error_handler("UART5");
    }

    /* Configuration Peripheral DMA(UART1) */
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)&psld.uart1_handle;

    psld.uart1dmatx_handle.Instance = DMA2_Stream7;
    psld.uart1dmatx_handle.Init.Channel = DMA_CHANNEL_4;
    psld.uart1dmatx_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    psld.uart1dmatx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    psld.uart1dmatx_handle.Init.MemInc = DMA_MINC_ENABLE;
    psld.uart1dmatx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    psld.uart1dmatx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    psld.uart1dmatx_handle.Init.Mode = DMA_NORMAL;//DMA_CIRCULAR;
    psld.uart1dmatx_handle.Init.Priority = DMA_PRIORITY_LOW;
    psld.uart1dmatx_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    psld.uart1dmatx_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    psld.uart1dmatx_handle.Init.MemBurst = DMA_MBURST_SINGLE;
    psld.uart1dmatx_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
    if(HAL_DMA_Init(&psld.uart1dmatx_handle) != HAL_OK)
    {
      error_handler("Uart1DmaTx");
    }
    __HAL_LINKDMA(huart, hdmatx, psld.uart1dmatx_handle);

    psld.uart1dmarx_handle.Instance = DMA2_Stream5;
    psld.uart1dmarx_handle.Init.Channel = DMA_CHANNEL_4;
    psld.uart1dmarx_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    psld.uart1dmarx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    psld.uart1dmarx_handle.Init.MemInc = DMA_MINC_ENABLE;
    psld.uart1dmarx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    psld.uart1dmarx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    psld.uart1dmarx_handle.Init.Mode = DMA_CIRCULAR;
    psld.uart1dmarx_handle.Init.Priority = DMA_PRIORITY_LOW;
    psld.uart1dmarx_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    psld.uart1dmarx_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    psld.uart1dmarx_handle.Init.MemBurst = DMA_MBURST_SINGLE;
    psld.uart1dmarx_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
    if(HAL_DMA_Init(&psld.uart1dmarx_handle) != HAL_OK)
    {
      error_handler("Uart1DmaRx");
    }
    __HAL_LINKDMA(huart, hdmarx, psld.uart1dmarx_handle);

    HAL_UART_Receive_DMA(huart, psld.uart1_rx_buffer, UART1_RXBUFF_SIZE);
    //p_uart1rxtail = uart1_rx_buffer;

    /* Configuration Peripheral DMA(UART2) */
    huart = (UART_HandleTypeDef *)&psld.uart2_handle;

    psld.uart2dmatx_handle.Instance = DMA1_Stream6;
    psld.uart2dmatx_handle.Init.Channel = DMA_CHANNEL_4;
    psld.uart2dmatx_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    psld.uart2dmatx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    psld.uart2dmatx_handle.Init.MemInc = DMA_MINC_ENABLE;
    psld.uart2dmatx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    psld.uart2dmatx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    psld.uart2dmatx_handle.Init.Mode = DMA_NORMAL;//DMA_CIRCULAR;
    psld.uart2dmatx_handle.Init.Priority = DMA_PRIORITY_LOW;
    psld.uart2dmatx_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    psld.uart2dmatx_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    psld.uart2dmatx_handle.Init.MemBurst = DMA_MBURST_SINGLE;
    psld.uart2dmatx_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
    if(HAL_DMA_Init(&psld.uart2dmatx_handle) != HAL_OK)
    {
      error_handler("Uart2DmaTx");
    }
    __HAL_LINKDMA(huart, hdmatx, psld.uart2dmatx_handle);

    psld.uart2dmarx_handle.Instance = DMA1_Stream5;
    psld.uart2dmarx_handle.Init.Channel = DMA_CHANNEL_4;
    psld.uart2dmarx_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    psld.uart2dmarx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    psld.uart2dmarx_handle.Init.MemInc = DMA_MINC_ENABLE;
    psld.uart2dmarx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    psld.uart2dmarx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    psld.uart2dmarx_handle.Init.Mode = DMA_CIRCULAR;
    psld.uart2dmarx_handle.Init.Priority = DMA_PRIORITY_LOW;
    psld.uart2dmarx_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    psld.uart2dmarx_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    psld.uart2dmarx_handle.Init.MemBurst = DMA_MBURST_SINGLE;
    psld.uart2dmarx_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
    if(HAL_DMA_Init(&psld.uart2dmarx_handle) != HAL_OK)
    {
      error_handler("Uart2DmaRx");
    }
    __HAL_LINKDMA(huart, hdmarx, psld.uart2dmarx_handle);

    HAL_UART_Receive_DMA(huart, psld.uart2_rx_buffer, UART2_RXBUFF_SIZE);
    //p_uart2rxtail = uart2_rx_buffer;

    /* Configuration Peripheral DMA(UART5) */
    huart = (UART_HandleTypeDef *)&psld.uart5_handle;

    psld.uart5dmatx_handle.Instance = DMA1_Stream7;
    psld.uart5dmatx_handle.Init.Channel = DMA_CHANNEL_4;
    psld.uart5dmatx_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    psld.uart5dmatx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    psld.uart5dmatx_handle.Init.MemInc = DMA_MINC_ENABLE;
    psld.uart5dmatx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    psld.uart5dmatx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    psld.uart5dmatx_handle.Init.Mode = DMA_NORMAL;//DMA_CIRCULAR;
    psld.uart5dmatx_handle.Init.Priority = DMA_PRIORITY_LOW;
    psld.uart5dmatx_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    psld.uart5dmatx_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    psld.uart5dmatx_handle.Init.MemBurst = DMA_MBURST_SINGLE;
    psld.uart5dmatx_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
    if(HAL_DMA_Init(&psld.uart5dmatx_handle) != HAL_OK)
    {
      error_handler("Uart2DmaTx");
    }
    __HAL_LINKDMA(huart, hdmatx, psld.uart5dmatx_handle);

    psld.uart5dmarx_handle.Instance = DMA1_Stream0;
    psld.uart5dmarx_handle.Init.Channel = DMA_CHANNEL_4;
    psld.uart5dmarx_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    psld.uart5dmarx_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    psld.uart5dmarx_handle.Init.MemInc = DMA_MINC_ENABLE;
    psld.uart5dmarx_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    psld.uart5dmarx_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    psld.uart5dmarx_handle.Init.Mode = DMA_CIRCULAR;
    psld.uart5dmarx_handle.Init.Priority = DMA_PRIORITY_LOW;
    psld.uart5dmarx_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    psld.uart5dmarx_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    psld.uart5dmarx_handle.Init.MemBurst = DMA_MBURST_SINGLE;
    psld.uart5dmarx_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
    if(HAL_DMA_Init(&psld.uart5dmarx_handle) != HAL_OK)
    {
      error_handler("Uart5DmaRx");
    }
    __HAL_LINKDMA(huart, hdmarx, psld.uart5dmarx_handle);

    HAL_UART_Receive_DMA(huart, psld.uart5_rx_buffer, UART5_RXBUFF_SIZE);
    //p_uart5rxtail = uart5_rx_buffer;

    /* Local handle  */
    psld.uart_handle[0].name = "uart1";
    psld.uart_handle[0].handle =  (UART_HandleTypeDef *)&psld.uart1_handle;
    psld.uart_handle[0].rx_buffer = psld.uart1_rx_buffer;
    psld.uart_handle[0].rx_tail = psld.uart1_rx_buffer;
    psld.uart_handle[0].rxbuff_size = UART1_RXBUFF_SIZE;
    psld.uart_handle[0].timeout = 0;
    psld.uart_handle[0].canonical = 0;

    psld.uart_handle[1].name = "uart2";
    psld.uart_handle[1].handle =  (UART_HandleTypeDef *)&psld.uart2_handle;
    psld.uart_handle[1].rx_buffer = psld.uart2_rx_buffer;
    psld.uart_handle[1].rx_tail = psld.uart2_rx_buffer;
    psld.uart_handle[1].rxbuff_size = UART2_RXBUFF_SIZE;
    psld.uart_handle[1].timeout = 0;
    psld.uart_handle[1].canonical = 0;

    psld.uart_handle[2].name = "uart5";
    psld.uart_handle[2].handle =  (UART_HandleTypeDef *)&psld.uart5_handle;
    psld.uart_handle[2].rx_buffer = psld.uart5_rx_buffer;
    psld.uart_handle[2].rx_tail = psld.uart5_rx_buffer;
    psld.uart_handle[2].rxbuff_size = UART5_RXBUFF_SIZE;
    psld.uart_handle[2].timeout = 0;
    psld.uart_handle[2].canonical = 0;

}




/*
 * NVIC(interrupt) configuration
 * @param : None
 * @return: None
 */

void nvic_configuration(){
    HAL_NVIC_SetPriority(TIM4_IRQn, 12, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);

    //HAL_NVIC_EnableIRQ(UART5_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 12, 1);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 12, 2);
    HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 12, 1);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 12, 2);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, 12, 1);
    HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 12, 2);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

    HAL_NVIC_SetPriority(EXTI4_IRQn, 15, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_SetPriority(SysTick_IRQn, 0x0, 0x0);
}



/*
 * Timer configuration
 * @param : None
 * @return: None
 */
void timer_configuration()
{
    // TIM4 Configuration
    // RCC ENABLE
    __TIM4_CLK_ENABLE();

    // Set TIMx instance
    psld.Tim4Handle.Instance = TIM4;

    /*  TIM4CLK = HCLK / 2 = SystemCoreClock /2
     *  Prescaler = (TIM3CLK / TIM3 counter clock) - 1
     *  Prescaler = ((SystemCoreClock /2) /10 KHz) - 1
     */
    uint32_t PrescalerValue = (uint16_t)(SystemCoreClock / 2 / 10000) -1;

    psld.Tim4Handle.Init.Period = 10 - 1; // 1msec
    psld.Tim4Handle.Init.Prescaler = PrescalerValue;
    psld.Tim4Handle.Init.ClockDivision = 0;
    psld.Tim4Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_IC_Init(&psld.Tim4Handle);

    HAL_TIM_Base_Init(&psld.Tim4Handle);
    HAL_TIM_Base_Start_IT(&psld.Tim4Handle);

}


/*
 * UART interrupt handler
 * @param : None
 * @return: None
 */
void UART1_IRQHandler(){
    HAL_UART_IRQHandler(&psld.uart1_handle);
}

void UART2_IRQHandler(){
    HAL_UART_IRQHandler(&psld.uart2_handle);
}

void UART5_IRQHandler(){
    HAL_UART_IRQHandler(&psld.uart5_handle);
}

void TIM4_IRQHandler(){
    HAL_TIM_IRQHandler(&psld.Tim4Handle);
    HAL_IncTick();
}

void DMA2_Stream7_IRQHandler(){
    HAL_DMA_IRQHandler(&psld.uart1dmatx_handle);
}

void DMA2_Stream5_IRQHandler(){
    HAL_DMA_IRQHandler(&psld.uart1dmarx_handle);
}

void DMA1_Stream6_IRQHandler(){
    HAL_DMA_IRQHandler(&psld.uart2dmatx_handle);
}

void DMA1_Stream5_IRQHandler(){
    HAL_DMA_IRQHandler(&psld.uart2dmarx_handle);
}

void DMA1_Stream7_IRQHandler(){
    HAL_DMA_IRQHandler(&psld.uart5dmatx_handle);
}

void DMA1_Stream1_IRQHandler(){
    HAL_DMA_IRQHandler(&psld.uart5dmarx_handle);
}

void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == UART2_RI_PIN){
        psld.modem_check_ringcall_interrupt_flag = 1;
    }

}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){

    if(htim==&psld.Tim4Handle){
        if(psld.watchdogtimer_enable==0){
            HAL_IncTick();
        }else if(psld.watchdogtimer_enable==1){
            watchdogtimer_counter();
        }
    }
}


void BlinkyTask(void)
{
  while(1)
  {
      led_control(0, 1);
      HAL_Delay(50);//vTaskDelay(50);
      led_control(0, 0);
      HAL_Delay(50);//vTaskDelay(50);
  }
}


void error_handler(char *string)
{
    /* User may add here some code to deal with this error */
    //printf("Config Error:%s.\n", str);
    //while (1){}
    BlinkyTask();
}


/*
 *
 *   PHS Shield board hardware controller
 *
 */

/*
 * LED controller
 * @param : ch [number of channel]
 * @param : on [1:ON, 0:OFF]
 * @return: none
 */
void led_control(uint8_t ch, uint8_t on)
{
    switch(ch){
    case 0:
        if(on){
            HAL_GPIO_WritePin(LED_GPIO_PORT, LED1_PIN, 0);
        }else {
            HAL_GPIO_WritePin(LED_GPIO_PORT, LED1_PIN, 1);
        }
        break;
    case 1:
        if(on){
            HAL_GPIO_WritePin(LED_GPIO_PORT, LED2_PIN, 0);
        }else {
            HAL_GPIO_WritePin(LED_GPIO_PORT, LED2_PIN, 1);
        }
        break;
    default:
        break;
    }
}


/*
 * Reset modem
 * @param : on [1:enable reset, 0:disable reset]
 * @return: none
 */
void modem_reset(uint8_t on)
{
    if(on){
        HAL_GPIO_WritePin(MODEM_RESET_PORT, MODEM_RESET_PIN, 0);
    }else {
        HAL_GPIO_WritePin(MODEM_RESET_PORT, MODEM_RESET_PIN, 1);
    }
}




void modem_power_off_enable(uint8_t on)
{
    if(on){
        psld.modem_power_enable_flag = 1;
    }else {
        psld.modem_power_enable_flag = 0;
    }
}

/*
 * Power on/off modem
 * @param : on [1:enable power, 0:disable power]
 * @return: none
 */
void modem_power(uint8_t on)
{
    if(on){
        psld.modem_power_enable_flag = 0;
        psld.modem_powerup_flag = 1;
        HAL_GPIO_WritePin(MODEM_POWER_PORT, MODEM_POWER_PIN , 0);
        if(psld.modem_status == MODEM_POWER_OFF){
            psld.modem_status = MODEM_POWER_ON;
        }
    }else {
        psld.modem_powerup_flag = 0;
        HAL_GPIO_WritePin(MODEM_POWER_PORT, MODEM_POWER_PIN , 1);
        psld.modem_status = MODEM_POWER_OFF;
    }
}

/*
 * Set DSR line
 * @param : on [1:set high level, 0:set low level]
 * @return: none
 */
void modem_dsr(uint8_t on)
{
    if(on){
        HAL_GPIO_WritePin(UART2_DSR_PORT, UART2_DSR_PIN , 1);
    }else {
        HAL_GPIO_WritePin(UART2_DSR_PORT, UART2_DSR_PIN , 0);
    }
}

/*
 * Get modem line status
 * @paran : DCD [1:high level, 0:low level]
 *          DTR [1:high level, 0:low level]
 *          RI  [1:high level, 0:low level]
 * @return: none
 */
void modem_line_status(uint8_t *dcd, uint8_t *dtr, uint8_t *ri)
{
    *dcd = HAL_GPIO_ReadPin(UART2_DCD_PORT, UART2_DCD_PIN);
    *dtr = HAL_GPIO_ReadPin(UART2_DTR_PORT, UART2_DTR_PIN);
    *ri  = HAL_GPIO_ReadPin(UART2_RI_PORT, UART2_RI_PIN);
}


/*
 * Check modem is cold start or not
 * @param : none
 * @return: status[1:cold start, 0:normal]
 */
int32_t modem_powerup()
{
    if(psld.modem_powerup_flag){
        psld.modem_powerup_flag = 0;
        return 1;
    }else{
        return 0;
    }
}



/*
 *   Printf() tools
 */
void PrintChar(int8_t data)
{
    volatile int8_t c = data;
#if 1
    extern xQueueHandle queue_uart;
    if(c=='\n'){
        xQueueSendToBack(queue_uart, &c, 0);
        c = '\r';
        xQueueSendToBack(queue_uart, &c, 0);
    }else{
        xQueueSendToBack(queue_uart, &c, 0);
    }
#else
    if(c=='\n'){
        uart1_send((uint8_t *)&c, 1);
        c = '\r';
        uart1_send((uint8_t *)&c, 1);
    }else{
        uart1_send((uint8_t *)&c, 1);
    }
#endif
}

void PrintString(const char *s)
{
    int8_t c;

    while(1){
        c = *s++;
        if(c==0){
            break;
        }else{
            PrintChar(c);
        }
    }
}

void puts_direct(char *s)
{
    int8_t c;

    while(1){
        c = *s++;
        if(c==0){
            break;
        }else{
            uart1_putchar(c);
        }
    }
}


void printf_queued_push()
{
    int8_t c;
    uint32_t count_char;
    extern xQueueHandle queue_uart;

    count_char = 0;
    while(xQueueReceive(queue_uart, &c, 0) == pdPASS){
        psld.printf_queued_buffer[count_char++] = c;
        if(count_char>=NUMBER_QUEUED_BUFFER){
            break;
        }
    }
    if(count_char){
        uart1_send(psld.printf_queued_buffer, count_char);
    }
}

void printf_queued_flush()
{
    printf_queued_push();
    vTaskDelay(3000);
}

/*
 *
 *   UART tools
 *
 */
int32_t uart_read(struct_uart *uart, uint8_t *buf, size_t size)
{
    uint32_t num, count, current_time;
    int32_t diff_time;
    int32_t data;

    if(uart==0){
        return -1;
    }

    num = uart_available(uart);
    if(num==0 && uart->timeout){
        current_time = HAL_GetTick();
        diff_time = 0;
        while(diff_time < uart->timeout){
            diff_time = HAL_GetTick();
            if(diff_time>=current_time){
                diff_time = HAL_GetTick() - current_time;
            }else{
                diff_time = HAL_GetTick() + (0x7fffffff - current_time);
            }
            vTaskDelay(10);
            num = uart_available(uart);
            if(num){
                break;
            }
        }
        if(num==0){
            return -2;
        }
    }

    count = 0;
    while(num-->0){
        data = uart_getchar(uart);
        *buf++ = data;
        if(++count==size){
            break;
        }
    }

    return count;
}

int32_t uart_available(struct_uart *uart)
{
    ptrdiff_t diff;
    uint8_t const *head, *tail;
    uint8_t data;
    int32_t count;

    head = uart->rx_buffer + uart->rxbuff_size - __HAL_DMA_GET_COUNTER(uart->handle->hdmarx);
    tail = uart->rx_tail;

    if(uart==0){
        return -1;
    }
    if(strcmp(uart->name, "uart5")==0){
        HAL_Delay(2);
    }
    if(head >= tail){
        diff = head - tail;
    }else{
        diff = head - tail + uart->rxbuff_size;
    }
    count = diff;

    if(uart->canonical && diff){
        count = 0;
        while(head != tail){
            data = *tail;
            count++;
            if(data=='\n'){
                break;
            }
            tail++;
            if(tail >= uart->rx_buffer + uart->rxbuff_size){
                tail -= uart->rxbuff_size;
            }
        }
        if(head == tail){
            return 0;
        }
    }
    return (int32_t)count;
}

int32_t uart_getchar(struct_uart *uart)
{
    uint8_t const *head, *tail;
    head = uart->rx_buffer + uart->rxbuff_size - __HAL_DMA_GET_COUNTER(uart->handle->hdmarx);
    tail = uart->rx_tail;

    if(uart==0){
        return -1;
    }
    if(head != tail){
        uint8_t data = *uart->rx_tail++;
        if(uart->rx_tail >= uart->rx_buffer + uart->rxbuff_size){
            uart->rx_tail -= uart->rxbuff_size;
        }
        return data;
    }
    return 0;
}

int32_t uart_send(struct_uart *uart, uint8_t* buffer, uint16_t size)
{
    HAL_StatusTypeDef status;
    uint32_t timeout_count;

    if(uart==0){
        return -1;
    }
    status = HAL_UART_Transmit_DMA(uart->handle, buffer, size);
    if(status == HAL_ERROR){
        return -2;
    }else if(status == HAL_BUSY){
        timeout_count = 0;
        while(HAL_UART_Transmit_DMA(uart->handle, buffer, size) != HAL_OK){
            vTaskDelay(10);
            if(timeout_count++>UART_TXDMA_TIMEOUT*100){
                HAL_DMA_DeInit(uart->handle->hdmatx);
                return -3;
            }
        }
    }
    return size;
}

int32_t uart_flush(struct_uart *uart)
{
    uint32_t state, receive_byte;

    if(uart==0){
        return -1;
    }
    state = uart->canonical;
    uart->canonical = 0;

    receive_byte = uart_available(uart);
    while(receive_byte-->0){
        uart_getchar(uart);
    }
    uart->canonical = state;
    return 0;
}

int32_t uart_set_baudrate(struct_uart *uart, uint32_t baudrate)
{
    if(uart==0){
        return -1;
    }
    uart->handle->Init.BaudRate = baudrate;

    if(HAL_UART_Init(uart->handle) != HAL_OK){
        return -2;
    }
    return 0;
}

int32_t uart_set_flow(struct_uart *uart, int enable)
{
    if(uart==0){
        return -1;
    }
    if(enable){
        uart->handle->Init.HwFlowCtl  = UART_HWCONTROL_RTS_CTS;
    }else{
        uart->handle->Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    }

    if(HAL_UART_Init(uart->handle) != HAL_OK){
        return -2;
    }
    return 0;
}

int32_t uart_set_timeout(struct_uart *uart, uint32_t timeout_msec)
{
    if(uart==0){
        return -1;
    }
    uart->timeout = timeout_msec;
    return 0;
}

int32_t uart_mode_canonical(struct_uart *uart)
{
    if(uart==0){
        return -1;
    }
    uart->canonical = 1;
    return 0;
}

int32_t uart_mode_noncanonical(struct_uart *uart)
{
    if(uart==0){
        return -1;
    }
    uart->canonical = 0;
    return 0;
}


int32_t uart1_send(uint8_t* buffer, uint16_t size)
{
    HAL_StatusTypeDef status;
    uint32_t timeout_count;
    struct_uart *uart;
    uart = (struct_uart *)&psld.uart_handle[0];

    if(uart==0){
        return -1;
    }
    status = HAL_UART_Transmit_DMA(uart->handle, buffer, size);
    if(status == HAL_ERROR){
        return -2;
    }else if(status == HAL_BUSY){
        timeout_count = 0;
        while(HAL_UART_Transmit_DMA(uart->handle, buffer, size) != HAL_OK){
            vTaskDelay(10);
            if(timeout_count++>UART_TXDMA_TIMEOUT*100){
                HAL_DMA_DeInit(uart->handle->hdmatx);
                return -3;
            }
        }
    }
    return (int32_t)size;
}


void uart1_putchar(int8_t c)
{
    uart1_send((uint8_t *)&c, 1);
}


int32_t uart1_getchar()
{
    struct_uart *uart;
    uint8_t const *head, *tail;

    uart = (struct_uart *)&psld.uart_handle[0];
    head = uart->rx_buffer + uart->rxbuff_size - __HAL_DMA_GET_COUNTER(uart->handle->hdmarx);
    tail = uart->rx_tail;

    if(uart==0){
        return -1;
    }
    if(head != tail){
        uint8_t data = *uart->rx_tail++;
        if(uart->rx_tail >= uart->rx_buffer + uart->rxbuff_size){
            uart->rx_tail -= uart->rxbuff_size;
        }
        return data;
    }
    return 0;
}


int32_t uart1_available()
{
    struct_uart *uart;
    ptrdiff_t diff;
    uint8_t const *head, *tail;
    uint8_t data;
    int32_t count;

    uart = (struct_uart *)&psld.uart_handle[0];

    head = uart->rx_buffer + uart->rxbuff_size - __HAL_DMA_GET_COUNTER(uart->handle->hdmarx);
    tail = uart->rx_tail;

    if(uart==0){
        return -1;
    }
    if(strcmp(uart->name, "uart5")==0){
        HAL_Delay(2);
    }
    if(head >= tail){
        diff = head - tail;
    }else{
        diff = head - tail + uart->rxbuff_size;
    }
    count = diff;

    if(uart->canonical && diff){
        count = 0;
        while(head != tail){
            data = *tail;
            count++;
            if(data=='\n'){
                break;
            }
            tail++;
            if(tail >= uart->rx_buffer + uart->rxbuff_size){
                tail -= uart->rxbuff_size;
            }
        }
        if(head == tail){
            return 0;
        }
    }
    return (int32_t)count;
}

int32_t uart1_set_baudrate(uint16_t baudrate)
{
    int32_t status;
    struct_uart *uart;
    uart = (struct_uart *)&psld.uart_handle[0];

    xSemaphoreTake(semaphore_sio, portMAX_DELAY);
    status = uart_set_baudrate(uart, baudrate);
    xSemaphoreGive(semaphore_sio);

    return status;
}

int32_t uart2_send(uint8_t* buffer, uint16_t size)
{
    struct_uart *uart;
    uart = (struct_uart *)&psld.uart_handle[1];
    return uart_send(uart, buffer, size);
}


int32_t uart2_getchar()
{
    struct_uart *uart;
    uart = (struct_uart *)&psld.uart_handle[1];
    return uart_getchar(uart);
}


int32_t uart2_available()
{
    struct_uart *uart;
    uart = (struct_uart *)&psld.uart_handle[1];
    return uart_available(uart);
}


int32_t uart2_set_baudrate(uint16_t baudrate)
{
    int32_t status;
    struct_uart *uart;
    uart = (struct_uart *)&psld.uart_handle[1];

    xSemaphoreTake(semaphore_sio, portMAX_DELAY);
    status = uart_set_baudrate(uart, baudrate);
    xSemaphoreGive(semaphore_sio);

    return status;
}

int32_t uart5_getchar()
{
    struct_uart *uart;
    uint8_t const *head, *tail;

    uart = (struct_uart *)&psld.uart_handle[2];
    head = uart->rx_buffer + uart->rxbuff_size - __HAL_DMA_GET_COUNTER(uart->handle->hdmarx);
    tail = uart->rx_tail;

    if(uart==0){
        return -1;
    }
    if(head != tail){
        uint8_t data = *uart->rx_tail++;
        if(uart->rx_tail >= uart->rx_buffer + uart->rxbuff_size){
            uart->rx_tail -= uart->rxbuff_size;
        }
        return data;
    }
    return 0;
}


int32_t uart5_available()
{
    struct_uart *uart;
    ptrdiff_t diff;
    uint8_t const *head, *tail;
    uint8_t data;
    int32_t count;

    uart = (struct_uart *)&psld.uart_handle[2];
    head = uart->rx_buffer + uart->rxbuff_size - __HAL_DMA_GET_COUNTER(uart->handle->hdmarx);
    tail = uart->rx_tail;

    if(uart==0){
        return -1;
    }
    if(strcmp(uart->name, "uart5")==0){
        HAL_Delay(2);
    }
    if(head >= tail){
        diff = head - tail;
    }else{
        diff = head - tail + uart->rxbuff_size;
    }
    count = diff;

    if(uart->canonical && diff){
        count = 0;
        while(head != tail){
            data = *tail;
            count++;
            if(data=='\n'){
                break;
            }
            tail++;
            if(tail >= uart->rx_buffer + uart->rxbuff_size){
                tail -= uart->rxbuff_size;
            }
        }
        if(head == tail){
            return 0;
        }
    }
    return (int32_t)count;}


int32_t uart5_send(uint8_t* buffer, uint16_t size)
{
    HAL_StatusTypeDef status;
    uint32_t timeout_count;
    struct_uart *uart;
    uart = (struct_uart *)&psld.uart_handle[2];

    if(uart==0){
        return -1;
    }
    status = HAL_UART_Transmit_DMA(uart->handle, buffer, size);
    if(status == HAL_ERROR){
        return -2;
    }else if(status == HAL_BUSY){
        timeout_count = 0;
        while(HAL_UART_Transmit_DMA(uart->handle, buffer, size) != HAL_OK){
            vTaskDelay(10);
            if(timeout_count++>UART_TXDMA_TIMEOUT*100){
                HAL_DMA_DeInit(uart->handle->hdmatx);
                return -3;
            }
        }
    }
    return (int32_t)size;
}


int32_t uart5_set_baudrate(uint16_t baudrate)
{
    struct_uart *uart;
    int32_t status;
    uart = (struct_uart *)&psld.uart_handle[2];

    xSemaphoreTake(semaphore_sio, portMAX_DELAY);
    status = uart_set_baudrate(uart, baudrate);
    xSemaphoreGive(semaphore_sio);

    return status;
}


