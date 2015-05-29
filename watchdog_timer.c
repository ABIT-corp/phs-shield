#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "phs_shield.h"
#include "rtc.h"
#include "watchdog_timer.h"
#include "power_saving.h"


#define WATCHDOGTIMER_LIMIT 0x00013600  /* about 30[s] */

extern struct_psld psld;


#if 0
/* IWDG handler declaration */
IWDG_HandleTypeDef IwdgHandle;
uint32_t watchdogtimer_use;

/**
  * Configures the Watchdog Timer.
  * @param  None
  * @return None
  */
void watchdogtimer_configuration(void)
{
    uint32_t stop_word;

    watchdogtimer_use = 0;

    if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET){
        __HAL_RCC_CLEAR_RESET_FLAGS();
        stop_word = rtc_backup_read();

        if(stop_word == SYSRESET_STOP_MODE){
            stopmode_entry();
            return;
        }

    }

    IwdgHandle.Instance = IWDG;
    IwdgHandle.Init.Prescaler = IWDG_PRESCALER_256;
    IwdgHandle.Init.Reload    = 0xfff;

    if(HAL_IWDG_Init(&IwdgHandle) != HAL_OK){
        /* Initialization Error */
        error_handler("wdt_1");
    }
    watchdogtimer_use = 1;
}


void watchdogtimer_start()
{
    if(watchdogtimer_use){
        HAL_IWDG_Start(&IwdgHandle);
    }
}


void watchdogtimer_stop()
{
    //running IWDG cannot be stopped except reset
}


void watchdogtimer_refresh()
{
    HAL_IWDG_Refresh(&IwdgHandle);
}

void watchdogtimer_counter()
{
}

void timer4_idle_mode()
{
    __TIM4_CLK_DISABLE();
    watchdogtimer_enable = 2;
}
#else

void watchdogtimer_configuration()
{
    psld.watchdogtimer_count = 0;
    psld.watchdogtimer_enable = 0;
}

void watchdogtimer_start()
{
    psld.watchdogtimer_enable = 1;
}

void watchdogtimer_stop()
{
    psld.watchdogtimer_enable = 0;
}

void watchdogtimer_refresh()
{
    psld.watchdogtimer_count = 0;
}

void watchdogtimer_counter()
{
    psld.watchdogtimer_count++;

    if(psld.watchdogtimer_count > 2*WATCHDOGTIMER_LIMIT){
        rtc_backup_write(19, SYSRESET_WATCHDOG);
        printf_queued_flush();
        NVIC_SystemReset();
    }
}

void timer4_idle_mode()
{
    psld.watchdogtimer_enable = 2;
}



#endif
