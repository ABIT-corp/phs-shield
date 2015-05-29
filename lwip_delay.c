/*!
 * @file portable_delay.c
 * @brief Portable Delay/Sleep functions
 * @author J. Kunugiza
 * @date 2014/12/20
 */

#ifdef __linux
/* Linux needs this for POSIX compatibility */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199506L
#endif /* of !_POSIX_C_SOURCE */
#endif /* of __linux */

#ifdef __unix__
#include <sys/types.h>
#include <time.h>


int
p_delay(unsigned int seconds)
{
    struct timespec t;
    int result;

    t.tv_sec = (time_t)seconds;
    t.tv_nsec = 0;

    result = nanosleep(&t, NULL);
    return (result);
}


int
p_mdelay(unsigned int milliseconds)
{
    struct timespec t;
    int result;

    if (milliseconds < 1000) {
        t.tv_sec = 0;
        t.tv_nsec = (long)milliseconds * 1000000;
    } else {
        t.tv_sec = (time_t)(milliseconds / 1000);
        t.tv_nsec = (long)(milliseconds % 1000) * 1000000;
    }

    result = nanosleep(&t, NULL);
    return (result);
}

#else


#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

extern uint32_t HAL_GetTick(void);

int
p_delay(unsigned int seconds)
{
    int i;
    for(i=0;i<seconds;i++){
        vTaskDelay(1000);
    }
    return 0;
}


int
p_mdelay(unsigned int milliseconds)
{
    vTaskDelay(milliseconds);
    return 0;
}

unsigned int
sys_now(void)
{
  return (unsigned int)HAL_GetTick();
}

#ifndef MAX_JIFFY_OFFSET
#define MAX_JIFFY_OFFSET ((~0U >> 1)-1)
#endif

#ifndef HZ
#define HZ 100
#endif

unsigned int
sys_jiffies(void)
{
    return (unsigned int)(HAL_GetTick()/(1000/HZ));
}

#include <sys/time.h>

int _gettimeofday( struct timeval *tv, void *tzvp )
{
    uint64_t t = HAL_GetTick();
    tv->tv_sec = t / 1000;              // convert to seconds
    tv->tv_usec = t * 1000;  // get remaining microseconds
    return 0;
}


#endif /* of __unix__ */
