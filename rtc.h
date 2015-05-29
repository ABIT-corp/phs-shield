#ifndef RTC_H
#define RTC_H


void rtc_configuration(void);
void rtc_calendar_show(uint8_t* showtime, uint8_t* showdate);
void rtc_time_display(uint32_t);
void error_handler(char *);
void rtc_backup_write(uint32_t, uint32_t);
uint32_t rtc_backup_read(uint32_t);


#endif
