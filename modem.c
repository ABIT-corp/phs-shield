/*!
 * @file 
 * @brief Hayes AT command compatible modem abstraction class
 * @author J. Kunugiza
 * @date 2014/11/1
 */

/* Linux needs this for BSD compatibility */
#ifdef __linux
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif /* of !_BSD_SOURCE */
#endif /* of __linux */

#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <ctype.h>

#include "phs_shield.h"
#include "serial_posix_api.h"
#include "lwip_delay.h"
#include "modem.h"
#include "rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define DBG_AT0 DBG0
#define DBG_AT1 DBG1
#define DBG_ENABLE DBG0
#include "debug.h"


typedef enum {
    MSTATE_IDOL,
    MSTATE_DATA
} mdm_state;



/*!
 * @struct modem_private
 * @brief private data for this class
 */
typedef struct {
    serial *serial;
    mdm_state modem_state;
    result_type result;
    char rx_buf[RX_BUFFER_SIZE];
    char *result_lines[MAX_RESULT_LINES];
    int result_nlines;
} modem_private;


typedef struct {
    const char * const result_str;
    const int result_length;
    const result_type result;
} result_type_list;


#define STR_AND_SIZE(str) (str), (sizeof(str) - 1)

static const result_type_list result_list[] = {
    { STR_AND_SIZE("OK"), RES_OK },
    { STR_AND_SIZE("ERROR"), RES_ERROR },
    { STR_AND_SIZE("CONNECT"), RES_CONNECT },
    { STR_AND_SIZE("NO CARRIER"), RES_NO_CARRIER },
    { STR_AND_SIZE("DELAYED"), RES_DELAYED },
    { STR_AND_SIZE("BUSY"), RES_BUSY },
    { STR_AND_SIZE("RING"), RES_RING },
    { STR_AND_SIZE("PROG"), RES_PROG },
    { STR_AND_SIZE("ALERT"), RES_ALERT },
    { NULL, 0, 0 }
};



static struct_lightmail modem_lightmail;
extern struct_psld psld;
extern xSemaphoreHandle semaphore_puts;


static int atcmd_exec(modem_private *prv, const char *command);
static int atcmd_send(modem_private *prv, const char *command);
static int atcmd_read_result(modem_private *prv);
static int atcmd_parse_result(modem_private *prv);
static int atcmd_escape(modem_private *prv);
static int modem_initialize_sequense(modem *);



/*
 * Instantiate modem object
 * @param : none
 * @return: pointer to object instance on success, NULL on allocation failure
 */

modem *
modem_create(void)
{
    size_t alloc_size;
    modem *obj;
    modem_private *prv;

    alloc_size = sizeof(modem) + sizeof(modem_private);
    prv = (modem_private *) malloc(alloc_size);

    if (prv == NULL) {
        return NULL;
    }
    
    prv->modem_state = MSTATE_IDOL;
    prv->result = RES_UNKNOWN;
    prv->result_nlines = 0;
    prv->serial = NULL;

    obj = (modem *) (prv + 1);
    obj->prv = (void *) prv;

    return (obj);
}

/*
 * @brief Destroy wsim object instance
 * @param[in]: obj pointer to object instance
 * @return   : None
 */
void
modem_destroy(modem *obj)
{
    modem_private *prv;
    
    if (obj == NULL) {
        return;
    }
    
    prv = (modem_private *) obj->prv;
    free(prv);

    return;
}

/*
 * @brief attach to serial device
 * @param[in]: obj pointer to object instance
 * @param[in]: device an instance of serial class
 * @return   : [0:success, -1:failure]
 * @note 
 */
int
modem_attach(modem *obj, serial *device)
{
    modem_private *prv;

    prv = (modem_private *) obj->prv;

    if (prv->serial != NULL) {
        return(-1);
    }

    prv->serial = device;
    return (0);
}

