#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "stm32f4xx_hal.h"
#include "phs_shield.h"
#include "phs_shield_uart.h"


extern struct_psld psld;


/*
 * @struct serial_private
 * @brief private data for this class
 */
typedef struct {
    int fd;          /*!< TTY file descriptor */
    int is_canonical;
    int timeout;
    int minimum_read;
    int error_num;
} serial_private;



/*
 * @brief Instantiate serial object
 * @retval pointer to object instance on success
 * @retval NULL on allocation failure
 */
serial *
serial_create(void)
{
    size_t alloc_size;
    serial *obj;
    serial_private *prv;

    alloc_size = sizeof(serial) + sizeof(serial_private);
    prv = (serial_private *) malloc(alloc_size);

    if (prv == NULL) {
        return NULL;
    }

    prv->fd = (-1);
    prv->error_num = 0;

    obj = (serial *) (prv + 1);
    obj->prv = (void *) prv;

    return(obj);
}

/*!
 * @brief close serial port
 * @retval 0 on success
 * @retval -1 on failure
 * @param[in] obj pointer to object instance
 */
int
serial_close(serial *obj)
{
    serial_private *prv;

    prv = (serial_private *) obj->prv;

    if (prv->fd < 0) {
        return(-1);
    }

    close(prv->fd);
    prv->fd = (-1);

    return(0);
}

/*
 * @brief Destroy serial object instance
 * @param[in] obj pointer to object instance
 * @retval None
 */
void
serial_destroy(serial *obj)
{
    serial_private *prv;

    prv = (serial_private *) obj->prv;

    serial_close(obj);
    free(prv);

    return;
}


/*!
 * @brief open serial port
 * @retval 0 on success
 * @retval -1 on failure
 * @param[in] obj pointer to object instance
 * @param[in] device device name to open
 * @note this method may never be called when the port is already open
 */

int
serial_open(serial *obj, const char *device)
{
    serial_private *prv;
    int fd, i, size;

    prv = (serial_private *) obj->prv;

    if (prv->fd >= 0) {
        return(-1);
    }

    size = psld.device_list_size;
    for(i=0;i<size;i++){
        if(strcmp((char *)device, psld.device_list[i].posix)==0){
            break;
        }
    }
    if(i==size){
        return -2;
    }

    fd = (int)psld.device_list[i].uart_handle;

    prv->fd = fd;
    prv->is_canonical = 0;
    prv->timeout = 0;
    prv->minimum_read = 0;

    return(0);
}


/*!
 * @brief send data to serial port
 * @retval 0 on success
 * @retval -1 on failure
 * @param[in] obj pointer to object instance
 * @param[in] buf pointer to buffer holding data to send
 * @param[in] size to send
 * @note serial port must be opened
 * @note will block until specified amount of data is sent or timeout
 */
size_t
serial_write(serial *obj, uint8_t* buffer, size_t size)
{
    struct_uart *uart;
    serial_private *prv;
    prv = (serial_private *)obj->prv;
    uart = (struct_uart *)(prv->fd);

    if (prv->fd < 0) {
        return(-1);
    }

    return uart_send(uart, buffer, size);
}

/*!
 * @brief receive data from serial port
 * @retval 0 on success
 * @retval -1 on failure
 * @param[in] obj pointer to object instance
 * @param[out] buf pointer to buffer to store received data
 * @param[in] size to receive
 * @note will block until specified amount of data is received or timeout
 */
size_t
serial_read(serial *obj, uint8_t *buffer, size_t size)
{
    struct_uart *uart;
    serial_private *prv;
    prv = (serial_private *)obj->prv;
    uart = (struct_uart *)(prv->fd);

    if (prv->fd < 0) {
        return(-1);
    }

    return uart_read(uart, buffer, size);
}

/*!
 * @brief set speed for an open device
 * @retval 0 on success
 * @retval -1 on failure
 * @param[in] obj pointer to object instance
 * @param[in] speed
 * @note serial port must be opened
 */
