/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Private variables ---------------------------------------------------------*/

uint64_t u64IdleTicksCnt=0; // Counts when the OS has no task to execute.
uint64_t tickTime=0;        // Counts OS ticks (default = 1000Hz).

//--------------------------------------------------------------
// TICK (default = 1000Hz)
//--------------------------------------------------------------
    /* vApplicationTickHook() will only be called if configUSE_TICK_HOOK is set
    to 1 in FreeRTOSConfig.h.  It is a hook function that will get called during
    each FreeRTOS tick interrupt.  Note that vApplicationTickHook() is called
    from an interrupt context. */
void vApplicationTickHook( void )
{
	tickTime++;
}


//--------------------------------------------------------------
// IDLE (about 2.5MHz)
//--------------------------------------------------------------
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If the application makes use of the
	vTaskDelete() API function (as this demo application does) then it is also
	important that vApplicationIdleHook() is permitted to return to its calling
	function, because it is the responsibility of the idle task to clean up
	memory allocated by the kernel to any task that has since been deleted. */
void vApplicationIdleHook( void )
{
    ++u64IdleTicksCnt;
}


//--------------------------------------------------------------
// STACK OVERFLOW
//--------------------------------------------------------------
/* Run time stack overflow checking is performed if
configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
function is called if a stack overflow is detected. */
void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
	extern signed int printf(const char *, ...);
#if 0 // zeus2015.2.5
	( void ) pcTaskName;
	( void ) pxTask;
#else
	puts("\r\nStack Overflowed.\r\n");
	puts("The stack overflowed is :");
	puts((char *)pcTaskName);
	puts("\r\n");
#endif
	taskDISABLE_INTERRUPTS();
	for( ;; );
}


//--------------------------------------------------------------
// MALLOC FAILED
//--------------------------------------------------------------
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
void vApplicationMallocFailedHook( void )
{
	puts("\r\nMalloc Failed.\r\n");
	taskDISABLE_INTERRUPTS();
	for( ;; );
}


void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
	volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;
	char string[20];

    /* Parameters are not used. */
#if 0 // zeus2015.2.5
    ( void ) ulLine;
    ( void ) pcFileName;
#else
	puts("\r\nAssertion!! task: ");
	puts((char *)pcFileName);
	sprintf(string, ", line: %d\r\n", (int)ulLine);
	puts(string);
#endif

    taskENTER_CRITICAL();
    {
        /* You can step out of this function to debug the assertion by using
        the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
        value. */
        while( ulSetToNonZeroInDebuggerToContinue == 0 )
        {
        }
    }
    taskEXIT_CRITICAL();
}