int
modem_initialize(modem *obj)
{
    modem_private *prv;

    prv = (modem_private *) obj->prv;

    psld.modem_check_ringcall_interrupt_flag = 0;

    if (serial_mode_canonical(prv->serial) < 0) {
        return (-1);
    }

    prv->modem_state = MSTATE_IDOL;
    atcmd_escape(prv);
    
    /*
     * try to reset the modem into known state
     */
    return modem_initialize_sequense(obj);
}

int
modem_initialize_sequense(modem *obj)
{
    modem_private *prv;

    prv = (modem_private *) obj->prv;

    modem_reset(1);
    p_mdelay(100);
    modem_reset(0);
    p_mdelay(1000);

#if defined(__linux)
    atcmd_send(prv, "\r\nAT");
#else
    atcmd_send(prv, "AT");
#endif
    p_mdelay(500);

    serial_flush(prv->serial);

    atcmd_exec(prv, MODEM_RESET);
    if (prv->result != RES_OK) {
        return (-1);
    }
    psld.modem_status = MODEM_ATZ;

    atcmd_exec(prv, MODEM_COMMAND_ECHO_OFF);
    if (prv->result != RES_OK) {
        return (-1);
    }

    atcmd_exec(prv, MODEM_CALLRESULT_ON);
    if (prv->result != RES_OK) {
        return (-1);
    }

    atcmd_exec(prv, MODEM_MAIL_RECEIVE_ON);
    if (prv->result != RES_OK) {
        return (-1);
    }
        
    atcmd_exec(prv, MODEM_CALLNUMBER_ON);
    if (prv->result != RES_OK) {
        return (-1);
    }

    return (0);
}


int
modem_command(modem *obj, const char *command)
{
    modem_private *prv;
    int result;

    prv = (modem_private *) obj->prv;

    if (prv->modem_state == MSTATE_DATA) {
        atcmd_escape(prv);
        result = serial_mode_canonical(prv->serial);
        if (result < 0) {
            return (-1);
        }

        result = atcmd_exec(prv, command);

        if (serial_mode_noncanonical(prv->serial) < 0) {
            return (-1);
        }
    } else {
        result = atcmd_exec(prv, command);
    }
    
    return (result);
}


result_type
modem_get_result_type(modem *obj)
{
    modem_private *prv;

    prv = (modem_private *) obj->prv;
    return (prv->result);
}


const char * const * 
modem_get_result_str(modem *obj)
{
    modem_private *prv;

    prv = (modem_private *) obj->prv;
    return ((const char * const *)&prv->result_lines);
}


ssize_t
modem_raw_read(modem *obj, uint8_t *buf, size_t size)
{
    modem_private *prv;
    ssize_t result;

    prv = (modem_private *) obj->prv;
    
    if (prv->modem_state == MSTATE_IDOL) {
        if (serial_mode_noncanonical(prv->serial) < 0) {
            return (-1);
        }
        if (serial_set_timeout(prv->serial, 0) < 0) {
            return (-2);
        }
        prv->modem_state = MSTATE_DATA;
    }
    
    result = serial_read(prv->serial, buf, size);
    return (result);
}


ssize_t
modem_raw_write(modem *obj, const uint8_t *buf, size_t size)
{
    modem_private *prv;
    ssize_t result;

    prv = (modem_private *) obj->prv;
    
    if (prv->modem_state == MSTATE_IDOL) {
        if (serial_mode_noncanonical(prv->serial) < 0) {
            return (-1);
        }
        if (serial_set_timeout(prv->serial, 0) < 0) {
            return (-2);
        }
        prv->modem_state = MSTATE_DATA;
    }
    
    result = serial_write(prv->serial, buf, size);
    return (result);
}


int
modem_rawmode(modem *obj)
{
    modem_private *prv;

    prv = (modem_private *) obj->prv;

    if (serial_mode_noncanonical(prv->serial) < 0) {
        return (-1);
    }
    if (serial_set_timeout(prv->serial, 0) < 0) {
        return (-2);
    }
    
    prv->modem_state = MSTATE_DATA;

    return (0);
}


