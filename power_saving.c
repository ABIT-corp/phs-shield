#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "phs_shield.h"
#include "rtc.h"
#include "watchdog_timer.h"


void stopmode_entry(void);


/*
 * GPIO configuration for EXTI interrupt
 * @param  :None
 * @return :None
 */
static void
stopmode_gpio_configuration()
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    GPIO_InitStruct.Pin = GPIO_PIN_All;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);


    GPIO_InitStruct.Pin       = ARDUINO_REGON_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_IT_RISING;
    HAL_GPIO_Init(ARDUINO_REGON_PORT, &GPIO_InitStruct);
}



void
stopmode_entry()
{
    extern xSemaphoreHandle semaphore_puts;

    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
    puts("modem power off\n\r");
    puts("cpu enter stop mode\n\r");
    xSemaphoreGive(semaphore_puts);

    modem_power(0);
    stopmode_gpio_configuration();

    vTaskSuspendAll();
    watchdogtimer_stop();
    rtc_backup_write(19, SYSRESET_STOP_MODE);

    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* STOP here until EXTI interrupt */
    NVIC_SystemReset();

    while(1){
        vTaskDelay(1000);
    }
}


