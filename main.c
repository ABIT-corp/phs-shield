#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "phs_shield.h"
#include "serial_posix_api.h"
#include "watchdog_timer.h"
#include "phs_shield_uart.h"
#include "monitor.h"
#include "rtc.h"
#include "modem.h"
#include "power_saving.h"


/* Semaphore  */
xSemaphoreHandle semaphore_sio;
xSemaphoreHandle semaphore_puts;
xSemaphoreHandle semaphore_tcp;

/* Handle of RTOS task  */
xTaskHandle handle_thread_control;
xTaskHandle handle_thread_arduino;
xTaskHandle handle_thread_lwip;

/* RTOS task thread */
static void thread_control(void *pvParameters);
static void thread_lwip(void *pvParameters);
static void thread_arduino(void *pvParameters);

/* Queue */
xQueueHandle queue_uart;
xQueueHandle queue_command;



extern struct_psld psld;


int main(void)
{
    /* STM32F4xx HAL library initialization:
    * Configure the Flash prefetch, instruction and Data caches
    * Configure the Systick to generate an interrupt each 1 msec
    * Set NVIC Group Priority to 4
    * Low Level Initialization
    * Configure the System clock to have a frequency of 168 MHz
    */
    HAL_initialization();

    /* User configuration */
    global_configuration();
    gpio_configuration();
    uart_configuration();
    led_control(0, 0);
    nvic_configuration();
    timer_configuration();
    rtc_configuration();
    watchdogtimer_configuration();
    monitor_configuration();arduino_watchdog_clear();

    /* PHS APM-002 Initialization */
    modem_reset(1);
    modem_power(1);
    modem_dsr(0);
    arduino_interruput(0);

    /* Mutex */
    semaphore_sio = xSemaphoreCreateMutex();
    semaphore_puts = xSemaphoreCreateMutex();
    semaphore_tcp = xSemaphoreCreateMutex();
    queue_uart = xQueueCreate(4096, sizeof(int8_t));
    queue_command = xQueueCreate(8, sizeof(int32_t));

    /* Create RTOS task */
    xTaskCreate(thread_control, (portCHAR *)"monitor", 128*2, NULL, 1, &handle_thread_control);
    xTaskCreate(thread_arduino, (portCHAR *)"arduino", 128*5, NULL, 1, &handle_thread_arduino);
    xTaskCreate(thread_lwip,    (portCHAR *)"lwip", 128*5, NULL, 1, &handle_thread_lwip);
    vTaskSuspend(handle_thread_lwip);

    printf("\nstart freeRTOS\r\n");
    printf("firmware version: %s\r\n", ARDUINO_API_VERSION);

    /* FreeRTOS start */
    led_control(1, 0);
    //__TIM4_CLK_DISABLE();
    timer4_idle_mode();

    vTaskStartScheduler();

    while(1);
}


// RTOS : Task A
void thread_control(void *pvParameters)
{
    uint8_t current_staus1, current_staus2, num, c;
    portTickType xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    uint32_t count_time;

    current_staus1 = 1;
    current_staus2 = 1;
    count_time = 0;
    while(1){
        printf_queued_push();

        if(psld.modem_check_ringcall_interrupt_flag){
            modem_check_ringcall_interrupt();
            psld.modem_check_ringcall_interrupt_flag = 0;
        }
        if(psld.profile_number_changed){
            modem_mode_change();
            psld.profile_number_changed = 0;
        }
        if(count_time++%1000==0){
            profile_update();
        }

        num = uart1_available();
        if(num>0 && !psld.direct_channel_modem && !psld.direct_channel_arduino){
            c = uart1_getchar();
            if(c=='\r'){
                monitor();
            }
        }

#if 1
        if(arduino_request_regurator()){
            if(current_staus1==1){
                vTaskDelay(10);
                if(arduino_request_regurator()){
                    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
                    printf("power on request\n");
                    xSemaphoreGive(semaphore_puts);
                    current_staus1 = 0;
                }
            }
        }else{
            if(current_staus1==0){
                vTaskDelay(10);
                if(!arduino_request_regurator()){
                    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
                    printf("power off request\n");
                    xSemaphoreGive(semaphore_puts);
                    current_staus1 = 1;
                    stopmode_entry();
                }
            }
        }

        if(arduino_request_power()){
            if(current_staus2==0){
                xSemaphoreTake(semaphore_puts, portMAX_DELAY);
                printf("modem on request\n");
                xSemaphoreGive(semaphore_puts);
            }
            current_staus2 = 1;
        }else{
            if(current_staus2==1){
                xSemaphoreTake(semaphore_puts, portMAX_DELAY);
                printf("modem off request\n");
                xSemaphoreGive(semaphore_puts);
            }
            current_staus2 = 0;
        }
#endif
        vTaskDelayUntil(&xLastWakeTime, 3 / portTICK_RATE_MS );
    }
}


void thread_arduino(void *pvParameters)
{
    const char *device = "/dev/ttyACM0";
    int32_t baudrate = 115200;
    int32_t command_data;
    uint8_t num, c;
    void (*command[])() = {NULL};

    vTaskDelay(1000);
    baudrate = 115200;
    serial_posix_setup(device, baudrate, (serial *)&psld.serialobj);
    if(modem_setup(psld.serialobj)){
        printf("modem failed. stopped.\n");
        while(1);
    }
    vTaskDelay(1000);
    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
    printf("start to receive arduino command\n");
    xSemaphoreGive(semaphore_puts);
    vTaskResume(handle_thread_lwip);

#if defined(WATCHDOG_TIMER)
    watchdogtimer_start();
#endif
    arduino_flush();

    while(1){
        watchdogtimer_refresh();

        if(psld.direct_channel_modem){
            xSemaphoreTake(semaphore_puts, portMAX_DELAY);
            num = uart1_available();
            if(num>0){
                c = uart1_getchar();
                PrintChar(c);
                uart2_send((uint8_t *)&c, 1);
            }
            num = uart2_available();
            if(num>0){
                c = uart2_getchar();
                PrintChar(c);
            }
            xSemaphoreGive(semaphore_puts);
        }

        if(psld.direct_channel_arduino){
            xSemaphoreTake(semaphore_puts, portMAX_DELAY);
            num = uart1_available();
            if(num>0){
                c = uart1_getchar();
                PrintChar(c);
                uart5_send((uint8_t *)&c, 1);
            }
            num = uart5_available();
            if(num>0){
                c = uart5_getchar();
                PrintChar(c);
            }
            xSemaphoreGive(semaphore_puts);
        }else{
            modem_ppp_linkup_surveillance();

            arduino_api();

            if(xQueueReceive(queue_command, &command_data, 0) == pdPASS){
                command[0] = (void *)command_data;
                command[0]();
            }
        }
        vTaskDelay(10);
        if(num>32){
            num = 0;
#if 0
            arduino_watchdog_timer();
#endif
        }else if(num++>16){
            led_control(1, 0);
        }else{
            led_control(1, 1);
        }
    }
}

void thread_lwip(void *pvParameters)
{
    lwip_main();
    while(1);
}

