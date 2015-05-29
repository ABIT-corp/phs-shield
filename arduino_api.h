#ifndef ARDUINO_API_H
#define ARDUINO_API_H


#define    ARDUINO_RESPONSE_SIZE 20
#define    ARDUINO_BUFFER_SIZE (ARDUINO_WORKING_SIZE - ARDUINO_RESPONSE_SIZE)
#define    ARDUINO_TIMEOUT_SECOND 15

#define UART5_BUARDRATE_TIME (psld.uart5_handle.Init.BaudRate==  2400 ? 40:\
                             (psld.uart5_handle.Init.BaudRate==  4800 ? 20:\
                             (psld.uart5_handle.Init.BaudRate==  9600 ? 10:\
                             (psld.uart5_handle.Init.BaudRate== 19200 ?  5:\
                             (psld.uart5_handle.Init.BaudRate== 38400 ?  3:\
                             (psld.uart5_handle.Init.BaudRate== 57600 ?  2:\
                             (psld.uart5_handle.Init.BaudRate==115200 ?  1:10)))))))/* 1 byte transmission time[0.1msec/word] */

#define TRANSMISSION_TIME(BYTE) ((uint32_t)((BYTE * UART5_BUARDRATE_TIME)/10)) /* 1 byte transmission time at 9600bps */


#define    STR_RESTART       "$YE"  /* Shutdown PHS shield                                   */
#define    STR_GETSERVICE    "$YS"  /* Get network service status                            */
#define    STR_GETIMEI       "$YI"  /* Get IEM's IMEI                                        */
#define    STR_GETTIME       "$YT"  /* Get current date and time                             */
#define    STR_GETRSSI       "$YR"  /* Get 3G RSSI                                           */
#define    STR_GETVERSION    "$YV"  /* Get Version                                           */
#define    STR_GETLOCATION   "$LG"  /* get Current Location                                  */
#define    STR_SETBAUDRATE   "$YB"  /* Set UART baud rate                                    */
#define    STR_SETLED        "$YL"  /* Set LED On/Off                                        */
#define    STR_PROFILESET    "$PS"  /* Set default profile(APN) number                       */
#define    STR_PROFILEGET    "$PR"  /* Get default profile(APN) number                       */
#define    STR_SMSSEND       "$SS"  /* Send SMS                                              */
#define    STR_SMSAVAILABLE  "$SC"  /* Check SMS                                             */
#define    STR_SMSREAD       "$SR"  /* Read received SMS                                     */
#define    STR_TCPCONNECT    "$TC"  /* Connect to server with TCP/IP connection              */
#define    STR_TCPDISCONNECT "$TD"  /* Disconnect from current server with TCP/IP connection */
#define    STR_TCPWRITE      "$TW"  /* Write byte into current TCP/IP connection             */
#define    STR_TCPREAD       "$TR"  /* Read data from current TCP/IP connection              */
#define    STR_HTTPGET       "$WG"  /* Request HTTP GET to server and port                   */
#define    STR_HTTPPOST      "$WP"  /* Request HTTP POST to server and port                  */
#define    STR_FLASHPUT      "$RW"  /* Set data to flash memory                              */
#define    STR_FLASHGET      "$RR"  /* Get data from flash memory                            */

typedef enum {
    CMD_RESTART = 1,
    CMD_GETSERVICE,
    CMD_GETIMEI,
    CMD_GETTIME,
    CMD_GETRSSI,
    CMD_GETVERSION,
    CMD_GETLOCATION,
    CMD_SETBAUDRATE,
    CMD_SETLED,
    CMD_PROFILESET,
    CMD_PROFILEGET,
    CMD_SMSSEND,
    CMD_SMSAVAILABLE,
    CMD_SMSREAD,
    CMD_TCPCONNECT,
    CMD_TCPDISCONNECT,
    CMD_TCPWRITE,
    CMD_TCPREAD,
    CMD_HTTPGET,
    CMD_HTTPPOST,
    CMD_FLASHPUT,
    CMD_FLASHGET
} command_id;


typedef struct
{
    const char  *name;
    int arg;
    command_id id;
} struct_command_list;

struct_command_list command_list [] = {
    {STR_RESTART,      0, CMD_RESTART     },
    {STR_GETSERVICE,   0, CMD_GETSERVICE   },
    {STR_GETIMEI,      0, CMD_GETIMEI      },
    {STR_GETTIME,      0, CMD_GETTIME      },
    {STR_GETRSSI,      0, CMD_GETRSSI      },
    {STR_GETVERSION,   0, CMD_GETVERSION   },
    {STR_GETLOCATION,  0, CMD_GETLOCATION  },
    {STR_SETBAUDRATE,  0, CMD_SETBAUDRATE  },
    {STR_SETLED,       0, CMD_SETLED       },
    {STR_PROFILESET,   0, CMD_PROFILESET   },
    {STR_PROFILEGET,   0, CMD_PROFILEGET   },
    {STR_SMSSEND,      0, CMD_SMSSEND      },
    {STR_SMSAVAILABLE, 0, CMD_SMSAVAILABLE },
    {STR_SMSREAD,      0, CMD_SMSREAD      },
    {STR_TCPCONNECT,   0, CMD_TCPCONNECT   },
    {STR_TCPDISCONNECT,0, CMD_TCPDISCONNECT},
    {STR_TCPWRITE,     0, CMD_TCPWRITE     },
    {STR_TCPREAD,      0, CMD_TCPREAD      },
    {STR_HTTPGET,      0, CMD_HTTPGET      },
    {STR_HTTPPOST,     0, CMD_HTTPPOST     },
    {STR_FLASHPUT,     0, CMD_FLASHPUT     },
    {STR_FLASHGET,     0, CMD_FLASHGET     },
    {0, 0, 0}
};



uint8_t arduino_flash_data_read(uint8_t *answer, uint16_t sector, uint16_t request_byte);
uint8_t arduino_tcpip_data_read(uint8_t *, uint16_t);
uint8_t arduino_check_parameter(uint8_t *, uint16_t *, uint8_t);
void arduino_skip_space_break_double_quotation(uint16_t *);
uint8_t arduino_check_buffer(uint16_t, uint16_t *, uint16_t *);
void arduino_skip_until_space(uint16_t *);
void arduino_flush();
void arduino_interruput(uint8_t);



#endif