int
serial_set_speed(serial *obj, uint32_t baudrate)
{
    struct_uart *uart;
    serial_private *prv;

    prv = (serial_private *)obj->prv;
    uart = (struct_uart *)(prv->fd);

    if (prv->fd < 0) {
        return(-1);
    }

    uart->handle->Init.BaudRate = baudrate;

    if(HAL_UART_Init(uart->handle) != HAL_OK){
        return 1;
    }
    return 0;
}


/*!
 * @brief enable or disable CTS hardware flow control on open device
 * @retval 0 on success
 * @retval -1 on failure
 * @param[in] obj pointer to object instance
 * @param[in] enabled non-zero value enable flow control
 * @note serial port must be opened
 */
int
serial_set_cts_flow(serial *obj, int enable)
{
    struct_uart *uart;
    serial_private *prv;

    prv = (serial_private *)obj->prv;
    uart = (struct_uart *)(prv->fd);

    if (prv->fd < 0) {
        return(-1);
    }
    if(HAL_UART_Init(uart->handle) != HAL_OK){
        return 1;
    }
    return 0;
}


/*!
 * @brief set timeout on open device
 * @retval 0 on success
 * @retval -1 on failure
 * @param[in] obj pointer to object instance
 * @param[in] timeout timeout in mili-seconds
 * @note serial port must be opened
 */
int
serial_set_timeout(serial *obj, uint32_t timeout_msec)
{
    struct_uart *uart;
    serial_private *prv;

    prv = (serial_private *)obj->prv;
    uart = (struct_uart *)(prv->fd);

    if (prv->fd < 0) {
        return(-1);
    }
    prv->timeout = timeout_msec;
    uart->timeout = timeout_msec;
    return 0;
}


/*!
 * @brief set serial device to canonical mode
 * @retval 0 on success
 * @retval -1 on failure
 * @param[in] obj pointer to object instance
 * @param[in] speed
 * @note serial port must be opened
 */
int
serial_mode_canonical(serial *obj)
{
    struct_uart *uart;
    serial_private *prv;

    prv = (serial_private *)obj->prv;
    uart = (struct_uart *)(prv->fd);

    if (prv->fd < 0) {
        return(-1);
    }
    prv->is_canonical = 1;
    uart->canonical = 1;
    return 0;
}


/*!
 * @brief set device to non-canonical mode
 * @retval 0 on success
 * @retval -1 on failure
 * @param[in] obj pointer to object instance
 * @param[in] speed
 * @note serial port must be opened
 */
int
serial_mode_noncanonical(serial *obj)
{
    struct_uart *uart;
    serial_private *prv;

    prv = (serial_private *)obj->prv;
    uart = (struct_uart *)(prv->fd);

    if (prv->fd < 0) {
        return(-1);
    }
    prv->is_canonical = 0;
    uart->canonical = 0;
    return 0;
}

/*!
 * @brief flush pending data
 * @retval 0 on success
 * @retval -1 on failure
 * @param[in] obj pointer to object instance
 * @note will block until specified amount of data is received or timeout
 */
int
serial_flush(serial *obj)
{
    struct_uart *uart;
    serial_private *prv;
    prv = (serial_private *)obj->prv;
    uart = (struct_uart *)(prv->fd);

    if (prv->fd < 0) {
        return(-1);
    }

    return uart_flush(uart);
}


int
serial_posix_setup(const char *device, int baud, serial **serialobj)
{

    /* open serial */
    *serialobj = serial_create();
    if (*serialobj == NULL) {
        printf("Unable to instantiate object: serial\n");
        goto fail;
    }

    if (serial_open(*serialobj, device) < 0) {
        printf("Unable to open serial devicer\n");
        goto fail;
    }

    if (serial_set_speed(*serialobj, baud) < 0) {
        printf("Unable to set serial speedr\n");
        goto fail;
    }

    if (serial_set_timeout(*serialobj, 10000) < 0) {
        printf("Unable to set timeoutr\n");
        goto fail;
    }

    if (serial_set_cts_flow(*serialobj, 1) < 0) {
        printf("Unable to set flow controlr\n");
        goto fail;
    }
    printf("%s is opened\n", device);
    return (0);

fail:
    serial_close(*serialobj);
    serial_destroy(*serialobj);
    return (1);
}

