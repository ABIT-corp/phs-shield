/*
 * ARDUINO Command API
 *
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "phs_shield.h"
#include "arduino_api.h"
#include "modem.h"
#include "tcp_client.h"
#include "rtc.h"
#include "phs_shield_uart.h"
#include "flash_memory.h"


extern struct_psld psld;


/*
 * Arduino command interpreter
 * return: no
 */

void arduino_api()
{
    uint32_t i, size;
    uint16_t received_byte, timeout_count,status;
    uint8_t command[4], *answer, data;

    /* search first command character */
    answer = (uint8_t *)psld.arduino_work_memory;
    received_byte = uart5_available();
    if(received_byte<3){
        return;
    }
    for(i=0;i<received_byte;i++)
    {
        command[0] = uart5_getchar();
        if(command[0]=='$'){
            break;
        }else{
            return;
        }
    }

    /* get second and third command character  */
    received_byte = uart5_available();
    if(received_byte<2){
        return;
    }
    command[1] = uart5_getchar();
    command[2] = uart5_getchar();
    command[3] = '\0';
    if(command[1]=='$'){
        printf("TRY_CMD:%02x:%02x:%02x\n", command[0], command[1], command[2]);
        command[1] = uart5_getchar();
        command[2] = uart5_getchar();
    }else if(command[2]=='$'){
        printf("TRY_CMD:%02x:%02x:%02x\n", command[0], command[1], command[2]);
        command[1] = uart5_getchar();
        command[2] = uart5_getchar();
    }

    /* search command from list to get id */
    size = sizeof(command_list) / sizeof(command_list[0]);
    for(i=0;i<size;i++)
    {
        if(strcmp((char *)command, command_list[i].name)==0){
            break;
        }
    }
    if(i==size){
        printf("ERR_CMD:%02x:%02x:%02x\n", command[0], command[1], command[2]);
        return;
    }
    printf("CMD:%s\n\r", command_list[i].name);
    strcpy((char *)answer, command_list[i].name);
    answer[3]='=';
    answer[4]='\0';

    timeout_count = 0;

    /* execute each command */
    switch(command_list[i].id)
    {
    case CMD_RESTART:
    {
        printf("system restart request\n");
        lwip_main_close(1);

        /* wait for IEM_REGULATOR_PIN activity */
        vTaskDelay(25*1000);
        printf("system restart\n");
        rtc_backup_write(19, SYSRESET_COMMAND);
        printf_queued_flush();
        NVIC_SystemReset();
        break;
    }
    case CMD_GETSERVICE:
        strcat((char *)answer, "OK 1\n");
        uart5_send(answer,  strlen((char *)answer));
        break;
    case CMD_GETIMEI:
    {
        uint8_t number[12];
        if(modem_imei_get(number)){
            strcat((char *)answer, "NG");
        }else{
            sprintf((char *)&answer[4], "OK %s\n", (char *)number);
        }
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    case CMD_GETTIME:
    {
        uint8_t showtime[12], showdate[12];

        rtc_calendar_show(showtime, showdate);

        strcat((char *)answer, "OK ");
        strcat((char *)answer, (char *)showdate);
        strcat((char *)answer, " ");
        strcat((char *)answer, (char *)showtime);
        strcat((char *)answer, "\n");
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    case CMD_GETRSSI:
    {
        uint8_t number[4];

        if(modem_rssi_get(number)){
            strcat((char *)answer, "NG");
        }else{
            strcat((char *)answer, "OK ");
            strcat((char *)answer, (char *)number);
            strcat((char *)answer, "\n");
        }
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    case CMD_GETVERSION:
        strcat((char *)answer, "OK ");
        strcat((char *)answer, ARDUINO_API_VERSION);
        strcat((char *)answer, "\n");
        uart5_send(answer,  strlen((char *)answer));
        break;
    case CMD_GETLOCATION:
    {
        uint8_t location[32];

        if(modem_location_get(location)){
            strcat((char *)answer, "NG");
        }else{
            strcat((char *)answer, "OK ");
            strcat((char *)answer, (char *)location);
            strcat((char *)answer, "\n");
        }
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    case CMD_SETBAUDRATE:
    {
        uint8_t baud[8], bit;
        uint16_t baudrate;

        vTaskDelay(TRANSMISSION_TIME(8));
        if(arduino_check_parameter(answer, &received_byte, 5)){
            break;
        }

        /* get baud-rate data */
        for(i=bit=0;bit<7;bit++){
            data = uart5_getchar();
            if(data == ' '){
                continue;
            }else if(isdigit(data)==0){
                continue;
            }else{
                baud[i++] = data;
            }
        }
        baud[i] = '\0';
        baudrate = (uint16_t)atoi((char *)baud);
        if((baudrate != 2400)&&(baudrate !=  4800)&&\
           (baudrate != 9600)&&(baudrate != 19200)&&\
           (baudrate !=38400)&&(baudrate != 57600)&&(baudrate != 115200)){
            strcat((char *)answer, "NG\n");
        }else{
            strcat((char *)answer, "OK\n");
        }
        uart5_send(answer,  strlen((char *)answer));
        vTaskDelay(200);
        if(uart5_set_baudrate(baudrate)){
            printf("New UART5 buad rate: %d\n", baudrate);
        }
        break;
    }
    case CMD_SETLED:
    {
        uint8_t switch_on;

        vTaskDelay(TRANSMISSION_TIME(2));
        if(arduino_check_parameter(answer, &received_byte, 2)){
            break;
        }

        /* get control data(1 or 0) */
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == ' '){
                continue;
            }else if(isdigit(data)==0){
                continue;
            }else{
                if(data=='1'){
                    switch_on = 1;
                }else{
                    switch_on = 0;
                }
            }
        }
        led_control(0, switch_on);

        strcat((char *)answer, "OK\n");
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    case CMD_PROFILESET:
    {
        uint8_t profile[3], bit;

        vTaskDelay(TRANSMISSION_TIME(3));
        if(arduino_check_parameter(answer, &received_byte, 3)){
            break;
        }

        /* get profile data */
        for(i=bit=0;bit<3;bit++){
            data = uart5_getchar();
            if(data == ' '){
                continue;
            }else if(isdigit(data)==0){
                continue;
            }else{
                profile[i++] = data;
            }
        }
        profile[i] = '\0';
        psld.profile_number = atoi((char *)profile) & 0xff;
        profile_update();

        strcat((char *)answer, "OK\n");
        uart5_send(answer,  strlen((char *)answer));
        psld.profile_number_changed = 1;
        break;
    }
    case CMD_PROFILEGET:
        sprintf((char *)&answer[4], "OK %d\n", (int)psld.profile_number);
        uart5_send(answer,  strlen((char *)answer));
        break;
    case CMD_SMSSEND:
    {
        uint8_t sms_number[12], sms_code[8], *sms_message;

        sms_message = (uint8_t *)(psld.arduino_work_memory + ARDUINO_RESPONSE_SIZE);
        vTaskDelay(TRANSMISSION_TIME(90));
        if(arduino_check_parameter(answer, &received_byte, 13)){
            break;
        }

        /* get call number */
        i=0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data=='\"'){
                break;
            }else if(data == ' '){
                continue;
            }else if(isdigit(data)==0){
                continue;
            }else{
                sms_number[i++] = data;
            }
        }
        sms_number[i] = '\0';

        /* get message */
        received_byte = uart5_available();
        if(received_byte<2){
            strcat((char *)answer, "NG 2\n");
            uart5_send(answer,  strlen((char *)answer));
            break;
        }
        i=0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data=='\"'){
                break;
            }else{
                sms_message[i++] = data;
            }
            if(arduino_check_buffer(0, &received_byte, &timeout_count)){
                break;
            }
            if(i>ARDUINO_WORKING_SIZE){
                break;
            }
        }
        sms_message[i] = '\0';

        /* get character code */
        received_byte = uart5_available();
        i=0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data=='\r' || data=='\n'){
                break;
            }
            if(data == ' '){
                continue;
            }else{
                sms_code[i++] = data;
            }
        }
        sms_code[i] = '\0';

        /* strip character code */
        if(strcmp((char *)sms_code, "ASCII")!=0){
            strcat((char *)answer, "NG 3\n");
            uart5_send(answer,  strlen((char *)answer));
            break;
        }

        /* send mail data to modem */
        if(modem_mail_write(sms_number, sms_message)){
            strcat((char *)answer, "NG 4\n");
            uart5_send(answer,  strlen((char *)answer));
            break;
        }
        strcat((char *)answer, "OK\n");
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    case CMD_SMSAVAILABLE:
        if(modem_mail_available()){
            strcat((char *)answer, "NG\n");
        }else{
            strcat((char *)answer, "OK 1\n");
        }
        uart5_send(answer,  strlen((char *)answer));
        break;
    case CMD_SMSREAD:
    {
        int8_t *number, *message;
        /* receive mail data from modem */
        if(modem_mail_available()){
            strcat((char *)answer, "NG\n");
        }else{
            modem_mail_get(&number, &message);
            strcat((char *)answer, "OK ");
            strcat((char *)answer, (char *)number);
            strcat((char *)answer, " \"");
            strcat((char *)answer, (char *)message);
            strcat((char *)answer, "\"\n");
        }
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    case CMD_TCPCONNECT:
    {
        uint8_t *server, port[8];
        uint16_t port_number;

        server = (uint8_t *)(psld.arduino_work_memory + ARDUINO_RESPONSE_SIZE);
        vTaskDelay(TRANSMISSION_TIME(30));
        if(arduino_check_parameter(answer, &received_byte, 5)){
            break;
        }

        /* skip until space */
        arduino_skip_until_space((uint16_t *)&received_byte);

        /* save server string */
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == ' ' || data=='\r' || data=='\n'){
                break;
            }else{
                server[i++] = data;
            }
            if(arduino_check_buffer(0, (uint16_t *)&received_byte, (uint16_t *)&timeout_count)){
                break;
            }
            if(i>ARDUINO_BUFFER_SIZE){
                break;
            }
        }
        server[i] = '\0';

        /* save port string */
        received_byte = uart5_available();
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == ' ' || data=='\r' || data=='\n'){
                continue;
            }else{
                port[i++] = data;
            }
        }
        port[i] = '\0';
        port_number = (uint16_t)atoi((char *)port);

        status = tcp_client_connect(server, port_number);
        if(status){
            if(status<3){
                status = 606;
            }else if(status<5){
                status = 604;
            }else if(status==5){
                status = 605;
            }else if(status==6){
                status = 609;
            }else if(status==7){
                status = 607;
            }
            strcat((char *)answer, "NG ");
            sprintf((char *)&answer[7], "%d\n", status);
        }else{
            strcat((char *)answer, "OK\n");
        }
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    case CMD_TCPDISCONNECT:
        tcpip_tcp_read_buffer_free(1);
        status = tcpip_tcp_disconnect();
        if(status){
            strcat((char *)answer, "NG ");
            sprintf((char *)&answer[7], "%d\n", status);
        }else{
            strcat((char *)answer, "OK\n");
        }
        uart5_send(answer,  strlen((char *)answer));
        break;
    case CMD_TCPWRITE:
    {
        uint8_t data, code[4], *buffer;
        uint16_t total_byte;

        buffer = (uint8_t *)(psld.arduino_work_memory + ARDUINO_RESPONSE_SIZE);
        vTaskDelay(TRANSMISSION_TIME(30));
        if(arduino_check_parameter(answer, &received_byte, 5)){
            break;
        }

        /* skip space, break '"' */
        arduino_skip_space_break_double_quotation(&received_byte);

        /* save data until receiving '"' */
        i = 0;
        while(received_byte>0){
            if(arduino_check_buffer(4, (uint16_t *)&received_byte, (uint16_t *)&timeout_count)){
                break;
            }
            data = uart5_getchar();
            received_byte--;
            if(data == '"' /*|| data=='\r' || data=='\n'*/){
                break;
            }else if(data=='$'){
                data = uart5_getchar();
                received_byte--;
                if(data=='"'){
                    buffer[i++] = '"';
                }else if(data=='$'){
                    buffer[i++] = '$';
                }else if(data=='x'){
                    code[0] = uart5_getchar();
                    code[1] = uart5_getchar();
                    code[2] = '\0';
                    data = (int8_t)strtol((char *)code, NULL, 16);
                    buffer[i++] = data;
                    received_byte-=2;
                }
            }else{
                buffer[i++] = data;
            }
            if(i>ARDUINO_BUFFER_SIZE){
                break;
            }
        }
        total_byte = i;

        /* send binary data to modem */
        if(tcpip_tcp_write(buffer, total_byte)){
            sprintf((char *)&answer[4], "NG\n");

        }else{
            sprintf((char *)&answer[4], "OK %d\n", total_byte);
        }
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    case CMD_TCPREAD:
    {
        uint8_t send_byte_string[8];
        uint16_t bit, send_byte_request;

        vTaskDelay(TRANSMISSION_TIME(3));
        if(arduino_check_parameter(answer, (uint16_t *)&received_byte, 3)){
            break;
        }

        /* get number of data that Arduino can receive*/
        for(i=bit=0;bit<5;bit++){
            data = uart5_getchar();
            if(data == ' '){
                continue;
            }else if(isdigit(data)==0){
                continue;
            }else{
                send_byte_string[i++] = data;
            }
        }
        send_byte_string[i] = '\0';
        send_byte_request = (uint16_t)atoi((char *)send_byte_string);

        /* get number of received data, and response to Arduino */
        arduino_tcpip_data_read(answer, send_byte_request);

        break;
    }
    case CMD_HTTPGET:
    {
        uint8_t *header, *path, *server, port[8], ssl;
        uint16_t port_number, j, send_byte_request;

        server = (uint8_t *)(psld.arduino_work_memory + ARDUINO_RESPONSE_SIZE);
        vTaskDelay(TRANSMISSION_TIME(8));
        if(arduino_check_parameter(answer, (uint16_t *)&received_byte, 8)){
            break;
        }

        /* save SSL from http or https */
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == ' '){
                continue;
            }else if(data == '/'){
                break;
            }else{
                port[i++] = data;
            }
        }
        if(strstr((char *)port, "https:")){
            ssl = 1;
            port_number = 80;
        }else{
            ssl = 0;
            port_number = 80;
        }

        /* save server (and port) string */
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == '/'){
                continue;
            }else if(data == ':'){
                server[i] = '\0';
                path = &server[i+1];

                /* save port string */
                j = 0;
                while(received_byte>0){
                    data = uart5_getchar();
                    received_byte--;
                    if(isdigit(data)==0){
                        break;
                    }else{
                        port[j++] = data;
                    }
                }
                port[j] = '\0';
                port_number = (uint16_t)atoi((char *)port);
                break;
            }else{
                server[i++] = data;
            }
            if(arduino_check_buffer(0, (uint16_t *)&received_byte, (uint16_t *)&timeout_count)){
                break;
            }
            if(i>ARDUINO_BUFFER_SIZE){
                break;
            }
        }


        /* save path string */
        i = 0;
        while(received_byte>0){
            if(i!=0){
                data = uart5_getchar();
                received_byte--;
            }
            if(data == ' '||data == '\r'||data == '\n'){
                break;
            }else{
                path[i++] = data;
            }
            if(arduino_check_buffer(0, (uint16_t *)&received_byte, (uint16_t *)&timeout_count)){
                break;
            }
            if(i>ARDUINO_BUFFER_SIZE){
                break;
            }
        }
        path[i] = '\0';
        header = &path[i+1];

        /* skip space & '"' */
        received_byte = uart5_available();
        arduino_skip_space_break_double_quotation((uint16_t *)&received_byte);

        /* save header string */
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == '\"'){
                break;
            }else{
                header[i++] = data;
            }
            if(arduino_check_buffer(0, (uint16_t *)&received_byte, (uint16_t *)&timeout_count)){
                break;
            }
            if(i>ARDUINO_BUFFER_SIZE){
                break;
            }
        }
        header[i] = '\0';

        status = tcpip_tcp_httpget(server, port_number, path, header, ssl);

        if(status){
            strcat((char *)answer, "NG ");
            sprintf((char *)&answer[7], "%d\n", status);
            uart5_send(answer,  strlen((char *)answer));
        }else{
            send_byte_request = 128;
            arduino_tcpip_data_read(answer, send_byte_request);
        }

        break;
    }
    case CMD_HTTPPOST:
    {
        uint8_t *header, *body, *path, *server, port[8], ssl;
        uint16_t port_number, j, send_byte_request;

        server = (uint8_t *)(psld.arduino_work_memory + ARDUINO_RESPONSE_SIZE);
        vTaskDelay(TRANSMISSION_TIME(8));
        if(arduino_check_parameter(answer, (uint16_t *)&received_byte, 8)){
            break;
        }

        /* save SSL from http or https */
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == ' '){
                continue;
            }else if(data == '/'){
                break;
            }else{
                port[i++] = data;
            }
        }
        if(strstr((char *)port, "https:")){
            ssl = 1;
            port_number = 80;
        }else{
            ssl = 0;
            port_number = 80;
        }

        /* save server (and port) string */
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == '/'){
                continue;
            }else if(data == ':'){
                server[i] = '\0';
                path = &server[i+1];

                /* save port string */
                j = 0;
                while(received_byte>0){
                    data = uart5_getchar();
                    received_byte--;
                    if(isdigit(data)==0){
                        break;
                    }else{
                        port[j++] = data;
                    }
                }
                port[j] = '\0';
                port_number = (uint16_t)atoi((char *)port);
                break;
            }else{
                server[i++] = data;
            }
            if(arduino_check_buffer(0, (uint16_t *)&received_byte, (uint16_t *)&timeout_count)){
                break;
            }
            if(i>ARDUINO_BUFFER_SIZE){
                break;
            }
        }


        /* save path string */
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == ' '||data == '\r'||data == '\n'){
                break;
            }else{
                path[i++] = data;
            }
            if(arduino_check_buffer(0, (uint16_t *)&received_byte, (uint16_t *)&timeout_count)){
                break;
            }
            if(i>ARDUINO_BUFFER_SIZE){
                break;
            }
        }
        path[i] = '\0';
        body = &path[i+1];

        /* skip space & '"' */
        received_byte = uart5_available();
        arduino_skip_space_break_double_quotation((uint16_t *)&received_byte);

        /* save body string */
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == '\"'){
                data = uart5_getchar();
                received_byte--;
                if(data == '\"'){
                    continue;
                }else{
                    break;
                }
            }else{
                body[i++] = data;
            }
            if(arduino_check_buffer(0, (uint16_t *)&received_byte, (uint16_t *)&timeout_count)){
                break;
            }
            if(i>ARDUINO_BUFFER_SIZE){
                break;
            }
        }
        body[i] = '\0';
        header = &body[i+1];

        arduino_skip_space_break_double_quotation((uint16_t *)&received_byte);

        /* save header string */
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == '\"'){
                break;
            }else{
                header[i++] = data;
            }
            if(arduino_check_buffer(0, (uint16_t *)&received_byte, (uint16_t *)&timeout_count)){
                break;
            }
            if(i>ARDUINO_BUFFER_SIZE){
                break;
            }
        }
        header[i] = '\0';

        status = tcpip_tcp_httppost(server, port_number, path, header, body, ssl);

        if(status){
            strcat((char *)answer, "NG ");
            sprintf((char *)&answer[7], "%d\n", status);
            uart5_send(answer,  strlen((char *)answer));
        }else{
            send_byte_request = 128;
            arduino_tcpip_data_read(answer, send_byte_request);
        }

        break;
    }
    case CMD_FLASHPUT:
    {
        uint8_t data, code[4], *buffer, sector;
        uint16_t total_byte;

        buffer = (uint8_t *)(psld.arduino_work_memory + ARDUINO_RESPONSE_SIZE);
        vTaskDelay(TRANSMISSION_TIME(30));
        if(arduino_check_parameter(answer, &received_byte, 5)){
            break;
        }

        /* skip space, break '"' */
        i = 0;
        while(received_byte>0){
            data = uart5_getchar();
            received_byte--;
            if(data == ' '){
                continue;
            }else if(data=='\"'){
                break;
            }else{
                 code[i++] = data;
            }
        }
        code[i] = '\0';
        sector = atoi((char *)code);

        /* save data until receiving '"' */
        i = 0;
        while(received_byte>0){
            if(arduino_check_buffer(4, (uint16_t *)&received_byte, (uint16_t *)&timeout_count)){
                break;
            }
            data = uart5_getchar();
            received_byte--;
            if(data == '"' /*|| data=='\r' || data=='\n'*/){
                break;
            }else if(data=='$'){
                data = uart5_getchar();
                received_byte--;
                if(data=='"'){
                    buffer[i++] = '"';
                }else if(data=='$'){
                    buffer[i++] = '$';
                }else if(data=='x'){
                    code[0] = uart5_getchar();
                    code[1] = uart5_getchar();
                    code[2] = '\0';
                    data = (int8_t)strtol((char *)code, NULL, 16);
                    buffer[i++] = data;
                    received_byte-=2;
                }
            }else{
                buffer[i++] = data;
            }
            if(i>ARDUINO_BUFFER_SIZE){
                break;
            }
        }
        total_byte = i;

        /* send binary data to modem */
        if(flash_sector_write(sector, buffer, &total_byte)){
            sprintf((char *)&answer[4], "NG\n");

        }else{
            sprintf((char *)&answer[4], "OK %d\n", total_byte);
        }
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    case CMD_FLASHGET:
    {
        uint8_t sector_string[8];
        uint16_t bit, sector;

        vTaskDelay(TRANSMISSION_TIME(3));
        if(arduino_check_parameter(answer, (uint16_t *)&received_byte, 3)){
            break;
        }

        /* get number of data that Arduino can receive*/
        for(i=bit=0;bit<5;bit++){
            data = uart5_getchar();
            if(data == ' '){
                continue;
            }else if(isdigit(data)==0){
                continue;
            }else{
                sector_string[i++] = data;
            }
        }
        sector_string[i] = '\0';
        sector = (uint16_t)atoi((char *)sector_string);

        /* get number of received data, and response to Arduino */
        arduino_flash_data_read(answer, sector, 1024);

        break;
    }
    default:
        printf("No supported command\n");
        strcat((char *)answer, "NG\n");
        uart5_send(answer,  strlen((char *)answer));
        break;
    }
    arduino_watchdog_clear();
}



