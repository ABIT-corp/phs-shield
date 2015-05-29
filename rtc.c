#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "tcp_client.h"
#include "rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define RTC_ASYNCH_PREDIV  127
#define RTC_SYNCH_PREDIV   249

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} struct_time;

typedef struct
{
    const char *month;
    const char *week;
    uint8_t  number;
} struct_month_list;

struct_time current_time;

struct_month_list month_list [] = {
    {"Jan",  "Mon",  1},
    {"Feb",  "Tue",  2},
    {"Mar",  "Wed",  3},
    {"Apr",  "Thu",  4},
    {"May",  "Fri",  5},
    {"Jun",  "Sat",  6},
    {"Jul",  "sun",  7},
    {"Aug",  "---",  8},
    {"Sep",  "---",  9},
    {"Oct",  "---", 10},
    {"Nov",  "---", 11},
    {"Dec",  "---", 12},
    {0, 0, 0}
};

/* RTC handler declaration */
RTC_HandleTypeDef RtcHandle;

static int32_t rtc_calendar_configuration(struct_time *time);


/**
  * RTC MSP Initialization
  *  This function configures the hardware resources used in this example
  * @param hrtc: RTC handle pointer
  * @note  Care must be taken when HAL_RCCEx_PeriphCLKConfig() is used to select
  *        the RTC clock source; in this case the Backup domain will be reset in
  *        order to modify the RTC Clock source, as consequence RTC registers (including
  *        the backup registers) and RCC_BDCR register are set to their reset values.  *
  * @return None
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
      RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

      PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
      PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
      if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
      {
        error_handler("rtc_msp1");
      }

      /* Enable RTC Clock */
      __HAL_RCC_RTC_ENABLE();
}


/**
  * Configures the RTC.
  * @param  None
  * @return None
  */
void rtc_configuration(void)
{
    RtcHandle.Instance = RTC;

    /* Configure RTC prescaler and RTC data registers */
    /* RTC configured as follow:
    *   - Hour Format    = Format 24
    *   - Asynch Prediv  = Value according to source clock
    *   - Synch Prediv   = Value according to source clock
    *   - OutPut         = Output Disable
    *   - OutPutPolarity = High Polarity
    *   - OutPutType     = Open Drain
    */
    RtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
    RtcHandle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
    RtcHandle.Init.SynchPrediv = RTC_SYNCH_PREDIV;
    RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
    RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if(HAL_RTC_Init(&RtcHandle) != HAL_OK){
        error_handler("rtc");
    }

    /*##-2- Check if Data stored in BackUp register0: No Need to reconfigure RTC#*/
    /* Read the BackUp Register 0 Data */
    if(HAL_RTCEx_BKUPRead(&RtcHandle, RTC_BKP_DR0) != 0x32F2){
        struct_time time;
        time.year = 2014;
        time.month = 3;
        time.date = 15;
        time.week  = 1;
        time.hour = 2;
        time.minute = 10;
        time.second = 0;
        rtc_calendar_configuration(&time);
    }

    //HAL_RTCEx_BKUPWrite(&RtcHandle, 19, 0x0);
}


/**
  * Configure the current time and date.
  * @param  None
  * @return None
  */
int32_t rtc_calendar_configuration(struct_time *time)
{
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;

    /*##-1- Configure the Date ##*/
    /* Set Date: Tuesday February 18th 2014 */
    sdatestructure.Year    = RTC_ByteToBcd2(time->year - 2000);
    sdatestructure.Month   = RTC_ByteToBcd2(time->month);
    sdatestructure.Date    = RTC_ByteToBcd2(time->date);
    sdatestructure.WeekDay = RTC_ByteToBcd2(time->week);

    if(HAL_RTC_SetDate(&RtcHandle,&sdatestructure,FORMAT_BCD) != HAL_OK){
    /* Initialization Error */
        return 1;
    }

    /*##-2- Configure the Time ##*/
    /* Set Time: 02:00:00 */
    stimestructure.Hours   = RTC_ByteToBcd2(time->hour);
    stimestructure.Minutes = RTC_ByteToBcd2(time->minute);
    stimestructure.Seconds = RTC_ByteToBcd2(time->second);
    //stimestructure.TimeFormat     = RTC_HOURFORMAT12_AM;
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

    if(HAL_RTC_SetTime(&RtcHandle,&stimestructure,FORMAT_BCD) != HAL_OK){
    /* Initialization Error */
        return 2;
    }

    /*##-3- Writes a data in a RTC Backup data Register0 #######################*/
    HAL_RTCEx_BKUPWrite(&RtcHandle,RTC_BKP_DR0,0x32F2);

    return 0;
}

