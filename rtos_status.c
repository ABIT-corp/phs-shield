#include <stdio.h>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

extern void puts_direct(char *);
extern xSemaphoreHandle semaphore_puts;

void status_task_list_view()
{
    char *work_buffer;

    work_buffer = (char *)malloc(40*20);
    vTaskList(work_buffer);
    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
    puts_direct("name         state  priority free_mem  num\r\n");
    puts_direct(work_buffer);
    xSemaphoreGive(semaphore_puts);

    free(work_buffer);
}


void status_task_runtime_view()
{
    char *work_buffer;

    work_buffer = (char *)malloc(40*20);

    vTaskGetRunTimeStats(work_buffer);
    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
    puts_direct(work_buffer);
    xSemaphoreGive(semaphore_puts);

    free(work_buffer);
}

uint32_t register_sp_get()
{
   uint32_t result;

  __asm volatile ("mov   %0, sp" : "=r" (result) : );
  return(result);
}

void status_heap_size_view()
{
    char *work_buffer;

    work_buffer = (char *)malloc(40*20);

    sprintf(work_buffer, "current sp: %08x, free heap size: %d[bytes]\r\n", (unsigned int)register_sp_get(), (unsigned int)xPortGetFreeHeapSize());
    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
    puts_direct(work_buffer);
    xSemaphoreGive(semaphore_puts);

    free(work_buffer);
}