/*
 * Discard unused data
 *   return : none
 */
void arduino_flush()
{
    uint16_t num, i;

    num = uart5_available();
    if(num>0){
        for(i=0;i<num;i++)
        {
            uart5_getchar();
        }
    }
}


/*
 * Check parameter sent from arduino
 *   answer : pointer to address that store response data to arduino
 *   received_byte: pointer to address that store received data
 *   estimated_parameter: estimated number of received data
 *   return : status[1:error 0:pass]
 */
uint8_t arduino_check_parameter(uint8_t *answer, uint16_t *received_byte, uint8_t estimated_parameter)
{
    *received_byte = uart5_available();
    if(*received_byte<estimated_parameter){
        strcat((char *)answer, "NG 1\n");
        uart5_send(answer,  strlen((char *)answer));
        return 1;
    }
    return 0;
}


/*
 * Skip space, break out on double_quotation
 *   received_byte: pointer to address that store received data
 *   return : none
 */
void arduino_skip_space_break_double_quotation(uint16_t *received_byte)
{
    uint8_t data;

    while(*received_byte>0){
        data = uart5_getchar();
        *received_byte = *received_byte - 1;
        if(data == ' '){
            continue;
        }else if(data=='\"'){
            break;
        }
    }
}


/*
 * Skip space
 *   received_byte: pointer to address that store received data
 *   return : none
 */