int
modem_escape(modem *obj)
{
    modem_private *prv;
    prv = (modem_private *) obj->prv;

    if (prv->modem_state != MSTATE_IDOL) {
        atcmd_escape(prv);
        if (serial_mode_canonical(prv->serial) < 0) {
            return (-1);
        }
        if (serial_set_timeout(prv->serial, 2000) < 0) {
            return (-2);
        }
        prv->modem_state = MSTATE_IDOL;
    }

    return (0);
}


static int
atcmd_exec(modem_private *prv, const char *command)
{
    int result;

    prv->result = RES_UNKNOWN;
    prv->result_nlines = 0;
    
    result = atcmd_send(prv, command);
    if (result <= 0) {
        return (result);
    }
    
    result = atcmd_read_result(prv);
    if (result <= 0) {
        return (result);
    }
    
    return (0);
}


static int
atcmd_send(modem_private *prv, const char *command)
{
    int result;
    int length;
    char tx_buf[64];

#if defined(__linux)
    length = sprintf(tx_buf, "%s\r\n", command);
#else
    length = sprintf(tx_buf, "%s\r", command);
#endif
    result = serial_write(prv->serial, (uint8_t *)tx_buf, (size_t)length);

    dbg(DBG_AT0, "AT send: %s (result = %d)", command, result);
    p_mdelay(200);
    
    return (result);
}


static int
atcmd_read_result(modem_private *prv)
{
    int nread;
    int result;
    int nlines;

    nread = 0;
    prv->result = RES_UNKNOWN;
    prv->result_nlines = 0;

    for (nlines = 0; nlines < MAX_RESULT_LINES - 1; nlines++) {
        prv->result_lines[nlines] = prv->rx_buf + nread;
        result = serial_read(prv->serial, (uint8_t *)prv->result_lines[nlines], (size_t)(RX_BUFFER_SIZE - nread));
        if (result <= 0) {
            return (result);
        }
        
        /* remove trailing CR and LF's */
        while ((result > 0) && 
               (prv->result_lines[nlines][result - 1] == '\r' ||
                prv->result_lines[nlines][result - 1] == '\n')) {
            prv->result_lines[nlines][result - 1] = '\0';
            result--;
        }

        xSemaphoreTake(semaphore_puts, portMAX_DELAY);
        dbg(DBG_AT1, "AT result[%d]: %s (length = %d)", nlines, prv->result_lines[nlines], result);
        xSemaphoreGive(semaphore_puts);

        nread += result;
        prv->result_nlines = nlines + 1;
        
        if (atcmd_parse_result(prv) == 0) {
            prv->result_lines[prv->result_nlines] = NULL;
            break;
        }
    }
    
    return (prv->result_nlines);
}

static int
atcmd_parse_result(modem_private *prv)
{
    const result_type_list *p;
    
    for (p = result_list; p->result_str != NULL; p++) {
        if (strncmp(prv->result_lines[prv->result_nlines - 1], p->result_str, p->result_length) == 0) {
            prv->result = p->result;
            xSemaphoreTake(semaphore_puts, portMAX_DELAY);
            dbg(DBG_AT0, "AT result[%d]: parsed as %s", prv->result_nlines - 1, p->result_str);
            xSemaphoreGive(semaphore_puts);
            return (0);
        }
    }
    
    prv->result = RES_UNKNOWN;
    return (-1);
}

static int
atcmd_escape(modem_private *prv)
{
    int result;

    dbg(DBG_AT0, "sending +++");
    
    p_delay(0);
    result = serial_write(prv->serial, (uint8_t *)MODEM_ESCAPE, 3);
    p_delay(2);

    dbg(DBG_AT0, "escaped");
    
    return (result);
}