void rtc_backup_write(uint32_t address, uint32_t data)
{
    address += RTC_BKP_DR0;
    HAL_RTCEx_BKUPWrite(&RtcHandle, address, data);
}

uint32_t rtc_backup_read(uint32_t address)
{
    uint32_t stop_word;

    address += RTC_BKP_DR0;
    stop_word = HAL_RTCEx_BKUPRead(&RtcHandle, address);
    return stop_word;
}

/**
  * Display the current time and date.
  * @param  showtime : pointer to buffer
  * @param  showdate : pointer to buffer
  * @return None
  */
void rtc_calendar_show(uint8_t* showtime, uint8_t* showdate)
{
  RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructureget;

  HAL_RTC_GetTime(&RtcHandle, &stimestructureget, FORMAT_BIN);
  HAL_RTC_GetDate(&RtcHandle, &sdatestructureget, FORMAT_BIN);

  sprintf((char*)showtime,"%02d:%02d:%02d",stimestructureget.Hours, stimestructureget.Minutes, stimestructureget.Seconds);
  sprintf((char*)showdate,"%02d/%02d/%02d",2000 + sdatestructureget.Year, sdatestructureget.Month, sdatestructureget.Date);
}


void rtc_time_display(uint32_t direct)
{
    uint8_t ShowTime[12], ShowDate[12];
    extern void puts_direct();

    rtc_calendar_show(ShowTime, ShowDate);
    if(direct){
        puts_direct((char *)ShowDate);
        puts_direct(" ");
        puts_direct((char *)ShowTime);
        puts_direct("\r\n");
    }else{
        printf("%s %s\n", (char *)ShowDate, (char *)ShowTime);
    }
}



/*
  * Synchronize RTC time to ntp server
  * @param  none
  * @return none
  */
int32_t rtc_time_synchronize()
{
    uint16_t avaliable_byte, status;
    uint8_t *buffer, str[5], i, *date_position;
    uint8_t server[] = "www.nict.go.jp";
    uint8_t path[] = "";
    uint8_t header[] = "Accept: text/html\r\nConnection: Keep-Close";
    extern xSemaphoreHandle semaphore_puts;

    xSemaphoreTake(semaphore_puts, portMAX_DELAY);
    puts("rtc time synchronizer");
    xSemaphoreGive(semaphore_puts);

    status = tcpip_tcp_httpget(server, 80, path, header, 0);

    if(status){
        return 1;
    }

    avaliable_byte = 50;

    if((status = tcpip_tcp_read(&buffer, (uint16_t *)&avaliable_byte, 15))){
        return 2;
    }
    buffer[avaliable_byte] = '\0';

    date_position = (uint8_t *)strstr((char *)buffer, "Date:");
    date_position += 6;
    i = 0;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = '\0';
    for(i=0;i<7;i++){
        if(strcmp((char *)str, month_list[i].week)==0){
            break;
        }
    }
    current_time.week = month_list[i].number;
    date_position+=2;
    i = 0;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = '\0';
    current_time.date = atoi((char *)str);
    date_position++;
    i = 0;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = '\0';
    for(i=0;i<12;i++){
        if(strcmp((char *)str, month_list[i].month)==0){
            break;
        }
    }
    current_time.month = month_list[i].number;
    date_position++;
    i = 0;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = '\0';
    current_time.year = atoi((char *)str);
    date_position++;
    i = 0;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = '\0';
    current_time.hour = atoi((char *)str) + 9;
    date_position++;
    i = 0;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = '\0';
    current_time.minute = atoi((char *)str);
    date_position++;
    i = 0;
    str[i++] = *date_position++;
    str[i++] = *date_position++;
    str[i++] = '\0';
    current_time.second = atoi((char *)str);

    tcpip_tcp_read_buffer_free();
    tcpip_tcp_disconnect();

    rtc_time_display(0);
    rtc_calendar_configuration(&current_time);

    return 0;
}