void arduino_skip_until_space(uint16_t *received_byte)
{
    uint8_t data;

    while(*received_byte>0){
        data = uart5_getchar();
        *received_byte = *received_byte - 1;
        if(data == ' '){
            break;
        }
    }
}


/*
 * Check unread data sent from arduino
 *   byte_remained: if unread data is bellow [byte_remained], it begins to wait
 *   received_byte: pointer to address that store received data
 *   timeout_count: timeout[100msec]
 *   return : status[1:error 0:pass]
 */
uint8_t arduino_check_buffer(uint16_t byte_remained, uint16_t *received_byte, uint16_t *timeout_count)
{
    if(*received_byte<=byte_remained){
        vTaskDelay((uint32_t)100);
        *received_byte = uart5_available();
        if(*timeout_count++>ARDUINO_TIMEOUT_SECOND*10){
            return 1;
        }
    }
    return 0;
}


/*
 * Transfer data to arduino from tcpip
 *   answer : pointer on address that store response data to arduino
 *   byte_request: count of data that arduino requests
 *   return : status[1:error 0:pass]
 */
uint8_t arduino_tcpip_data_read(uint8_t *answer, uint16_t request_byte)
{
    int16_t status;
    uint16_t send_byte,avaliable_byte;
    uint8_t *buffer;

    avaliable_byte = request_byte;

    if((status = tcpip_tcp_read(&buffer, (uint16_t *)&avaliable_byte, 15))){
        strcat((char *)answer, "NG ");
        sprintf((char *)&answer[7], "%d\n", status);
        uart5_send(answer,  strlen((char *)answer));
        return 1;
    }
    send_byte = avaliable_byte;

    strcat((char *)answer, "OK ");
    sprintf((char *)&answer[7], "%d\n", send_byte);
    uart5_send(answer,  strlen((char *)answer));
    // wait until DMA release buffer
    vTaskDelay(TRANSMISSION_TIME(30));

    memcpy((char *)&answer[0], (char *)buffer, send_byte);

    answer[send_byte] = '\0';
    strcat((char *)&answer[send_byte], "\n");
    uart5_send(answer,  send_byte+1); // Don't use strlen(), contents of answer[] is binary.

    tcpip_tcp_read_buffer_free(0);
    return 0;
}