int32_t
modem_setup(serial *serialobj)
{
    uint32_t retry;
    modem_private *prv;


    psld.modemobj = modem_create();
    if (psld.modemobj == NULL) {
        printf("Unable to instantiate object: modem\n");
        goto fail;
    }

    if (modem_attach(psld.modemobj, serialobj) < 0) {
        printf("Unable to attach to modem\n");
        goto fail;
    }

    retry = 30;
    while(retry-->0){
        if (modem_initialize(psld.modemobj) < 0) {
            printf("Unable to initialize the modem\n");
            continue;
        }
           prv = (modem_private *)psld.modemobj->prv;

        serial_flush(prv->serial);
        if (modem_command(psld.modemobj, MODEM_CALL_DATALINK) < 0) {
            printf("ATD command failed\n");
            continue;
        }
        //p_delay(5);

        if (modem_get_result_type(psld.modemobj) != RES_CONNECT) {
            printf("Cannot connect\n");
            continue;
        }else{
            break;
        }
    }
    psld.modem_status = PPP_CONNECT;
    modem_rawmode(psld.modemobj);

    return (0);

fail:
    serial_close(serialobj);
    serial_destroy(serialobj);
    return (1);
}


uint32_t
modem_write(uint8_t *data, size_t length)
{
    modem_private *prv;

    prv = (modem_private *)psld.modemobj->prv;

    return serial_write(prv->serial, (uint8_t *) data, (size_t) length);
}


uint32_t
modem_read(uint8_t *data, size_t length)
{
    modem_private *prv;

    prv = (modem_private *)psld.modemobj->prv;

    return serial_read(prv->serial, (uint8_t *) data, (size_t) length);
}


/*
 * Write PHS Light Mail
 * @number : call number
 * @message: ASCII-code of message data(max 90 characters)
 * @return : status
 */
int32_t modem_mail_write(uint8_t *number, uint8_t *message)
{
    int32_t status, i, response;
    uint8_t *command, *answer;
    modem_private *prv;
    char *str;

    if(psld.modem_status==PPP_LINK){
        return -4;
    }
    modem_escape(psld.modemobj);

    prv = (modem_private *)psld.modemobj->prv;

    answer  = (uint8_t *)psld.modem_work_memory;
    command = answer + 40;
    memset(command, 0, LIGHTMAIL_MAX_SIZE);
    sprintf((char *)command, "%s=%s", MODEM_MAIL_DATA_TX, LIGHTMAIL_CODE);
    i = 0;
    while(1){
        sprintf((char *)&command[19+i], "%03d", *message++);
        if(*message=='\0'){
            break;
        }
        i += 3;
    }
    strcat((char *)command, "\r");
    status = serial_write(prv->serial, (uint8_t *)command, (size_t)strlen((char *)command));
    if(status<=0){
        return -1;
    }
    status = atcmd_read_result(prv);
    if(status<=0){
        return -2;
    }
    p_mdelay(500);

    memset(command, 0, LIGHTMAIL_ID_SIZE);
    sprintf((char *)command, "%s", MODEM_MAILCALL);
    strcat((char *)command, (char *)number);
    strcat((char *)command, "##0\r");

    status = serial_write(prv->serial, (uint8_t *)command, (size_t)strlen((char *)command));
    if(status<0){
        return -3;
    }
    response = 0;
    while((status = atcmd_read_result(prv))>0){
        str = prv->result_lines[status-1];
        if(strcmp(str, RESULT_PROG)==0){
            response |= 0x1;
        }else if(strcmp(str, RESULT_ALERT)==0){
            response |= 0x2;
        }else if(strncmp(str, RESULT_CONNECT, strlen(RESULT_CONNECT))==0){
            response |= 0x4;
        }else if(strcmp(str, RESULT_DISC)==0){
            response |= 0x8;
        }else if(strcmp(str, RESULT_NO_CARRIER)==0){
            response |= 0x10;
        }
    }
    serial_flush(prv->serial);
    if(response & 0x1){
        return 0;
    }else{
        return -response;
    }
}


