#ifndef PHS_SHIELD_H
#define PHS_SHIELD_H

#include "stm32f4xx_hal.h"

#define ARDUINO_API_VERSION "2.34"

/* Watchdog-Timer */
#define WATCHDOG_TIMER

/* Software reset event */
#define SYSRESET_STOP_MODE     110
#define SYSRESET_COMMAND       111
#define SYSRESET_WAKE_UP       112
#define SYSRESET_WATCHDOG      113
#define SYSRESET_PPPLINKRETRY  114
#define SYSRESET_CONNECTRETRY  115
#define SYSRESET_BUFALLOCRETRY 116
#define SYSRESET_TCPIPBUSY     117
#define SYSRESET_PPPREBOOT     118


/* LED */
#define LED1_PIN                         GPIO_PIN_7
#define LED2_PIN                         GPIO_PIN_6
#define LED_GPIO_PORT                    GPIOC

/* MODEM Power */
#define MODEM_POWER_PIN                  GPIO_PIN_9
#define MODEM_POWER_PORT                 GPIOC

/* MODEM */
#define MODEM_RESET_PIN                  GPIO_PIN_11
#define MODEM_RESET_PORT                 GPIOB

/* Arduino */
#define ARDUINO_REGON_PIN                GPIO_PIN_4
#define ARDUINO_REGON_PORT               GPIOC
#define ARDUINO_POWERON_PIN              GPIO_PIN_5
#define ARDUINO_POWERON_PORT             GPIOC
#define ARDUINO_INTERRUPT_PIN            GPIO_PIN_1
#define ARDUINO_INTERRUPT_PORT           GPIOB

/* USART1 */
#define USART1_AF                        GPIO_AF7_USART1
#define USART1_TX_PIN                    GPIO_PIN_6
#define USART1_TX_GPIO_PORT              GPIOB
#define USART1_RX_PIN                    GPIO_PIN_7
#define USART1_RX_GPIO_PORT              GPIOB

/* UART2 for APM-002 */
#define USART2_AF                        GPIO_AF7_USART2
#define USART2_TX_PIN                    GPIO_PIN_2
#define USART2_TX_GPIO_PORT              GPIOA
#define USART2_RX_PIN                    GPIO_PIN_3
#define USART2_RX_GPIO_PORT              GPIOA
#define UART2_CTS_PIN                    GPIO_PIN_0
#define UART2_CTS_PORT                   GPIOA
#define UART2_RTS_PIN                    GPIO_PIN_1
#define UART2_RTS_PORT                   GPIOA

#define UART2_DCD_PIN                    GPIO_PIN_7
#define UART2_DCD_PORT                   GPIOA
#define UART2_DCD_MODE                   GPIO_MODE_INPUT
#define UART2_DTR_PIN                    GPIO_PIN_6
#define UART2_DTR_PORT                   GPIOA
#define UART2_DTR_MODE                   GPIO_MODE_INPUT
#define UART2_DSR_PIN                    GPIO_PIN_5
#define UART2_DSR_PORT                   GPIOA
#define UART2_DSR_MODE                   GPIO_MODE_OUTPUT_PP
#define UART2_RI_PIN                     GPIO_PIN_4
#define UART2_RI_PORT                    GPIOA
#define UART2_RI_MODE                    GPIO_MODE_INPUT


/* UART5 for Arduino */
#define USART5_AF                        GPIO_AF8_UART5
#define USART5_TX_PIN                    GPIO_PIN_12
#define USART5_TX_GPIO_PORT              GPIOC
#define USART5_RX_PIN                    GPIO_PIN_2
#define USART5_RX_GPIO_PORT              GPIOD


/*
 * Global variable
 */
#define MODEM_WORKING_SIZE               1024
#define ARDUINO_WORKING_SIZE             1048

/* Uart buffer size */
#define UART1_TXBUFF_SIZE                128
#define UART1_RXBUFF_SIZE                128
#define UART2_TXBUFF_SIZE                1024
#define UART2_RXBUFF_SIZE                1024
#define UART5_TXBUFF_SIZE                1024
#define UART5_RXBUFF_SIZE                1024

