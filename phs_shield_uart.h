#ifndef PHS_SHIELD_UART_H
#define PHS_SHIELD_UART_H



int32_t uart_read(struct_uart *uart, uint8_t *buf, size_t size);
int32_t uart_getchar(struct_uart *uart);
int32_t uart_available(struct_uart *uart);
int32_t uart_send(struct_uart *uart, uint8_t* buffer, uint16_t size);
int32_t uart_set_baudrate(struct_uart *uart, uint32_t baudrate);
int32_t uart_set_flow(struct_uart *uart, int enable);
int32_t uart_set_timeout(struct_uart *uart, uint32_t timeout_msec);
int32_t uart_mode_canonical(struct_uart *uart);
int32_t uart_mode_noncanonical(struct_uart *uart);
int32_t uart_flush(struct_uart *uart);
int32_t uart1_send(uint8_t* buffer, uint16_t size);
int32_t uart1_getchar();
void    uart1_putchar(int8_t);
int32_t uart1_available();
int32_t uart1_set_baudrate(uint16_t baudrate);
int32_t uart2_send(uint8_t* buffer, uint16_t size);
int32_t uart2_getchar();
int32_t uart2_available();
int32_t uart2_set_baudrate(uint16_t baudrate);
int32_t uart5_getchar();
int32_t uart5_available();
int32_t uart5_send(uint8_t* buffer, uint16_t size);
int32_t uart5_set_baudrate(uint16_t baudrate);
void puts_direct(char* );


#endif