/*
 * Get Light-mail from modem
 * @max_size : max size of [result]
 * @result   : pointer to address that store response data
 * @return   : status
 **/
int32_t modem_mail_read(uint8_t *result, size_t max_size)
{
    uint32_t i, bit, decimal, line_end;
    int32_t status;
    uint8_t  c, code[4];
    modem_private *prv;

    prv = (modem_private *)psld.modemobj->prv;

    /* find light mail code, and skip */
    modem_escape(psld.modemobj);
    i = 0;
    while(1){
        if((status = serial_read(prv->serial, &c, 1))<0){
            break;
        }
        if(c=='\n' || c=='\r'){
        }else{
            if(LIGHTMAIL_CODE[i]==c){
                i++;
                if(i==12){
                    break;
                }
            }
        }
    }

    /* receive data and convert to ASCII-code */
    i = bit = line_end = 0;
    while(1){
        if((status = serial_read(prv->serial, &c, 1))<0){
            break;
        }
        if(c=='\n' || c=='\r'){
        }else{
            code[bit++] = c;
            if(bit==3){
                code[bit] = '\0';
                decimal = atoi((char *)code);
                if(decimal==129){
                    line_end = 1;
                }
                if(line_end==0){
                    result[i++] = decimal;
                }
                bit = 0;
            }
        }
    }
    result[i] = '\0';
    return 0;
}

/*
 * Get right-mail(ID and message)
 * @number : pointer to address that tore call number
 * @message: pointer to address that store message
 * @return : None
 */
int32_t modem_mail_get(int8_t **number, int8_t **message)
{
    int32_t status;
    uint8_t *answer;
    modem_private *prv;

    prv = (modem_private *)psld.modemobj->prv;

    arduino_interruput(0);

    answer = (uint8_t *)psld.modem_work_memory;
    memset(answer, 0, MODEM_WORKING_SIZE);
    atcmd_send(prv, MODEM_MAIL_DATA_RX);

    status = modem_mail_read(answer, MODEM_WORKING_SIZE);
    if(status>0){
        return -1;
    }
    *number = (int8_t *)modem_lightmail.id;
    *message = (int8_t *)answer;
    modem_lightmail.flag = 0;
    return 0;
}

/*
 * Check RING Call from modem
 * @parameter: none
 * @return   : [1:RING CALL, 0:No RING Call]
 */
int32_t modem_check_ringcall()
{
    uint32_t i, max_size;
    int32_t status;
    uint8_t c, *answer, *position;
    modem_private *prv;

    prv = (modem_private *)psld.modemobj->prv;

    max_size = 45;
    answer = (uint8_t *)psld.modem_work_memory;

    modem_escape(psld.modemobj);
    i = 0;
    while(1){
        if((status = serial_read(prv->serial, &c, 1))<0){
            break;
        }
        if(c=='\n' || c=='\r'){
        }else{
            if(i<max_size){
                answer[i++] = c;
            }
        }
    }
    answer[i] = '\0';

    if(strstr((char *)answer, RESULT_RING)==0){
        return -3;
    }
    modem_lightmail.flag = 1;

    position = (uint8_t *)strstr((char *)answer, RESULT_ID);
    if(position==0){
        printf("RING call from modem\n");
        return 1;
    }
    strncpy((char *)modem_lightmail.id, (char *)(position+3), 11);
    modem_lightmail.id[12] = '\0';
    printf("RING call[%s] from modem\n", modem_lightmail.id);
    return 2;
}

/*
 * Check RING Call from modem on interrupt handle routine
 * @parameter: none
 * @return   : None
 */
void modem_check_ringcall_interrupt()
{
    uint16_t status, i;
    status=0;
    i=0;
    while(status==0){
        status=modem_check_ringcall();
        if(status>0){
            arduino_interruput(1);
        }
        if(i++>100){
            break;
        }
    }
}