/*
 * Transfer data to arduino from flash memory
 *   answer : pointer on address that store response data to arduino
 *   byte_request: count of data that arduino requests
 *   return : status[1:error 0:pass]
 */
uint8_t arduino_flash_data_read(uint8_t *answer, uint16_t sector, uint16_t request_byte)
{
    uint16_t send_byte;

    send_byte = request_byte;

    if((request_byte > 1024) || (sector == 0) || (sector > 128)){
        strcat((char *)answer, "NG ");
        sprintf((char *)&answer[7], "%d\n", 1);
        uart5_send(answer,  strlen((char *)answer));
        return 1;
    }

    strcat((char *)answer, "OK ");
    sprintf((char *)&answer[7], "%d\n", send_byte);
    uart5_send(answer,  strlen((char *)answer));
    vTaskDelay(TRANSMISSION_TIME(15)); // wait until DMA release buffer

    flash_sector_read(sector, &answer[0], &send_byte);

    answer[send_byte] = '\0';
    strcat((char *)&answer[send_byte], "\n");
    uart5_send(answer, send_byte+1); // Don't use strlen(), contents of answer[] is binary.

    return 0;
}


/*
 * Get power control request from arduino
 * return: 1:request for power on, 0:request for power off
 */
uint8_t arduino_request_power()
{
    return (uint8_t)HAL_GPIO_ReadPin(ARDUINO_POWERON_PORT, ARDUINO_POWERON_PIN);
}

uint8_t arduino_request_regurator()
{
    return (uint8_t)HAL_GPIO_ReadPin(ARDUINO_REGON_PORT, ARDUINO_REGON_PIN);
}


/*
 * Trigger interrupt to Arduino
 * on : 1:Enable interrupt, 0:Disable interrupt
 */
void arduino_interruput(uint8_t on)
{
    if(on){
        HAL_GPIO_WritePin(ARDUINO_INTERRUPT_PORT, ARDUINO_INTERRUPT_PIN , 0);
    }else {
        HAL_GPIO_WritePin(ARDUINO_INTERRUPT_PORT, ARDUINO_INTERRUPT_PIN , 1);
    }
}

uint32_t arduino_watchdog_count;

void arduino_watchdog_clear()
{
    arduino_watchdog_count = 0;
}

void arduino_watchdog_timer()
{
    arduino_watchdog_count++;
    if(arduino_watchdog_count>800){
        printf("reset arduino\n");
        arduino_interruput(1);
        vTaskDelay(1000);
        arduino_interruput(0);
        arduino_watchdog_count = 0;
    }
}