/* Printf queued buffer size */
#define NUMBER_QUEUED_BUFFER             1024

/* Timeout */
#define UART_TXDMA_TIMEOUT               30
#define PPPLINKUPRETRY_TIMEOUT           1
#define TCPIP_CONNECT_PRETRY_TIMEOUT     5
#define TCPIP_BUSY_TIMEOUT               5
#define BUF_ALLOC_RETRY_TIMEOUT          5

/* Profile */
#define PROFILE_PPP_LINKUP                0x01
#define PROFILE_PPP_LINKDOWN              0x02


typedef struct
{
    int start;
    int end;
} struct_memory_map;

typedef struct {
    void *prv;
} serial;

typedef struct {
    void *prv;
} modem;

typedef enum {
    MODEM_POWER_OFF = 1,
    MODEM_POWER_ON,
    MODEM_ATZ,
    PPP_CONNECT,
    PPP_LINK,
    PPP_NO_LINK,
} modem_link_status;

typedef struct
{
    const char *name;
    UART_HandleTypeDef *handle;
    const uint8_t *rx_buffer;
    const uint8_t *rx_tail;
    uint16_t rxbuff_size;
    uint32_t timeout;
    uint32_t canonical;
} struct_uart;

typedef struct
{
    const char *posix;
    const char *hal;
    uint16_t  id;
    struct_uart *uart_handle;
} struct_device_list;


typedef struct {
    TIM_HandleTypeDef  Tim4Handle;

    UART_HandleTypeDef uart1_handle;
    UART_HandleTypeDef uart2_handle;
    UART_HandleTypeDef uart5_handle;

    DMA_HandleTypeDef uart1dmatx_handle;
    DMA_HandleTypeDef uart1dmarx_handle;
    DMA_HandleTypeDef uart2dmatx_handle;
    DMA_HandleTypeDef uart2dmarx_handle;
    DMA_HandleTypeDef uart5dmatx_handle;
    DMA_HandleTypeDef uart5dmarx_handle;

    uint8_t uart1_rx_buffer[UART1_RXBUFF_SIZE];
    uint8_t uart2_rx_buffer[UART2_RXBUFF_SIZE];
    uint8_t uart5_rx_buffer[UART5_RXBUFF_SIZE];
    uint8_t printf_queued_buffer[NUMBER_QUEUED_BUFFER];

    struct_uart uart_handle[3];
    struct_device_list *device_list;
    uint8_t  device_list_size;

    uint8_t direct_channel_modem;
    uint8_t direct_channel_arduino;

    serial *serialobj;
    modem *modemobj;

    volatile uint32_t profile_number_changed;
    volatile uint32_t profile_number;
    volatile uint8_t modem_powerup_flag;
    volatile uint8_t modem_power_enable_flag;
    volatile int32_t modem_check_ringcall_interrupt_flag;
    volatile modem_link_status modem_status;

    struct_memory_map *memory_map;
    uint8_t  memory_map_size;

    uint8_t modem_work_memory[MODEM_WORKING_SIZE];
    uint8_t arduino_work_memory[ARDUINO_WORKING_SIZE];

    int ppp_enable;
    int ppp_done;
    int ppp_restart;
    uint32_t ppp_linkup_retry;
    uint32_t tcpip_connect_retry;
    uint32_t tcpip_busy_retry;
    uint32_t buf_alloc_retry;

    uint32_t watchdogtimer_count;
    uint32_t watchdogtimer_enable;


} struct_psld;



void gpio_configuration();
void timer_configuration();
void nvic_configuration();
void uart_configuration(void);
void PrintChar(int8_t);
void led_control(uint8_t ch, uint8_t on);
void arduino_api();
void modem_power(uint8_t);
void modem_reset(uint8_t);
void modem_dsr(uint8_t);
void arduino_interruput(uint8_t);
void HAL_initialization();
void arduino_flush();
uint8_t arduino_request_power();
uint8_t arduino_request_regurator();
void error_handler(char *);
void vTaskDelay(uint32_t);
void lwip_main_restart();
void lwip_main_close(uint8_t);
void lwip_main(void);
void global_configuration();
void puts_direct(char *s);


#endif