/*
 * Check if mail is received
 * @parameter: none
 * @return   : 1:mail is received, 0:no mail
 */
int32_t modem_mail_available()
{
    if(psld.modem_status == PPP_LINK){
        return 1;
    }
    if(modem_lightmail.flag){
        return 0;
    }else{
        return 1;
    }
}


/*
 * Check RSSI
 * @parameter: none
 * @return   : [1:can not read, 0:can read rssi]
 */
int32_t modem_rssi_get(uint8_t *rssi)
{
    uint8_t i, c, detect;
    modem_private *prv;
    int32_t status;

    if(psld.modem_status == PPP_LINK){
        return 1;
    }
    prv = (modem_private *)psld.modemobj->prv;
    serial_flush(prv->serial);

    status = atcmd_send(prv, MODEM_CSIDRSSI);
    if(status<=0){
        return 2;
    }
    p_delay(3);

    i = 0;
    detect = 0;
    while(1){
        if((status = serial_read(prv->serial, &c, 1))<0){
            break;
        }
        if(c=='\0'){
            break;
        }else if(c==','){
            detect = 1;
        }else if(detect && isdigit(c)){
            rssi[i++] = c;
        }
        if(i>1){
            break;
        }
    }
    rssi[i++] = '\0';

    return 0;
}

/*
 * Check IMEI(My phone number)
 * @parameter: none
 * @return   : [1:can not read, 0:can read rssi]
 */
int32_t modem_imei_get(uint8_t *imei)
{
    uint8_t c, i;
    int32_t status;
    modem_private *prv;

    if(psld.modem_status == PPP_LINK){
        return 1;
    }
    prv = (modem_private *)psld.modemobj->prv;
    serial_flush(prv->serial);

    status = atcmd_send(prv, MODEM_IMEI);
    if(status<=0){
        return 2;
    }

    i = 0;
    while(1){
        if((status = serial_read(prv->serial, &c, 1))<0){
            break;
        }
        if(c=='\0'){
            break;
        }else{
            if(isdigit(c)){
                imei[i++] = c;
            }
        }
        if(i>10){
            break;
        }
    }
    imei[i++] = '\0';

    return 0;
}


/*
 * Check Location(CS-ID)
 * @parameter: none
 * @return   : [1:can not read, 0:can read rssi]
 */
int32_t modem_location_get(uint8_t *location)
{
    uint8_t c, i, step;
    int32_t status;
    modem_private *prv;

    if(psld.modem_status == PPP_LINK){
        return 1;
    }
    prv = (modem_private *)psld.modemobj->prv;

    status = atcmd_send(prv, MODEM_CSID);
    if(status<=0){
        return 2;
    }
    p_mdelay(2000);
    serial_flush(prv->serial);
    status = atcmd_send(prv, MODEM_CSID);
    if(status<=0){
        return 2;
    }

    i = 0;
    step = 0;
    while(1){
        if((status = serial_read(prv->serial, &c, 1))<0){
            break;
        }
        if(c=='\0'){
            break;
        }
        if(c=='N'){
            step = 1;
            continue;
        }
        if(c=='E'){
            step = 2;
            continue;
        }
        if(step == 0 && (c=='\r' || c=='\n')){
            continue;
        }
        if(step == 1 && (c=='\r' || c=='\n')){
            location[i++] = ' ';
            continue;
        }
        if(step == 2 && (c=='\r' || c=='\n')){
            break;
        }
        if(step){
            if(c==':'){
                location[i++] = '.';
            }else{
                location[i++] = c;
            }
        }
    }
    location[i++] = '\0';

    return 0;
}



/*
 * PPP re-link up routine
 * @parameter: none
 * @return   : none
 */
uint32_t modem_ppp_link_reconnect()
{
    uint32_t timeout_count;

    psld.ppp_linkup_retry++;
    modem_escape(psld.modemobj);

    while(1){
        if (modem_initialize_sequense(psld.modemobj)) {
            continue;
        }

        if (modem_command(psld.modemobj, MODEM_CALL_DATALINK) < 0) {
            printf("ATD command failed\n");
            continue;
        }

        if (modem_get_result_type(psld.modemobj) != RES_CONNECT) {
            printf("Cannot connect\n");
            continue;
        }else{
            break;
        }
    }
    psld.modem_status = PPP_CONNECT;

    modem_rawmode(psld.modemobj);
    p_delay(3);

    lwip_main_restart();

    printf("MODEM: wait link up\n");
    timeout_count = 0;
    while(psld.modem_status != PPP_LINK){
        p_mdelay(500);
        if(timeout_count++>MODEM_LINKUP_TIMEOUT*2){
            break;
        }
    }
    if(timeout_count>MODEM_LINKUP_TIMEOUT*2){
        printf("MODEM: link up timeout\n");
        if(psld.ppp_linkup_retry>=PPPLINKUPRETRY_TIMEOUT){
            rtc_backup_write(19, SYSRESET_PPPLINKRETRY);
            printf_queued_flush();
            NVIC_SystemReset();
        }
        return 2;
    }
    psld.ppp_linkup_retry = 0;
    return 3;
}


/*
 * Check network and modem status
 * @parameter: none
 * @return   : none
 */
uint32_t modem_ppp_link_check()
{
    uint32_t timeout_count;

    if(psld.modem_status == PPP_LINK){
        return 0;
    }
    printf("MODEM: %d\n", psld.modem_status);

    if(psld.modem_status == PPP_CONNECT){
        printf("MODEM: wait link up\n");
        timeout_count = 0;
        while(psld.modem_status != PPP_LINK){
            p_mdelay(500);
            if(timeout_count++>MODEM_LINKUP_TIMEOUT*2){
                break;
            }
        }
        if(timeout_count>MODEM_LINKUP_TIMEOUT*2){
            printf("MODEM: link up timeout\n");
            return 1;
        }else{
            return 0;
        }
    }

    return modem_ppp_link_reconnect();
}


/*
 * Check regularly routine modem status, ppp link up by force if ppp link down
 * @parameter: none
 * @return   : none
 */
void modem_ppp_linkup_surveillance()
{
    if(psld.profile_number & PROFILE_PPP_LINKDOWN){
        return;
    }
    if(psld.modem_status != PPP_NO_LINK){
        return;
    }
    if(psld.ppp_linkup_retry == 0){
#if 1 //zeus20150517
        rtc_backup_write(19, SYSRESET_PPPREBOOT);
        printf_queued_flush();
        NVIC_SystemReset();
#else
        modem_ppp_link_reconnect();
        p_delay(10);
#endif
    }
}


/*
 * Update profile
 * @parameter: none
 * @return   : none
 */
void profile_update()
{
    psld.profile_number = (psld.profile_number & 0xff) | (psld.modem_status << 8);
}


/*
 * Change modem mode[ppp link up/down]
 * @parameter: none
 * @return   : none
 */
void modem_mode_change()
{
    modem_private *prv;

    prv = (modem_private *)psld.modemobj->prv;

    if(psld.profile_number & PROFILE_PPP_LINKDOWN){
        if(psld.modem_status == PPP_LINK){
            puts("ppp link down request");

            lwip_main_close(0);

            modem_escape(psld.modemobj);
            serial_flush(prv->serial);
            atcmd_exec(prv, MODEM_MAIL_RECEIVE_ON);
            p_mdelay(10);
            atcmd_exec(prv, MODEM_CSID_ON);
        }
    }else{
        puts("ppp link up request");

        if(psld.modem_status != PPP_LINK){
            modem_ppp_link_reconnect();
        }
    }
}

